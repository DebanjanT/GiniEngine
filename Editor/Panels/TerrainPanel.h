#pragma once

#include "EditorPanel.h"
#include "Terrain/Terrain.h"

namespace Gini {

struct TerrainBrush {
  f32 radius = 5.0f;
  f32 strength = 0.5f;
  f32 falloff = 0.5f;
};

class TerrainPanel : public EditorPanel {
public:
  TerrainPanel() : EditorPanel("Terrain Editor") {}
  ~TerrainPanel() = default;

  void OnImGuiRender() override;

  void SetTerrain(Ref<Terrain> terrain) { m_Terrain = terrain; }
  Ref<Terrain> GetTerrain() const { return m_Terrain; }

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
