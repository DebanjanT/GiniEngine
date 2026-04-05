#pragma once

#include "EditorPanel.h"
#include "Renderer/Camera3D.h"
#include "Renderer/Framebuffer.h"
#include "Renderer/Texture.h"
#include "Terrain/Terrain.h"

namespace Gini {

struct TerrainMaterial {
  std::string name = "Material";
  std::string albedoPath = "";
  std::string normalPath = "";
  Ref<Texture2D> albedoTexture;
  Ref<Texture2D> normalTexture;
  Vec3 fallbackColor = Vec3(0.5f);
  f32 tiling = 10.0f;
  f32 metallic = 0.0f;
  f32 roughness = 0.8f;
};

class TerrainEditorWindow {
public:
  TerrainEditorWindow();
  ~TerrainEditorWindow() = default;

  void Open();
  void Close();
  bool IsOpen() const { return m_IsOpen; }

  void OnUpdate(f32 deltaTime);
  void OnImGuiRender();

  Ref<Terrain> GetTerrain() const { return m_Terrain; }

private:
  void DrawToolbar();
  void DrawViewport();
  void DrawLayersPanel();
  void DrawBrushPanel();
  void DrawSettingsPanel();

  void HandlePainting(f32 deltaTime);
  Vec3 ScreenToWorldRay(const Vec2 &screenPos);
  void LoadTextureForLayer(u32 layerIndex);
  void SaveTerrain();
  void ExportTerrain();
  void CreateNewTerrain();

  bool m_IsOpen = false;

  // Terrain
  Ref<Terrain> m_Terrain;

  // Viewport
  Ref<Framebuffer> m_Framebuffer;
  Vec2 m_ViewportSize = Vec2(800, 600);
  Vec2 m_ViewportBounds[2];
  bool m_ViewportHovered = false;
  bool m_ViewportFocused = false;

  // Camera
  Scope<Camera3D> m_Camera;
  Scope<OrbitCameraController> m_CameraController;

  // Materials (4 layers for splatmap RGBA)
  TerrainMaterial m_Materials[4];
  u32 m_SelectedLayer = 0;

  // Brush settings
  enum class BrushMode { Sculpt, Paint };
  enum class SculptMode { Raise, Lower, Smooth, Flatten };

  BrushMode m_BrushMode = BrushMode::Sculpt;
  SculptMode m_SculptMode = SculptMode::Raise;

  f32 m_BrushRadius = 5.0f;
  f32 m_BrushStrength = 0.5f;
  f32 m_BrushFalloff = 0.5f;

  // Painting state
  Vec3 m_HitPoint = Vec3(0.0f);
  bool m_IsHit = false;

  // Terrain settings
  int m_TerrainWidth = 256;
  int m_TerrainHeight = 256;
  float m_TerrainScale = 1.0f;
  float m_TerrainMaxHeight = 50.0f;

  // File paths
  std::string m_CurrentFilePath = "";

  // Layout reset flag
  bool m_NeedsLayoutReset = true;
};

} // namespace Gini
