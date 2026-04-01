#include "EditorApp.h"
#include "Renderer/Renderer3D.h"
#include "UI/ImGuiLayer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_internal.h>

namespace Gini {

EditorApp::EditorApp()
    : Application([]() {
        EngineConfig config;
        config.windowTitle = "Gini Editor";
        config.windowWidth = 1600;
        config.windowHeight = 900;
        config.vsync = true;
        return config;
      }()) {}

void EditorApp::OnInit() {
  GINI_INFO("Gini Editor initialized!");

  // Initialize ImGui
  ImGuiLayer::Init();

  // Initialize 3D Renderer
  Renderer3D::Init();

  // Create framebuffer for viewport
  FramebufferSpec fbSpec;
  fbSpec.width = 1280;
  fbSpec.height = 720;
  fbSpec.samples = 1;
  m_Framebuffer = Framebuffer::Create(fbSpec);

  // Setup editor camera
  m_EditorCamera = CreateScope<Camera3D>(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
  m_EditorCamera->SetPosition(Vec3(0.0f, 5.0f, 10.0f));
  m_EditorCamera->LookAt(Vec3(0.0f, 0.0f, 0.0f));

  m_CameraController = CreateScope<OrbitCameraController>(m_EditorCamera.get());
  m_CameraController->SetDistance(15.0f);

  // Create default scene
  NewScene();

  // Setup panels
  m_HierarchyPanel.SetScene(m_ActiveScene);
  m_HierarchyPanel.SetSelectionCallback([this](Entity entity) {
    m_SelectedEntity = entity;
    m_PropertiesPanel.SetSelectedEntity(entity);
  });

  m_PropertiesPanel.SetScene(m_ActiveScene);
}

void EditorApp::OnShutdown() {
  Renderer3D::Shutdown();
  ImGuiLayer::Shutdown();
  GINI_INFO("Gini Editor shutdown!");
}

void EditorApp::OnUpdate(f32 deltaTime) {
  // Update stats panel
  m_StatsPanel.OnUpdate(deltaTime);
  if (m_ActiveScene) {
    m_StatsPanel.SetEntityCount(m_ActiveScene->GetEntityCount());
  }

  // Handle viewport input
  if (m_ViewportFocused) {
    auto &input = Input::Get();

    if (input.IsMouseButtonDown(MouseButton::Right)) {
      Vec2 delta = input.GetMouseDelta();
      m_CameraController->OnMouseMove(delta.x, delta.y, true, false);
    }

    if (input.IsMouseButtonDown(MouseButton::Middle)) {
      Vec2 delta = input.GetMouseDelta();
      m_CameraController->OnMouseMove(delta.x, delta.y, false, true);
    }
  }

  m_CameraController->OnUpdate(deltaTime);

  // Update scene
  if (m_SceneState == SceneState::Play && m_ActiveScene) {
    m_ActiveScene->OnUpdate(deltaTime);
  }
}

void EditorApp::OnRender() {
  // Render to framebuffer
  m_Framebuffer->Bind();
  Renderer3D::SetViewport(0, 0, static_cast<u32>(m_ViewportSize.x),
                          static_cast<u32>(m_ViewportSize.y));
  Renderer3D::Clear();
  Renderer3D::SetClearColor(Color(0.1f, 0.1f, 0.15f));

  // Render scene
  Renderer3D::BeginScene(*m_EditorCamera);

  if (m_ActiveScene) {
    // Draw grid
    for (int i = -10; i <= 10; i++) {
      Renderer3D::DrawLine(Vec3(i, 0, -10), Vec3(i, 0, 10),
                           Color(0.3f, 0.3f, 0.3f));
      Renderer3D::DrawLine(Vec3(-10, 0, i), Vec3(10, 0, i),
                           Color(0.3f, 0.3f, 0.3f));
    }

    // Draw entities with transforms
    auto &world = m_ActiveScene->GetWorld();
    auto view = world.GetRegistry().view<TransformComponent>();
    for (auto entity : view) {
      auto &transform = view.get<TransformComponent>(entity);

      // Draw a small cube for each entity
      Mat4 modelMatrix = transform.GetTransform();

      // Highlight selected entity
      if (entity == m_SelectedEntity) {
        // Draw selection outline
      }
    }
  }

  Renderer3D::EndScene();
  m_Framebuffer->Unbind();

  // Render ImGui
  ImGuiLayer::Begin();

  SetupDockspace();
  DrawMenuBar();
  DrawToolbar();
  DrawViewport();

  // Draw panels
  m_HierarchyPanel.OnImGuiRender();
  m_PropertiesPanel.OnImGuiRender();
  m_StatsPanel.OnImGuiRender();
  m_ConsolePanel.OnImGuiRender();

  if (m_ShowDemoWindow) {
    ImGui::ShowDemoWindow(&m_ShowDemoWindow);
  }

  ImGuiLayer::End();
}

void EditorApp::OnEvent(Event &event) {
  ImGuiLayer::OnEvent(event);

  if (event.GetType() == EventType::WindowResize) {
    auto &e = static_cast<WindowResizeEvent &>(event);
    // Handle resize
  }

  if (event.GetType() == EventType::MouseScrolled && m_ViewportHovered) {
    auto &e = static_cast<MouseScrolledEvent &>(event);
    m_CameraController->OnScroll(e.GetYOffset());
  }

  if (event.GetType() == EventType::KeyPressed) {
    auto &e = static_cast<KeyPressedEvent &>(event);

    // Shortcuts
    bool ctrl = Input::Get().IsKeyDown(Key::LeftControl) ||
                Input::Get().IsKeyDown(Key::RightControl);

    if (ctrl) {
      i32 keyCode = e.GetKeyCode();
      if (keyCode == static_cast<i32>(Key::N))
        NewScene();
      else if (keyCode == static_cast<i32>(Key::O))
        OpenScene();
      else if (keyCode == static_cast<i32>(Key::S))
        SaveScene();
    }

    // Delete selected entity
    if (e.GetKeyCode() == static_cast<i32>(Key::Delete) &&
        m_SelectedEntity != NullEntity) {
      m_ActiveScene->DestroyEntity(m_SelectedEntity);
      m_SelectedEntity = NullEntity;
    }
  }
}

void EditorApp::SetupDockspace() {
  // Simple fullscreen window setup (docking requires ImGui docking branch)
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar;

  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  windowFlags |=
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  static bool open = true;
  ImGui::Begin("MainWindow", &open, windowFlags);
  ImGui::PopStyleVar(3);

  ImGui::End();
}

void EditorApp::DrawMenuBar() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Scene", "Ctrl+N"))
        NewScene();
      if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
        OpenScene();
      ImGui::Separator();
      if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
        SaveScene();
      if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
        SaveSceneAs();
      ImGui::Separator();
      if (ImGui::MenuItem("Exit")) {
        // Close application
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
      }
      if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Preferences")) {
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Scene Hierarchy", nullptr, &m_HierarchyPanel.m_Visible);
      ImGui::MenuItem("Properties", nullptr, &m_PropertiesPanel.m_Visible);
      ImGui::MenuItem("Stats", nullptr, &m_StatsPanel.m_Visible);
      ImGui::MenuItem("Console", nullptr, &m_ConsolePanel.m_Visible);
      ImGui::Separator();
      ImGui::MenuItem("ImGui Demo", nullptr, &m_ShowDemoWindow);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Entity")) {
      if (ImGui::MenuItem("Create Empty")) {
        if (m_ActiveScene)
          m_ActiveScene->CreateEntity("Empty Entity");
      }
      if (ImGui::MenuItem("Create Camera")) {
        if (m_ActiveScene)
          m_ActiveScene->CreateEntity("Camera");
      }
      if (ImGui::MenuItem("Create Light")) {
        if (m_ActiveScene)
          m_ActiveScene->CreateEntity("Light");
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("About Gini Engine")) {
      }
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}

void EditorApp::DrawToolbar() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));

  ImGui::Begin("##toolbar", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoScrollWithMouse);

  float size = ImGui::GetWindowHeight() - 4.0f;

  ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) -
                       (size * 0.5f));

  bool isPlaying = m_SceneState == SceneState::Play;

  if (ImGui::Button(isPlaying ? "Stop" : "Play", ImVec2(size * 2, size))) {
    if (m_SceneState == SceneState::Edit) {
      m_SceneState = SceneState::Play;
      if (m_ActiveScene)
        m_ActiveScene->OnStart();
    } else {
      m_SceneState = SceneState::Edit;
      if (m_ActiveScene)
        m_ActiveScene->OnStop();
    }
  }

  ImGui::End();
  ImGui::PopStyleVar(2);
}

