#pragma once

#include "Gini.h"
#include "Scene/Scene.h"
#include "Renderer/Framebuffer.h"
#include "Renderer/Camera3D.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/PropertiesPanel.h"
#include "Panels/StatsPanel.h"
#include "Panels/ConsolePanel.h"

namespace Gini {

class EditorApp : public Application {
public:
    EditorApp();
    
    void OnInit() override;
    void OnShutdown() override;
    void OnUpdate(f32 deltaTime) override;
    void OnRender() override;
    void OnEvent(Event& event) override;
    
private:
    void SetupDockspace();
    void DrawMenuBar();
    void DrawViewport();
    void DrawToolbar();
    
    void NewScene();
    void OpenScene();
    void SaveScene();
    void SaveSceneAs();
    
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
};

} // namespace Gini
