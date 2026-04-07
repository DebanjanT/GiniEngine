#pragma once

#include "../Core/Types.h"
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace Gini {

enum class AssetType {
  Unknown = 0,
  Texture,
  Material,
  Mesh,
  Scene,
  Audio,
  Script,
  Shader,
  Font
};

struct AssetMetadata {
  u64 uuid = 0;
  std::string name;
  std::filesystem::path path;        // Relative to assets folder
  std::filesystem::path absolutePath;
  AssetType type = AssetType::Unknown;
  u64 lastModified = 0;
  bool isLoaded = false;
  bool isDirectory = false;
};

class AssetRegistry {
public:
  static AssetRegistry& Get() {
    static AssetRegistry instance;
    return instance;
  }
  
  // Scan project assets directory
  void ScanDirectory(const std::filesystem::path& directory);
  void Refresh();
  void Clear();
  
  // Asset queries
  const AssetMetadata* GetMetadata(u64 uuid) const;
  const AssetMetadata* GetMetadata(const std::filesystem::path& path) const;
  std::vector<AssetMetadata> GetAssetsOfType(AssetType type) const;
  std::vector<AssetMetadata> GetAssetsInDirectory(const std::filesystem::path& directory) const;
  
  // Asset registration
  u64 RegisterAsset(const std::filesystem::path& path, AssetType type);
  void UnregisterAsset(u64 uuid);
  
  // Type detection
  static AssetType GetAssetTypeFromExtension(const std::string& extension);
  static const char* AssetTypeToString(AssetType type);
  static const char* GetAssetTypeIcon(AssetType type);
  
  // Callbacks
  using AssetChangedCallback = std::function<void(const AssetMetadata&)>;
  void SetOnAssetChanged(AssetChangedCallback callback) { m_OnAssetChanged = callback; }
  
  const std::unordered_map<u64, AssetMetadata>& GetAllAssets() const { return m_Assets; }
  
  void SetRootPath(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_RootPath = path;
  }
  std::filesystem::path GetRootPath() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_RootPath;
  }

private:
  AssetRegistry() = default;
  
  void ScanDirectoryRecursive(const std::filesystem::path& directory, const std::filesystem::path& relativePath);
  bool ShouldSkipDirectoryName(const std::string& name) const;
  u64 GenerateUUID();
  
  mutable std::mutex m_Mutex;
  std::unordered_map<u64, AssetMetadata> m_Assets;
  std::unordered_map<std::string, u64> m_PathToUUID;  // path string -> UUID
  std::filesystem::path m_RootPath;
  static constexpr size_t MAX_ASSET_ENTRIES = 20000;
  
  AssetChangedCallback m_OnAssetChanged;
};

} // namespace Gini
