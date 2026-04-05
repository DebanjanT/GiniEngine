#include "AssetBrowserPanel.h"
#include "Core/Logger.h"
#include "Project/Project.h"
#include <algorithm>
#include <fstream>
#include <imgui.h>
#include <yaml-cpp/yaml.h>

namespace Gini {

AssetBrowserPanel::AssetBrowserPanel() : EditorPanel("Asset Browser") {}

void AssetBrowserPanel::SetRootPath(const std::filesystem::path &path) {
  m_RootPath = path;
  m_CurrentDirectory = path;
  AssetRegistry::Get().SetRootPath(path);
  AssetRegistry::Get().ScanDirectory(path);
}

void AssetBrowserPanel::Refresh() { AssetRegistry::Get().Refresh(); }

void AssetBrowserPanel::OnImGuiRender() {
  ImGui::Begin("Asset Browser", &m_Visible);

  DrawTopBar();
  ImGui::Separator();

  // Split view: directory tree on left, content on right
  float treeWidth = m_ShowDirectoryTree ? 200.0f : 0.0f;

  if (m_ShowDirectoryTree) {
    ImGui::BeginChild("DirectoryTree", ImVec2(treeWidth, 0), true);
    DrawDirectoryTree();
    ImGui::EndChild();

    ImGui::SameLine();
  }

  ImGui::BeginChild("ContentArea", ImVec2(0, 0), true);
  DrawContentArea();
  ImGui::EndChild();

  // Context menu
  DrawContextMenu();

  ImGui::End();
}

void AssetBrowserPanel::DrawTopBar() {
  // Navigation buttons
  if (ImGui::Button("<")) {
    NavigateBack();
  }
  ImGui::SameLine();
  if (ImGui::Button(">")) {
    NavigateForward();
  }
  ImGui::SameLine();
  if (ImGui::Button("^")) {
    if (m_CurrentDirectory != m_RootPath &&
        m_CurrentDirectory.has_parent_path()) {
      NavigateTo(m_CurrentDirectory.parent_path());
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Refresh")) {
    Refresh();
  }

  ImGui::SameLine();
  ImGui::Checkbox("Tree", &m_ShowDirectoryTree);

  // Current path display
  ImGui::SameLine();
  std::filesystem::path relativePath =
      std::filesystem::relative(m_CurrentDirectory, m_RootPath);
  std::string pathStr = "Assets";
  if (!relativePath.empty() && relativePath != ".") {
    pathStr += "/" + relativePath.string();
  }
  ImGui::Text("%s", pathStr.c_str());

  // Search filter
  ImGui::SameLine(ImGui::GetWindowWidth() - 250);
  ImGui::SetNextItemWidth(150);
  char searchBuffer[256];
  strncpy(searchBuffer, m_SearchFilter.c_str(), sizeof(searchBuffer));
  if (ImGui::InputText("##Search", searchBuffer, sizeof(searchBuffer))) {
    m_SearchFilter = searchBuffer;
  }

  // Thumbnail size slider
  ImGui::SameLine();
  ImGui::SetNextItemWidth(80);
  ImGui::SliderFloat("##Size", &m_ThumbnailSize, 48.0f, 256.0f, "%.0f");
}

void AssetBrowserPanel::DrawDirectoryTree() {
  if (m_RootPath.empty()) {
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No project loaded");
    return;
  }

  ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
  if (m_CurrentDirectory == m_RootPath) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }

  bool opened =
      ImGui::TreeNodeEx("Assets", flags | ImGuiTreeNodeFlags_DefaultOpen);

  if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
    NavigateTo(m_RootPath);
  }

  if (opened) {
    DrawDirectoryTreeNode(m_RootPath);
    ImGui::TreePop();
  }
}

