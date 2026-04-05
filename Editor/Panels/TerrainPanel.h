#pragma once

#include "EditorPanel.h"
#include "Terrain/Terrain.h"

namespace Gini {

struct TerrainBrush {
  f32 radius = 5.0f;
  f32 strength = 0.5f;
  f32 falloff = 0.5f;
};

struct TerrainLayerUI {
  std::string name = "Layer";
  Vec3 color = Vec3(0.5f);
  std::string texturePath = "";
  f32 tiling = 10.0f;
};

class TerrainPanel : public EditorPanel {
public:
  TerrainPanel() : EditorPanel("Terrain Editor") {}
  ~TerrainPanel() = default;

  void OnImGuiRender() override;

  void SetTerrain(Ref<Terrain> terrain);
  Ref<Terrain> GetTerrain() const { return m_Terrain; }

  // Sync layer colors to terrain
  void SyncLayersToTerrain();

  // Painting state
  bool IsPainting() const { return m_IsPainting; }
  TerrainBrush &GetBrush() { return m_Brush; }

  enum class PaintMode {
    None,
    RaiseHeight,
    LowerHeight,
    Smooth,
    Flatten,
    PaintMaterial
  };

  PaintMode GetPaintMode() const { return m_PaintMode; }
  u32 GetSelectedMaterialLayer() const { return m_SelectedMaterialLayer; }

private:
  void DrawTerrainSettings();
  void DrawBrushSettings();
  void DrawMaterialLayers();
  void DrawGenerationSettings();

  Ref<Terrain> m_Terrain;
  TerrainBrush m_Brush;

  PaintMode m_PaintMode = PaintMode::None;
  bool m_IsPainting = false;
  u32 m_SelectedMaterialLayer = 0;

  // Layer UI data (4 layers max for splatmap)
  TerrainLayerUI m_LayerUI[4] = {{"Grass", Vec3(0.3f, 0.5f, 0.2f), "", 10.0f},
                                 {"Sand", Vec3(0.76f, 0.7f, 0.5f), "", 10.0f},
                                 {"Rock", Vec3(0.4f, 0.4f, 0.4f), "", 10.0f},
                                 {"Snow", Vec3(0.9f, 0.9f, 0.95f), "", 10.0f}};

  // Generation settings
  int m_GenWidth = 256;
  int m_GenHeight = 256;
  float m_GenWorldSize = 100.0f;
  float m_GenMaxHeight = 20.0f;
  float m_NoiseFrequency = 0.02f;
  float m_NoiseAmplitude = 50.0f;
  int m_NoiseOctaves = 4;
};

} // namespace Gini
