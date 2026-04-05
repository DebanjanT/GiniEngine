#include "TerrainPanel.h"
#include "Core/Logger.h"
#include <cstring>
#include <imgui.h>

namespace Gini {

void TerrainPanel::SetTerrain(Ref<Terrain> terrain) {
  m_Terrain = terrain;
  SyncLayersToTerrain();
}

void TerrainPanel::SyncLayersToTerrain() {
  if (!m_Terrain)
    return;

  // Ensure terrain has 4 layers
  while (m_Terrain->GetLayerCount() < 4) {
    TerrainLayer layer;
    u32 idx = m_Terrain->GetLayerCount();
    layer.name = m_LayerUI[idx].name;
    layer.color = m_LayerUI[idx].color;
    layer.tiling = Vec2(m_LayerUI[idx].tiling);
    m_Terrain->AddLayer(layer);
  }

  // Update layer colors from UI
  for (u32 i = 0; i < 4 && i < m_Terrain->GetLayerCount(); i++) {
    auto &layer = m_Terrain->GetLayer(i);
    layer.name = m_LayerUI[i].name;
    layer.color = m_LayerUI[i].color;
    layer.tiling = Vec2(m_LayerUI[i].tiling);
  }
}

void TerrainPanel::OnImGuiRender() {
  ImGui::Begin("Terrain Editor");

  if (!m_Terrain) {
    ImGui::Text("No terrain loaded");
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Create New Terrain",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      DrawGenerationSettings();
    }

    ImGui::End();
    return;
  }

  // Terrain info
  ImGui::Text("Terrain: %dx%d", m_Terrain->GetWidth(), m_Terrain->GetHeight());
  ImGui::Text("Scale: %.1f", m_Terrain->GetScale());

  ImGui::Separator();

