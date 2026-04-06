#include "ScenePropertiesPanel.h"
#include "Core/Logger.h"
#include "Utils/FileDialog.h"
#include <imgui.h>

namespace Gini {

ScenePropertiesPanel::ScenePropertiesPanel() : EditorPanel("Scene Properties") {
  m_Visible = true;
}

void ScenePropertiesPanel::OnImGuiRender() {
  if (!m_Visible)
    return;

  ImGui::Begin("Scene Properties", &m_Visible);

  if (!m_Scene) {
    ImGui::Text("No scene loaded");
    ImGui::End();
    return;
  }

  // Scene Name
  ImGui::Text("Scene: %s", m_Scene->GetName().c_str());
  ImGui::Separator();

  // Scene Statistics
  if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto stats = m_Scene->GetStats();
    ImGui::Text("Entities: %u", stats.entityCount);
    ImGui::Text("Meshes: %u", stats.meshCount);
    ImGui::Text("Lights: %u", stats.lightCount);
    ImGui::Text("Cameras: %u", stats.cameraCount);
  }

  ImGui::Separator();

  // Terrain Section
  if (ImGui::CollapsingHeader("Terrain", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (m_Scene->HasTerrain()) {
      auto terrain = m_Scene->GetTerrain();
      ImGui::Text("Terrain Loaded");
      ImGui::Text("Size: %u x %u", terrain->GetWidth(), terrain->GetHeight());
      ImGui::Text("Scale: %.2f", terrain->GetScale());
      
      if (!m_Scene->GetTerrainPath().empty()) {
        ImGui::Text("Path: %s", m_Scene->GetTerrainPath().c_str());
      }
      
      ImGui::Spacing();
      
      if (ImGui::Button("Unlink Terrain")) {
        m_Scene->SetTerrain(nullptr);
        m_Scene->SetTerrainPath("");
        GINI_INFO("Terrain unlinked from scene");
      }
      
      ImGui::SameLine();
      
      if (ImGui::Button("Replace Terrain...")) {
        std::vector<FileDialogFilter> filters = {
          {"Gini Terrain", "gterrain"}
        };
        std::string filepath = FileDialog::OpenFile(filters);
        if (!filepath.empty()) {
          m_Scene->LoadTerrainFromFile(filepath);
        }
      }
    } else {
      ImGui::Text("No terrain linked to this scene");
      ImGui::Spacing();
      
      if (ImGui::Button("Link Terrain...")) {
        std::vector<FileDialogFilter> filters = {
          {"Gini Terrain", "gterrain"}
        };
        std::string filepath = FileDialog::OpenFile(filters);
        if (!filepath.empty()) {
          m_Scene->LoadTerrainFromFile(filepath);
        }
      }
      
      ImGui::SameLine();
      
      if (ImGui::Button("Create New Terrain")) {
        // Create a default terrain
        auto terrain = Terrain::Create(256, 256, 1.0f);
        m_Scene->SetTerrain(terrain);
        GINI_INFO("Created new terrain for scene");
      }
    }
  }

  ImGui::End();
}

} // namespace Gini
