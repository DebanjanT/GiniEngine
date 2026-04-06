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
        std::vector<FileDialogFilter> filters = {{"Gini Terrain", "gterrain"}};
        std::string filepath = FileDialog::OpenFile(filters);
        if (!filepath.empty()) {
          m_Scene->LoadTerrainFromFile(filepath);
        }
      }
    } else {
      ImGui::Text("No terrain linked to this scene");
      ImGui::Spacing();

      if (ImGui::Button("Link Terrain...")) {
        std::vector<FileDialogFilter> filters = {{"Gini Terrain", "gterrain"}};
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

  ImGui::Separator();

  // Atmospheric Sky Section
  if (ImGui::CollapsingHeader("Atmospheric Sky",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    bool skyEnabled = m_Scene->IsAtmosphericSkyEnabled();
    if (ImGui::Checkbox("Enable Atmospheric Sky", &skyEnabled)) {
      m_Scene->EnableAtmosphericSky(skyEnabled);
    }

    if (skyEnabled && m_Scene->HasAtmosphericSky()) {
      auto sky = m_Scene->GetAtmosphericSky();
      auto &sun = sky->GetSunSettings();
      auto &atmosphere = sky->GetAtmosphereSettings();
      auto &fog = sky->GetFogSettings();

      ImGui::Spacing();
      ImGui::Text("Sun Settings");

      // Time of day slider
      static float timeOfDay = 12.0f;
      if (ImGui::SliderFloat("Time of Day", &timeOfDay, 0.0f, 24.0f,
                             "%.1f h")) {
        sky->SetSunFromTimeOfDay(timeOfDay);
      }

      ImGui::DragFloat3("Sun Direction", &sun.direction.x, 0.01f, -1.0f, 1.0f);
      ImGui::ColorEdit3("Sun Color", &sun.color.x);
      ImGui::DragFloat("Sun Intensity", &sun.intensity, 0.1f, 0.0f, 10.0f);
      ImGui::DragFloat("Sun Disk Size", &sun.diskSize, 0.001f, 0.001f, 0.1f);

      ImGui::Spacing();
      ImGui::Text("Atmosphere Settings");
      ImGui::DragFloat("Rayleigh Scale", &atmosphere.rayleighScale, 100.0f,
                       1000.0f, 20000.0f);
      ImGui::DragFloat("Mie Scale", &atmosphere.mieScale, 10.0f, 100.0f,
                       5000.0f);
      ImGui::DragFloat("Mie G (Anisotropy)", &atmosphere.mieG, 0.01f, -0.99f,
                       0.99f);

      ImGui::Spacing();
      ImGui::Text("Cloud Settings");
      auto &clouds = sky->GetCloudSettings();
      ImGui::Checkbox("Enable Clouds", &clouds.enabled);
      if (clouds.enabled) {
        ImGui::DragFloat("Coverage", &clouds.coverage, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Density", &clouds.density, 0.1f, 0.1f, 5.0f);
        ImGui::DragFloat("Height (m)", &clouds.height, 100.0f, 500.0f,
                         10000.0f);
        ImGui::DragFloat("Thickness (m)", &clouds.thickness, 100.0f, 500.0f,
                         5000.0f);
        ImGui::DragFloat("Wind Speed", &clouds.speed, 0.1f, 0.0f, 5.0f);
      }

      ImGui::Spacing();
      ImGui::Text("Fog Settings");
      ImGui::Checkbox("Enable Fog", &fog.enabled);
      if (fog.enabled) {
        ImGui::ColorEdit3("Fog Color", &fog.color.x);
        ImGui::DragFloat("Fog Density", &fog.density, 0.00001f, 0.0f, 0.01f,
                         "%.6f");
        ImGui::DragFloat("Height Falloff", &fog.heightFalloff, 0.0001f, 0.0f,
                         0.1f, "%.4f");
        ImGui::DragFloat("Start Distance", &fog.startDistance, 1.0f, 0.0f,
                         100.0f);
        ImGui::DragFloat("Max Opacity", &fog.maxOpacity, 0.01f, 0.0f, 1.0f);
      }
    }
  }

  ImGui::End();
}

} // namespace Gini