  if (ImGui::CollapsingHeader("Terrain Settings",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    DrawTerrainSettings();
  }

  if (ImGui::CollapsingHeader("Brush Settings",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    DrawBrushSettings();
  }

  if (ImGui::CollapsingHeader("Material Layers",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    DrawMaterialLayers();
  }

  if (ImGui::CollapsingHeader("Generation", ImGuiTreeNodeFlags_None)) {
    DrawGenerationSettings();
  }

  ImGui::End();
}

void TerrainPanel::DrawTerrainSettings() {
  float heightScale = m_Terrain->GetHeightScale();
  if (ImGui::DragFloat("Height Scale", &heightScale, 0.1f, 0.1f, 100.0f)) {
    m_Terrain->SetHeightScale(heightScale);
  }

  ImGui::Separator();
  ImGui::Text("Paint Mode:");

  bool isRaise = m_PaintMode == PaintMode::RaiseHeight;
  bool isLower = m_PaintMode == PaintMode::LowerHeight;
  bool isSmooth = m_PaintMode == PaintMode::Smooth;
  bool isFlatten = m_PaintMode == PaintMode::Flatten;
  bool isPaintMat = m_PaintMode == PaintMode::PaintMaterial;

  if (ImGui::RadioButton("None", m_PaintMode == PaintMode::None)) {
    m_PaintMode = PaintMode::None;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Raise", isRaise)) {
    m_PaintMode = PaintMode::RaiseHeight;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Lower", isLower)) {
    m_PaintMode = PaintMode::LowerHeight;
  }

  if (ImGui::RadioButton("Smooth", isSmooth)) {
    m_PaintMode = PaintMode::Smooth;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Flatten", isFlatten)) {
    m_PaintMode = PaintMode::Flatten;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Material", isPaintMat)) {
    m_PaintMode = PaintMode::PaintMaterial;
  }

  // Keyboard shortcuts hint
  ImGui::TextDisabled(
      "Shortcuts: 1-Raise, 2-Lower, 3-Smooth, 4-Flatten, 5-Material");
}

void TerrainPanel::DrawBrushSettings() {
  ImGui::DragFloat("Radius", &m_Brush.radius, 0.5f, 0.5f, 50.0f);
  ImGui::DragFloat("Strength", &m_Brush.strength, 0.01f, 0.01f, 1.0f);
  ImGui::DragFloat("Falloff", &m_Brush.falloff, 0.1f, 0.0f, 1.0f);

  // Visual brush preview
  ImGui::Separator();
  ImGui::Text("Brush Preview:");

  ImVec2 canvasSize(100, 100);
  ImVec2 canvasPos = ImGui::GetCursorScreenPos();
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  // Background
  drawList->AddRectFilled(
      canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
      IM_COL32(30, 30, 30, 255));

  // Brush circle with falloff
  ImVec2 center(canvasPos.x + canvasSize.x * 0.5f,
                canvasPos.y + canvasSize.y * 0.5f);
  float maxRadius = canvasSize.x * 0.4f;
  float innerRadius = maxRadius * (1.0f - m_Brush.falloff);

  // Outer circle (falloff zone)
  drawList->AddCircleFilled(center, maxRadius, IM_COL32(100, 150, 200, 100));
  // Inner circle (full strength)
  drawList->AddCircleFilled(center, innerRadius, IM_COL32(100, 150, 200, 200));
  // Border
  drawList->AddCircle(center, maxRadius, IM_COL32(150, 200, 255, 255), 32,
                      2.0f);

  ImGui::Dummy(canvasSize);
}

void TerrainPanel::DrawMaterialLayers() {
  ImGui::Text("Material Layers (4 max for splatmap):");
  ImGui::Separator();

  // Display all 4 layers
  for (u32 i = 0; i < 4; i++) {
    ImGui::PushID(i);

    bool selected = (m_SelectedMaterialLayer == i);

    // Layer header with color preview
    ImVec4 col(m_LayerUI[i].color.x, m_LayerUI[i].color.y, m_LayerUI[i].color.z,
               1.0f);
    ImGui::ColorButton("##color", col, ImGuiColorEditFlags_NoTooltip,
                       ImVec2(20, 20));
    ImGui::SameLine();

    char label[64];
    snprintf(label, sizeof(label), "Layer %d: %s", i,
             m_LayerUI[i].name.c_str());
    if (ImGui::Selectable(label, selected)) {
      m_SelectedMaterialLayer = i;
    }

    ImGui::PopID();
  }

  ImGui::Separator();

  // Edit selected layer
  if (m_SelectedMaterialLayer < 4) {
    TerrainLayerUI &layerUI = m_LayerUI[m_SelectedMaterialLayer];

    ImGui::Text("Edit Layer %d:", m_SelectedMaterialLayer);

    // Name input
    char nameBuf[64];
    strncpy(nameBuf, layerUI.name.c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
      layerUI.name = nameBuf;
    }

    // Color picker
    float color[3] = {layerUI.color.x, layerUI.color.y, layerUI.color.z};
    if (ImGui::ColorEdit3("Color", color)) {
      layerUI.color = Vec3(color[0], color[1], color[2]);
      SyncLayersToTerrain();
    }

    // Tiling
    if (ImGui::DragFloat("Tiling", &layerUI.tiling, 0.5f, 1.0f, 100.0f)) {
      SyncLayersToTerrain();
    }

    // Texture path (display only for now)
    ImGui::Text("Texture: %s", layerUI.texturePath.empty()
                                   ? "(none - using color)"
                                   : layerUI.texturePath.c_str());

    if (ImGui::Button("Load Texture...", ImVec2(-1, 0))) {
      // TODO: File dialog for texture
      GINI_INFO("Load texture dialog for layer {}", m_SelectedMaterialLayer);
    }

    if (!layerUI.texturePath.empty()) {
      if (ImGui::Button("Clear Texture", ImVec2(-1, 0))) {
        layerUI.texturePath = "";
      }
    }
  }

  ImGui::Separator();
  ImGui::TextDisabled("Click and drag on terrain to paint selected layer");
  ImGui::TextDisabled("Hold Shift + Click to paint height");
}

void TerrainPanel::DrawGenerationSettings() {
  ImGui::DragInt("Width", &m_GenWidth, 1, 64, 1024);
  ImGui::DragInt("Height", &m_GenHeight, 1, 64, 1024);
  ImGui::DragFloat("World Size", &m_GenWorldSize, 1.0f, 10.0f, 1000.0f);
  ImGui::DragFloat("Max Height", &m_GenMaxHeight, 0.5f, 1.0f, 200.0f);

  ImGui::Separator();
  ImGui::Text("Noise Settings:");
  ImGui::DragFloat("Frequency", &m_NoiseFrequency, 0.001f, 0.001f, 0.1f,
                   "%.4f");
  ImGui::DragFloat("Amplitude", &m_NoiseAmplitude, 1.0f, 1.0f, 200.0f);
  ImGui::DragInt("Octaves", &m_NoiseOctaves, 1, 1, 8);

  ImGui::Separator();

  if (ImGui::Button("Generate Flat Terrain", ImVec2(-1, 0))) {
    m_Terrain = Terrain::Create(m_GenWidth, m_GenHeight, m_GenWorldSize);
    m_Terrain->GenerateFlat();
    GINI_INFO("Generated flat terrain {}x{}", m_GenWidth, m_GenHeight);
  }

  if (ImGui::Button("Generate Noise Terrain", ImVec2(-1, 0))) {
    m_Terrain = Terrain::Create(m_GenWidth, m_GenHeight, m_GenWorldSize);
    m_Terrain->GenerateFromNoise(m_NoiseFrequency, m_NoiseAmplitude,
                                 m_NoiseOctaves);
    GINI_INFO("Generated noise terrain {}x{}", m_GenWidth, m_GenHeight);
  }

  ImGui::Separator();

  if (ImGui::Button("Load Heightmap...", ImVec2(-1, 0))) {
    // TODO: File dialog for heightmap
    ImGui::OpenPopup("LoadHeightmapPopup");
  }

  if (ImGui::BeginPopup("LoadHeightmapPopup")) {
    ImGui::Text("Heightmap loading requires file dialog");
    ImGui::Text("(Coming soon)");
    if (ImGui::Button("Close")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

} // namespace Gini
