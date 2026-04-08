#include "AssetBrowserPanel.h"
#include "Core/Logger.h"
#include "Project/Project.h"
#include <algorithm>
#include <cstdio>
#include <imgui.h>

namespace Gini {

AssetBrowserPanel::AssetBrowserPanel() : EditorPanel("Asset Browser") {}

void AssetBrowserPanel::SetRootPath(const std::filesystem::path &path) {
  GINI_INFO("AssetBrowserPanel::SetRootPath start: ", path.string());
  m_RootPath = path;
  m_CurrentDirectory = path;
  m_BackHistory.clear();
  m_ForwardHistory.clear();
  m_ThumbnailCache.clear();

  AssetRegistry::Get().SetRootPath(path);
  AssetRegistry::Get().ScanDirectory(path);

  GINI_INFO("AssetBrowserPanel::SetRootPath end");
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
  ImVec4 navBtnCol(0.0f, 0.0f, 0.0f, 0.0f);
  ImVec4 navBtnHov(0.24f, 0.24f, 0.27f, 1.0f);
  ImGui::PushStyleColor(ImGuiCol_Button, navBtnCol);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, navBtnHov);

  bool canBack = !m_BackHistory.empty();
  bool canFwd = !m_ForwardHistory.empty();
  bool canUp = m_CurrentDirectory != m_RootPath && m_CurrentDirectory.has_parent_path();

  if (!canBack) ImGui::BeginDisabled();
  if (ImGui::SmallButton("<##back")) NavigateBack();
  if (!canBack) ImGui::EndDisabled();
  ImGui::SameLine(0, 2);
  if (!canFwd) ImGui::BeginDisabled();
  if (ImGui::SmallButton(">##fwd")) NavigateForward();
  if (!canFwd) ImGui::EndDisabled();
  ImGui::SameLine(0, 2);
  if (!canUp) ImGui::BeginDisabled();
  if (ImGui::SmallButton("^##up")) NavigateTo(m_CurrentDirectory.parent_path());
  if (!canUp) ImGui::EndDisabled();
  ImGui::SameLine(0, 8);
  if (ImGui::SmallButton("Refresh")) Refresh();
  ImGui::PopStyleColor(2);

  ImGui::SameLine(0, 8);
  ImGui::Checkbox("Tree", &m_ShowDirectoryTree);

