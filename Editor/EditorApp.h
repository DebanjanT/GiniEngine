#pragma once

#include "Gini.h"
#include "Panels/ConsolePanel.h"
#include "Panels/PropertiesPanel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/StatsPanel.h"
#include "Renderer/Camera3D.h"
#include "Renderer/Framebuffer.h"
#include "Scene/Scene.h"

namespace Gini {

class EditorApp : public Application {
public:
  EditorApp();

  void OnInit() override;
  void OnShutdown() override;
  void OnUpdate(f32 deltaTime) override;
  void OnRender() override;
  void OnEvent(Event &event) override;

private:
  void SetupDockspace();
  void DrawMenuBar();
  void DrawViewport();
  void DrawToolbar();

  void NewScene();
  void OpenScene();
  void SaveScene();
  void SaveSceneAs();

  // Mouse picking
  Entity PickEntity(const Vec2 &mousePos);
  bool RayIntersectsAABB(const Vec3 &rayOrigin, const Vec3 &rayDir,
                         const Vec3 &boxMin, const Vec3 &boxMax, float &t);

  // Scene
  Ref<Scene> m_ActiveScene;
  Ref<Scene> m_EditorScene;

  // Viewport
  Ref<Framebuffer> m_Framebuffer;
  Vec2 m_ViewportSize = Vec2(1280, 720);
  Vec2 m_ViewportBounds[2];
  bool m_ViewportFocused = false;
  bool m_ViewportHovered = false;

  // Editor Camera
  Scope<Camera3D> m_EditorCamera;
  Scope<OrbitCameraController> m_CameraController;

  // Panels
  SceneHierarchyPanel m_HierarchyPanel;
  PropertiesPanel m_PropertiesPanel;
  StatsPanel m_StatsPanel;
  ConsolePanel m_ConsolePanel;

  // Editor state
  Entity m_SelectedEntity = NullEntity;
  bool m_ShowDemoWindow = false;

  enum class SceneState { Edit, Play, Pause };
  SceneState m_SceneState = SceneState::Edit;

  // Gizmo
  enum class GizmoOperation { Translate = 0, Rotate, Scale };
  GizmoOperation m_GizmoOperation = GizmoOperation::Translate;
  bool m_GizmoUsingSnap = false;
  float m_SnapTranslate = 0.5f;
  float m_SnapRotate = 45.0f;
  float m_SnapScale = 0.5f;
};

} // namespace Gini