void AssetBrowserPanel::DrawDirectoryTreeNode(
    const std::filesystem::path &directory) {
  if (!std::filesystem::exists(directory))
    return;

  for (const auto &entry : std::filesystem::directory_iterator(directory)) {
    if (!entry.is_directory())
      continue;

    std::string filename = entry.path().filename().string();
    if (filename[0] == '.')
      continue; // Skip hidden

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    // Check if this directory has subdirectories
    bool hasSubdirs = false;
    for (const auto &subEntry :
         std::filesystem::directory_iterator(entry.path())) {
      if (subEntry.is_directory() &&
          subEntry.path().filename().string()[0] != '.') {
        hasSubdirs = true;
        break;
      }
    }

    if (!hasSubdirs) {
      flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (entry.path() == m_CurrentDirectory) {
      flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool opened = ImGui::TreeNodeEx(filename.c_str(), flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
      NavigateTo(entry.path());
    }

    if (opened) {
      DrawDirectoryTreeNode(entry.path());
      ImGui::TreePop();
    }
  }
}

void AssetBrowserPanel::DrawContentArea() {
  if (m_RootPath.empty()) {
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No project loaded");
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                       "Create or open a project to browse assets");
    return;
  }

  float cellSize = m_ThumbnailSize + m_Padding;
  float panelWidth = ImGui::GetContentRegionAvail().x;
  int columnCount = (int)(panelWidth / cellSize);
  if (columnCount < 1)
    columnCount = 1;

  ImGui::Columns(columnCount, nullptr, false);

  // Get assets in current directory
  std::filesystem::path relativePath =
      std::filesystem::relative(m_CurrentDirectory, m_RootPath);
  if (relativePath == ".")
    relativePath = "";

  auto assets = AssetRegistry::Get().GetAssetsInDirectory(relativePath);

  for (const auto &asset : assets) {
    // Apply search filter
    if (!m_SearchFilter.empty()) {
      std::string lowerName = asset.name;
      std::string lowerFilter = m_SearchFilter;
      std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                     ::tolower);
      std::transform(lowerFilter.begin(), lowerFilter.end(),
                     lowerFilter.begin(), ::tolower);
      if (lowerName.find(lowerFilter) == std::string::npos) {
        continue;
      }
    }

    DrawAssetItem(asset);
    ImGui::NextColumn();
  }

  ImGui::Columns(1);
}

void AssetBrowserPanel::DrawAssetItem(const AssetMetadata &asset) {
  ImGui::PushID(asset.name.c_str());

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

  // Draw thumbnail/icon
  ImVec2 buttonSize(m_ThumbnailSize, m_ThumbnailSize);

  // Get appropriate icon or thumbnail
  Ref<Texture2D> icon = GetThumbnail(asset);

  if (icon) {
    ImGui::ImageButton("##thumb", (ImTextureID)(intptr_t)icon->GetID(),
                       buttonSize, ImVec2(0, 1), ImVec2(1, 0));
  } else {
    // Fallback: colored button with type indicator
    ImVec4 color;
    const char *typeChar;

    if (asset.isDirectory) {
      color = ImVec4(0.9f, 0.7f, 0.2f, 1.0f);
      typeChar = "D";
    } else {
      switch (asset.type) {
      case AssetType::Texture:
        color = ImVec4(0.2f, 0.7f, 0.9f, 1.0f);
        typeChar = "T";
        break;
      case AssetType::Material:
        color = ImVec4(0.9f, 0.3f, 0.5f, 1.0f);
        typeChar = "M";
        break;
      case AssetType::Mesh:
        color = ImVec4(0.3f, 0.9f, 0.5f, 1.0f);
        typeChar = "3D";
        break;
      case AssetType::Scene:
        color = ImVec4(0.5f, 0.3f, 0.9f, 1.0f);
        typeChar = "S";
        break;
      case AssetType::Audio:
        color = ImVec4(0.9f, 0.5f, 0.2f, 1.0f);
        typeChar = "A";
        break;
      default:
        color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        typeChar = "?";
        break;
      }
    }

    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::Button(typeChar, buttonSize);
    ImGui::PopStyleColor();
  }

  ImGui::PopStyleColor();

  // Handle double-click
  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
    if (asset.isDirectory) {
      NavigateTo(asset.absolutePath);
    } else {
      // Open asset (e.g., material editor)
      GINI_INFO("Opening asset: ", asset.name);
      // TODO: Open appropriate editor based on asset type
    }
  }

  // Drag source
  if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
    const char *payloadType = PAYLOAD_ASSET;
    if (asset.type == AssetType::Texture)
      payloadType = PAYLOAD_TEXTURE;
    else if (asset.type == AssetType::Material)
      payloadType = PAYLOAD_MATERIAL;

    std::string pathStr = asset.absolutePath.string();
    ImGui::SetDragDropPayload(payloadType, pathStr.c_str(), pathStr.size() + 1);
    ImGui::Text("%s", asset.name.c_str());
    ImGui::EndDragDropSource();
  }

  // Tooltip
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::Text("%s", asset.name.c_str());
    if (!asset.isDirectory) {
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Type: %s",
                         AssetRegistry::AssetTypeToString(asset.type));
    }
    ImGui::EndTooltip();
  }

  // Draw name (truncated if too long)
  std::string displayName = asset.name;
  float textWidth = ImGui::CalcTextSize(displayName.c_str()).x;
  if (textWidth > m_ThumbnailSize) {
    // Truncate with ellipsis
    while (textWidth > m_ThumbnailSize - 20 && displayName.length() > 3) {
      displayName = displayName.substr(0, displayName.length() - 1);
      textWidth = ImGui::CalcTextSize((displayName + "...").c_str()).x;
    }
    displayName += "...";
  }

  float textX =
      (m_ThumbnailSize - ImGui::CalcTextSize(displayName.c_str()).x) * 0.5f;
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
  ImGui::TextWrapped("%s", displayName.c_str());

  ImGui::PopID();
}

