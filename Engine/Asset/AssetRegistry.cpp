#include "AssetRegistry.h"
#include "../Core/Logger.h"
#include <algorithm>
#include <random>

namespace Gini {

bool AssetRegistry::ShouldSkipDirectoryName(const std::string &name) const {
  static const std::vector<std::string> skipDirs = {
      ".git", ".svn", ".hg", "build", "Build", "Binaries", "Intermediate",
      "DerivedDataCache", "node_modules", ".cache", "__pycache__"};
  return std::find(skipDirs.begin(), skipDirs.end(), name) != skipDirs.end();
}

void AssetRegistry::ScanDirectory(const std::filesystem::path &directory) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  try {
    std::error_code ec;
    if (!std::filesystem::exists(directory, ec) || ec) {
      GINI_ERROR("Asset directory not found: ", directory.string());
      return;
    }

    m_RootPath = directory;
    m_Assets.clear();
    m_PathToUUID.clear();

    ScanDirectoryRecursive(directory, "");
    GINI_INFO("Scanned ", m_Assets.size(), " assets in ", directory.string());
  } catch (const std::exception &e) {
    GINI_ERROR("AssetRegistry scan failed for ", directory.string(), " reason: ",
               e.what());
    m_Assets.clear();
    m_PathToUUID.clear();
  }
}

void AssetRegistry::ScanDirectoryRecursive(
    const std::filesystem::path &directory,
    const std::filesystem::path &relativePath) {
  std::error_code dirError;
  auto it = std::filesystem::directory_iterator(
      directory, std::filesystem::directory_options::skip_permission_denied,
      dirError);
  if (dirError) {
    GINI_WARN("Failed to open directory: ", directory.string(),
              " error: ", dirError.message());
    return;
  }

  for (const auto &entry : it) {
    if (m_Assets.size() >= MAX_ASSET_ENTRIES) {
      GINI_WARN("Asset registry limit reached (", MAX_ASSET_ENTRIES,
                "). Stopping scan.");
      return;
    }

    std::error_code entryError;
    if (entry.is_symlink(entryError) || entryError) {
      continue;
    }

    std::string filename = entry.path().filename().string();
    if (filename.empty() || filename[0] == '.') {
      continue;
    }

    std::filesystem::path relPath = relativePath / filename;

    if (entry.is_directory(entryError) && !entryError) {
      if (ShouldSkipDirectoryName(filename)) {
        continue;
      }

      AssetMetadata meta;
      meta.uuid = GenerateUUID();
      meta.name = filename;
      meta.path = relPath;
      meta.absolutePath = entry.path();
      meta.type = AssetType::Unknown;
      meta.isDirectory = true;

      std::error_code timeError;
      auto lastWrite =
          std::filesystem::last_write_time(entry.path(), timeError);
      if (!timeError) {
        meta.lastModified = lastWrite.time_since_epoch().count();
      }

      u64 uuid = meta.uuid;
      m_Assets[uuid] = std::move(meta);
      m_PathToUUID[relPath.string()] = uuid;

      ScanDirectoryRecursive(entry.path(), relPath);
    } else if (!entryError) {
      std::string extension = entry.path().extension().string();
      AssetType type = GetAssetTypeFromExtension(extension);

      AssetMetadata meta;
      meta.uuid = GenerateUUID();
      meta.name = filename;
      meta.path = relPath;
      meta.absolutePath = entry.path();
      meta.type = type;
      meta.isDirectory = false;

      std::error_code timeError;
      auto lastWrite =
          std::filesystem::last_write_time(entry.path(), timeError);
      if (!timeError) {
        meta.lastModified = lastWrite.time_since_epoch().count();
      }

      u64 uuid = meta.uuid;
      m_Assets[uuid] = std::move(meta);
      m_PathToUUID[relPath.string()] = uuid;
    }
  }
}

void AssetRegistry::Refresh() {
  std::filesystem::path rootCopy;
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    rootCopy = m_RootPath;
  }
  if (rootCopy.empty()) {
    return;
  }
  ScanDirectory(rootCopy);
}

void AssetRegistry::Clear() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Assets.clear();
  m_PathToUUID.clear();
}

const AssetMetadata *AssetRegistry::GetMetadata(u64 uuid) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Assets.find(uuid);
  if (it != m_Assets.end()) {
    return &it->second;
  }
  return nullptr;
}

const AssetMetadata *
AssetRegistry::GetMetadata(const std::filesystem::path &path) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_PathToUUID.find(path.string());
  if (it != m_PathToUUID.end()) {
    auto assetIt = m_Assets.find(it->second);
    if (assetIt != m_Assets.end()) {
      return &assetIt->second;
    }
  }
  return nullptr;
}