  // Breadcrumb path
  ImGui::SameLine(0, 12);
  {
    std::filesystem::path rel;
    std::error_code ec;
    rel = std::filesystem::relative(m_CurrentDirectory, m_RootPath, ec);
    if (ec) rel.clear();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f,0.24f,0.27f,1));

    // Root "Assets"
    if (ImGui::SmallButton("Assets")) NavigateTo(m_RootPath);

    if (!rel.empty() && rel != ".") {
      std::filesystem::path accumulated = m_RootPath;
      for (auto it = rel.begin(); it != rel.end(); ++it) {
        accumulated /= *it;
        ImGui::SameLine(0, 2);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.45f, 0.48f, 1.0f));
        ImGui::Text("/");
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 2);
        std::string part = it->string();
        std::string btnId = part + "##bc" + accumulated.string();
        if (ImGui::SmallButton(btnId.c_str())) NavigateTo(accumulated);
      }
    }
    ImGui::PopStyleColor(2);
  }

  // Right side: search + size slider
  float rightWidth = 230.0f;
  ImGui::SameLine(ImGui::GetWindowWidth() - rightWidth);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
  ImGui::SetNextItemWidth(140);
  char searchBuffer[256];
  std::snprintf(searchBuffer, sizeof(searchBuffer), "%s", m_SearchFilter.c_str());
  if (ImGui::InputTextWithHint("##Search", "Search...", searchBuffer, sizeof(searchBuffer))) {
    m_SearchFilter = searchBuffer;
  }
  ImGui::PopStyleVar();

  ImGui::SameLine(0, 8);
  ImGui::SetNextItemWidth(70);
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
  std::error_code ec;
  if (!std::filesystem::exists(directory, ec) || ec)
    return;

  // Collect directory entries first to avoid filesystem iteration during ImGui rendering
  struct DirEntry {
    std::filesystem::path path;
    std::string name;
    bool hasSubdirs = false;
  };
  std::vector<DirEntry> entries;

  std::error_code dirError;
  auto dirIt = std::filesystem::directory_iterator(
      directory, std::filesystem::directory_options::skip_permission_denied,
      dirError);
  if (dirError)
    return;

  for (const auto &entry : dirIt) {
    std::error_code entryEc;
    if (entry.is_symlink(entryEc) || entryEc)
      continue;
    if (!entry.is_directory(entryEc) || entryEc)
      continue;

    std::string filename = entry.path().filename().string();
    if (filename.empty() || filename[0] == '.')
      continue;

    DirEntry de;
    de.path = entry.path();
    de.name = filename;

    std::error_code subDirError;
    auto subIt = std::filesystem::directory_iterator(
        entry.path(),
        std::filesystem::directory_options::skip_permission_denied,
        subDirError);
    if (!subDirError) {
      for (const auto &subEntry : subIt) {
        if (subDirError)
          break;
        std::error_code subEc;
        if (subEntry.is_symlink(subEc) || subEc)
          continue;
        if (!subEntry.is_directory(subEc) || subEc)
          continue;
        std::string subName = subEntry.path().filename().string();
        if (!subName.empty() && subName[0] != '.') {
          de.hasSubdirs = true;
          break;
        }
      }
    }

    entries.push_back(std::move(de));
  }

  std::sort(entries.begin(), entries.end(),
            [](const DirEntry &a, const DirEntry &b) {
              return a.name < b.name;
            });

  for (const auto &de : entries) {
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (!de.hasSubdirs) {
      flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (de.path == m_CurrentDirectory) {
      flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool opened = ImGui::TreeNodeEx(de.name.c_str(), flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
      NavigateTo(de.path);
    }

    if (opened) {
      DrawDirectoryTreeNode(de.path);
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
  std::filesystem::path relativePath;
  std::error_code relError;
  relativePath = std::filesystem::relative(m_CurrentDirectory, m_RootPath, relError);
  if (relError) {
    relativePath.clear();
  }
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
    ImGui::PopStyleColor(); // Pop the transparent button background

    // Handle double-click
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
      if (asset.isDirectory) {
        NavigateTo(asset.absolutePath);
      } else {
        GINI_INFO("Opening asset: ", asset.name);
      }
    }

    // Drag source
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
      const char *payloadType = PAYLOAD_ASSET;
      if (asset.type == AssetType::Texture)
        payloadType = PAYLOAD_TEXTURE;
      else if (asset.type == AssetType::Material)
        payloadType = PAYLOAD_MATERIAL;
      else if (asset.type == AssetType::Mesh)
        payloadType = PAYLOAD_MESH;

      std::string pathStr = asset.absolutePath.string();
      ImGui::SetDragDropPayload(payloadType, pathStr.c_str(),
                                pathStr.size() + 1);
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
  } else {
    // Fallback: colored button with type indicator
    ImVec4 color;
    const char *typeChar;

    if (asset.isDirectory) {
      // Yellow for directory type
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                            ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
      ImGui::Button("D", buttonSize);
      ImGui::PopStyleColor(3); // Pop directory colors + transparent background
    } else {
      switch (asset.type) {
      case AssetType::Texture:
        // Green for texture type
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
        ImGui::Button("T", buttonSize);
        ImGui::PopStyleColor(3); // Pop texture colors + transparent background
        break;
      case AssetType::Material:
        // Blue for model type (using primary blue)
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.39f, 0.68f, 1.00f, 1.0f));
        ImGui::Button("M", buttonSize);
        ImGui::PopStyleColor(3); // Pop material colors + transparent background
        break;
      case AssetType::Mesh:
        // Blue for model type (using primary blue)
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.39f, 0.68f, 1.00f, 1.0f));
        ImGui::Button("3D", buttonSize);
        ImGui::PopStyleColor(3); // Pop mesh colors + transparent background
        break;
      case AssetType::Scene:
        // Yellow for scene type
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.6f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.8f, 0.7f, 0.3f, 1.0f));
        ImGui::Button("S", buttonSize);
        ImGui::PopStyleColor(3); // Pop scene colors + transparent background
        break;
      case AssetType::Audio:
        // Purple for audio type
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.7f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.7f, 0.4f, 0.8f, 1.0f));
        ImGui::Button("A", buttonSize);
        ImGui::PopStyleColor(3); // Pop audio colors + transparent background
        break;
      default:
        // Gray for unknown type (using secondary gradient)
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImVec4(0.25f, 0.28f, 0.32f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.35f, 0.38f, 0.42f, 1.0f));
        ImGui::Button("?", buttonSize);
        ImGui::PopStyleColor(3); // Pop default colors + transparent background
        break;
      }
    }

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
      else if (asset.type == AssetType::Mesh)
        payloadType = PAYLOAD_MESH;

      std::string pathStr = asset.absolutePath.string();
      ImGui::SetDragDropPayload(payloadType, pathStr.c_str(),
                                pathStr.size() + 1);
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

  }

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

  // Avoid loading full textures during browsing to prevent memory spikes on
  // large projects. Keep browser lightweight and use type icons/fallback tiles.
  if (asset.type == AssetType::Texture || asset.type == AssetType::Material) {
    return GetIconForAssetType(asset.type);
  }

  return GetIconForAssetType(asset.type);
}

} // namespace Gini