void AssetBrowserPanel::DrawContextMenu() {
  if (ImGui::BeginPopupContextWindow("AssetBrowserContext",
                                     ImGuiPopupFlags_MouseButtonRight |
                                         ImGuiPopupFlags_NoOpenOverItems)) {
    if (ImGui::MenuItem("New Folder")) {
      // TODO: Create new folder
    }
    if (ImGui::MenuItem("New Material")) {
      // TODO: Create new material
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Refresh")) {
      Refresh();
    }
    if (ImGui::MenuItem("Show in Explorer")) {
      // TODO: Open file explorer
    }
    ImGui::EndPopup();
  }
}

void AssetBrowserPanel::NavigateTo(const std::filesystem::path &directory) {
  if (directory == m_CurrentDirectory)
    return;

  m_BackHistory.push_back(m_CurrentDirectory);
  m_ForwardHistory.clear();
  m_CurrentDirectory = directory;
}

void AssetBrowserPanel::NavigateBack() {
  if (m_BackHistory.empty())
    return;

  m_ForwardHistory.push_back(m_CurrentDirectory);
  m_CurrentDirectory = m_BackHistory.back();
  m_BackHistory.pop_back();
}

void AssetBrowserPanel::NavigateForward() {
  if (m_ForwardHistory.empty())
    return;

  m_BackHistory.push_back(m_CurrentDirectory);
  m_CurrentDirectory = m_ForwardHistory.back();
  m_ForwardHistory.pop_back();
}

Ref<Texture2D> AssetBrowserPanel::GetIconForAssetType(AssetType type) {
  // TODO: Load actual icons
  return nullptr;
}

Ref<Texture2D> AssetBrowserPanel::GetThumbnail(const AssetMetadata &asset) {
  if (asset.isDirectory) {
    return m_FolderIcon;
  }

  // For textures, try to load the actual texture as thumbnail
  if (asset.type == AssetType::Texture) {
    auto it = m_ThumbnailCache.find(asset.absolutePath.string());
    if (it != m_ThumbnailCache.end()) {
      return it->second;
    }

    // Load texture (could be async in a real implementation)
    try {
      Ref<Texture2D> tex = Texture2D::Create(asset.absolutePath.string());
      if (tex) {
        m_ThumbnailCache[asset.absolutePath.string()] = tex;
        return tex;
      }
    } catch (...) {
      // Failed to load, use default icon
    }
  }

  // For materials (.gmat), try to load the albedo texture as thumbnail
  if (asset.type == AssetType::Material) {
    auto it = m_ThumbnailCache.find(asset.absolutePath.string());
    if (it != m_ThumbnailCache.end()) {
      return it->second;
    }

    try {
      YAML::Node data = YAML::LoadFile(asset.absolutePath.string());
      if (data["Material"]) {
        auto material = data["Material"];
        if (material["AlbedoTexture"]) {
          std::string texturePath = material["AlbedoTexture"].as<std::string>();
          if (std::filesystem::exists(texturePath)) {
            Ref<Texture2D> tex = Texture2D::Create(texturePath);
            if (tex) {
              m_ThumbnailCache[asset.absolutePath.string()] = tex;
              return tex;
            }
          }
        }
      }
    } catch (...) {
      // Failed to parse material, use default icon
    }
  }

  return GetIconForAssetType(asset.type);
}

} // namespace Gini