std::vector<AssetMetadata>
AssetRegistry::GetAssetsOfType(AssetType type) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  std::vector<AssetMetadata> result;
  for (const auto &[uuid, meta] : m_Assets) {
    if (meta.type == type) {
      result.push_back(meta);
    }
  }
  return result;
}

std::vector<AssetMetadata> AssetRegistry::GetAssetsInDirectory(
    const std::filesystem::path &directory) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  std::vector<AssetMetadata> result;

  for (const auto &[uuid, meta] : m_Assets) {
    if (meta.path.parent_path() == directory ||
        (directory.empty() && meta.path.parent_path().empty())) {
      result.push_back(meta);
    }
  }

  std::sort(result.begin(), result.end(),
            [](const AssetMetadata &a, const AssetMetadata &b) {
              if (a.isDirectory != b.isDirectory) {
                return a.isDirectory > b.isDirectory;
              }
              return a.name < b.name;
            });

  return result;
}

u64 AssetRegistry::RegisterAsset(const std::filesystem::path &path,
                                 AssetType type) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_PathToUUID.find(path.string());
  if (it != m_PathToUUID.end()) {
    return it->second;
  }

  AssetMetadata meta;
  meta.uuid = GenerateUUID();
  meta.name = path.filename().string();
  meta.path = path;
  meta.absolutePath = m_RootPath / path;
  meta.type = type;

  std::error_code ec;
  meta.isDirectory = std::filesystem::is_directory(meta.absolutePath, ec);
  if (std::filesystem::exists(meta.absolutePath, ec) && !ec) {
    std::error_code timeError;
    auto lastWrite =
        std::filesystem::last_write_time(meta.absolutePath, timeError);
    if (!timeError) {
      meta.lastModified = lastWrite.time_since_epoch().count();
    }
  }

  u64 uuid = meta.uuid;
  m_Assets[uuid] = std::move(meta);
  m_PathToUUID[path.string()] = uuid;

  if (m_OnAssetChanged) {
    m_OnAssetChanged(m_Assets[uuid]);
  }

  return uuid;
}

void AssetRegistry::UnregisterAsset(u64 uuid) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Assets.find(uuid);
  if (it != m_Assets.end()) {
    m_PathToUUID.erase(it->second.path.string());
    m_Assets.erase(it);
  }
}

AssetType
AssetRegistry::GetAssetTypeFromExtension(const std::string &extension) {
  std::string ext = extension;
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" ||
      ext == ".bmp" || ext == ".hdr" || ext == ".exr") {
    return AssetType::Texture;
  }
  if (ext == ".ginimat" || ext == ".mat" || ext == ".gmat") {
    return AssetType::Material;
  }
  if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" ||
      ext == ".dae" || ext == ".gmesh") {
    return AssetType::Mesh;
  }
  if (ext == ".giniscene" || ext == ".scene" || ext == ".gscene") {
    return AssetType::Scene;
  }
  if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") {
    return AssetType::Audio;
  }
  if (ext == ".lua" || ext == ".cs" || ext == ".cpp" || ext == ".h") {
    return AssetType::Script;
  }
  if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".shader") {
    return AssetType::Shader;
  }
  if (ext == ".ttf" || ext == ".otf") {
    return AssetType::Font;
  }

  return AssetType::Unknown;
}

const char *AssetRegistry::AssetTypeToString(AssetType type) {
  switch (type) {
  case AssetType::Texture:
    return "Texture";
  case AssetType::Material:
    return "Material";
  case AssetType::Mesh:
    return "Mesh";
  case AssetType::Scene:
    return "Scene";
  case AssetType::Audio:
    return "Audio";
  case AssetType::Script:
    return "Script";
  case AssetType::Shader:
    return "Shader";
  case AssetType::Font:
    return "Font";
  default:
    return "Unknown";
  }
}

const char *AssetRegistry::GetAssetTypeIcon(AssetType type) {
  switch (type) {
  case AssetType::Texture:
    return "T";
  case AssetType::Material:
    return "M";
  case AssetType::Mesh:
    return "3D";
  case AssetType::Scene:
    return "S";
  case AssetType::Audio:
    return "A";
  case AssetType::Script:
    return "Sc";
  case AssetType::Shader:
    return "Sh";
  case AssetType::Font:
    return "F";
  default:
    return "?";
  }
}

u64 AssetRegistry::GenerateUUID() {
  static std::random_device rd;
  static std::mt19937_64 gen(rd());
  static std::uniform_int_distribution<u64> dis;
  static std::mutex uuidMutex;

  std::lock_guard<std::mutex> lock(uuidMutex);
  return dis(gen);
}

} // namespace Gini
