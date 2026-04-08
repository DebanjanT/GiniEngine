#pragma once

#include "EditorPanel.h"
#include "Asset/AssetRegistry.h"
#include "Renderer/Texture.h"
#include <filesystem>
#include <vector>
#include <unordered_map>

namespace Gini {

class AssetBrowserPanel : public EditorPanel {
public:
  AssetBrowserPanel();
  ~AssetBrowserPanel() = default;
  
  void OnImGuiRender() override;
  void SetRootPath(const std::filesystem::path& path);
  void Refresh();
  
  // Drag-drop payload types
  static constexpr const char* PAYLOAD_ASSET = "ASSET_BROWSER_ITEM";
  static constexpr const char* PAYLOAD_TEXTURE = "ASSET_TEXTURE";
  static constexpr const char* PAYLOAD_MATERIAL = "ASSET_MATERIAL";
  static constexpr const char* PAYLOAD_MESH = "ASSET_MESH";

private:
  void DrawTopBar();
  void DrawDirectoryTree();
  void DrawDirectoryTreeNode(const std::filesystem::path& directory);
  void DrawContentArea();
  void DrawAssetItem(const AssetMetadata& asset);
  void DrawContextMenu();
  
  void NavigateTo(const std::filesystem::path& directory);
  void NavigateBack();
  void NavigateForward();
  
  Ref<Texture2D> GetIconForAssetType(AssetType type);
  Ref<Texture2D> GetThumbnail(const AssetMetadata& asset);
  
  std::filesystem::path m_RootPath;
  std::filesystem::path m_CurrentDirectory;
  std::vector<std::filesystem::path> m_BackHistory;
  std::vector<std::filesystem::path> m_ForwardHistory;
  
  // Icons
  Ref<Texture2D> m_FolderIcon;
  Ref<Texture2D> m_FileIcon;
  Ref<Texture2D> m_TextureIcon;
  Ref<Texture2D> m_MaterialIcon;
  Ref<Texture2D> m_MeshIcon;
  Ref<Texture2D> m_SceneIcon;
  
  // Thumbnail cache
  std::unordered_map<std::string, Ref<Texture2D>> m_ThumbnailCache;
  
  // UI state
  f32 m_ThumbnailSize = 96.0f;
  f32 m_Padding = 16.0f;
  std::string m_SearchFilter;
  AssetMetadata* m_SelectedAsset = nullptr;
  bool m_ShowDirectoryTree = true;
};

} // namespace Gini