void EditorApp::DrawViewport() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::Begin("Viewport");

  auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
  auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
  auto viewportOffset = ImGui::GetWindowPos();
  m_ViewportBounds[0] = {viewportMinRegion.x + viewportOffset.x,
                         viewportMinRegion.y + viewportOffset.y};
  m_ViewportBounds[1] = {viewportMaxRegion.x + viewportOffset.x,
                         viewportMaxRegion.y + viewportOffset.y};

  m_ViewportFocused = ImGui::IsWindowFocused();
  m_ViewportHovered = ImGui::IsWindowHovered();

  ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
  if (m_ViewportSize.x != viewportPanelSize.x ||
      m_ViewportSize.y != viewportPanelSize.y) {
    m_ViewportSize = {viewportPanelSize.x, viewportPanelSize.y};
    m_Framebuffer->Resize(static_cast<u32>(m_ViewportSize.x),
                          static_cast<u32>(m_ViewportSize.y));
    m_EditorCamera->SetAspectRatio(m_ViewportSize.x / m_ViewportSize.y);
  }

  u32 textureID = m_Framebuffer->GetColorAttachment();
  ImGui::Image((void *)(intptr_t)textureID,
               ImVec2(m_ViewportSize.x, m_ViewportSize.y), ImVec2(0, 1),
               ImVec2(1, 0));

  ImGui::End();
  ImGui::PopStyleVar();
}

void EditorApp::NewScene() {
  m_ActiveScene = CreateRef<Scene>("Untitled Scene");
  m_HierarchyPanel.SetScene(m_ActiveScene);
  m_PropertiesPanel.SetScene(m_ActiveScene);
  m_SelectedEntity = NullEntity;
  GINI_INFO("New scene created");
}

void EditorApp::OpenScene() {
  // TODO: File dialog
  GINI_INFO("Open scene dialog");
}

void EditorApp::SaveScene() {
  if (m_ActiveScene) {
    // TODO: File dialog if no filepath
    GINI_INFO("Save scene");
  }
}

void EditorApp::SaveSceneAs() {
  // TODO: File dialog
  GINI_INFO("Save scene as dialog");
}

} // namespace Gini

// Entry point
Gini::Application *Gini::CreateApplication() { return new Gini::EditorApp(); }

int main(int argc, char **argv) {
  auto app = Gini::CreateApplication();
  app->Run();
  delete app;
  return 0;
}
