#include "AssetRegistry.h"
#include "../Core/Logger.h"
#include <random>
#include <algorithm>

namespace Gini {

void AssetRegistry::ScanDirectory(const std::filesystem::path& directory) {
  if (!std::filesystem::exists(directory)) {
    GINI_ERROR("Asset directory not found: ", directory.string());
    return;
  }
  
  m_RootPath = directory;
  Clear();
  ScanDirectoryRecursive(directory, "");
  GINI_INFO("Scanned ", m_Assets.size(), " assets in ", directory.string());
}

void AssetRegistry::ScanDirectoryRecursive(const std::filesystem::path& directory, 
                                            const std::filesystem::path& relativePath) {
  for (const auto& entry : std::filesystem::directory_iterator(directory)) {
    std::string filename = entry.path().filename().string();
    
    // Skip hidden files and directories
    if (filename[0] == '.') continue;
    
    std::filesystem::path relPath = relativePath / filename;
    
    if (entry.is_directory()) {
      // Register directory
      AssetMetadata meta;
      meta.uuid = GenerateUUID();
      meta.name = filename;
      meta.path = relPath;
      meta.absolutePath = entry.path();
      meta.type = AssetType::Unknown;
      meta.isDirectory = true;
      meta.lastModified = std::filesystem::last_write_time(entry.path()).time_since_epoch().count();
      
      m_Assets[meta.uuid] = meta;
      m_PathToUUID[relPath.string()] = meta.uuid;
      
      // Recurse into subdirectory
      ScanDirectoryRecursive(entry.path(), relPath);
    } else {
      // Register file
      std::string extension = entry.path().extension().string();
      AssetType type = GetAssetTypeFromExtension(extension);
      
      AssetMetadata meta;
      meta.uuid = GenerateUUID();
      meta.name = filename;
      meta.path = relPath;
      meta.absolutePath = entry.path();
      meta.type = type;
      meta.isDirectory = false;
      meta.lastModified = std::filesystem::last_write_time(entry.path()).time_since_epoch().count();
      
      m_Assets[meta.uuid] = meta;
      m_PathToUUID[relPath.string()] = meta.uuid;
    }
  }
}

void AssetRegistry::Refresh() {
  if (m_RootPath.empty()) return;
  ScanDirectory(m_RootPath);
}

void AssetRegistry::Clear() {
  m_Assets.clear();
  m_PathToUUID.clear();
}

const AssetMetadata* AssetRegistry::GetMetadata(u64 uuid) const {
  auto it = m_Assets.find(uuid);
  if (it != m_Assets.end()) {
    return &it->second;
  }
  return nullptr;
}

const AssetMetadata* AssetRegistry::GetMetadata(const std::filesystem::path& path) const {
  auto it = m_PathToUUID.find(path.string());
  if (it != m_PathToUUID.end()) {
    return GetMetadata(it->second);
  }
  return nullptr;
}

std::vector<AssetMetadata> AssetRegistry::GetAssetsOfType(AssetType type) const {
  std::vector<AssetMetadata> result;
  for (const auto& [uuid, meta] : m_Assets) {
    if (meta.type == type) {
      result.push_back(meta);
    }
  }
  return result;
}

std::vector<AssetMetadata> AssetRegistry::GetAssetsInDirectory(const std::filesystem::path& directory) const {
  std::vector<AssetMetadata> result;
  
  for (const auto& [uuid, meta] : m_Assets) {
    if (meta.path.parent_path() == directory || 
        (directory.empty() && meta.path.parent_path().empty())) {
      result.push_back(meta);
    }
  }
  
  // Sort: directories first, then by name
  std::sort(result.begin(), result.end(), [](const AssetMetadata& a, const AssetMetadata& b) {
    if (a.isDirectory != b.isDirectory) {
      return a.isDirectory > b.isDirectory;
    }
    return a.name < b.name;
  });
  
  return result;
}

u64 AssetRegistry::RegisterAsset(const std::filesystem::path& path, AssetType type) {
  // Check if already registered
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
  meta.isDirectory = std::filesystem::is_directory(meta.absolutePath);
  
  if (std::filesystem::exists(meta.absolutePath)) {
    meta.lastModified = std::filesystem::last_write_time(meta.absolutePath).time_since_epoch().count();
  }
  
  m_Assets[meta.uuid] = meta;
  m_PathToUUID[path.string()] = meta.uuid;
  
  if (m_OnAssetChanged) {
    m_OnAssetChanged(meta);
  }
  
  return meta.uuid;
}

void AssetRegistry::UnregisterAsset(u64 uuid) {
  auto it = m_Assets.find(uuid);
  if (it != m_Assets.end()) {
    m_PathToUUID.erase(it->second.path.string());
    m_Assets.erase(it);
  }
}

AssetType AssetRegistry::GetAssetTypeFromExtension(const std::string& extension) {
  std::string ext = extension;
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  
  // Textures
  if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || 
      ext == ".tga" || ext == ".bmp" || ext == ".hdr" || ext == ".exr") {
    return AssetType::Texture;
  }
  
  // Materials
  if (ext == ".ginimat" || ext == ".mat") {
    return AssetType::Material;
  }
  
  // Meshes
  if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae") {
    return AssetType::Mesh;
  }
  
  // Scenes
  if (ext == ".giniscene" || ext == ".scene") {
    return AssetType::Scene;
  }
  
  // Audio
  if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") {
    return AssetType::Audio;
  }
  
  // Scripts
  if (ext == ".lua" || ext == ".cs" || ext == ".cpp" || ext == ".h") {
    return AssetType::Script;
  }
  
  // Shaders
  if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".shader") {
    return AssetType::Shader;
  }
  
  // Fonts
  if (ext == ".ttf" || ext == ".otf") {
    return AssetType::Font;
  }
  
  return AssetType::Unknown;
}

const char* AssetRegistry::AssetTypeToString(AssetType type) {
  switch (type) {
    case AssetType::Texture:  return "Texture";
    case AssetType::Material: return "Material";
    case AssetType::Mesh:     return "Mesh";
    case AssetType::Scene:    return "Scene";
    case AssetType::Audio:    return "Audio";
    case AssetType::Script:   return "Script";
    case AssetType::Shader:   return "Shader";
    case AssetType::Font:     return "Font";
    default:                  return "Unknown";
  }
}

const char* AssetRegistry::GetAssetTypeIcon(AssetType type) {
  switch (type) {
    case AssetType::Texture:  return "🖼";
    case AssetType::Material: return "🎨";
    case AssetType::Mesh:     return "📦";
    case AssetType::Scene:    return "🎬";
    case AssetType::Audio:    return "🔊";
    case AssetType::Script:   return "📜";
    case AssetType::Shader:   return "✨";
    case AssetType::Font:     return "🔤";
    default:                  return "📄";
  }
}

u64 AssetRegistry::GenerateUUID() {
  static std::random_device rd;
  static std::mt19937_64 gen(rd());
  static std::uniform_int_distribution<u64> dis;
  return dis(gen);
}

} // namespace Gini
