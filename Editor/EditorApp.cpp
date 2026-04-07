#include "EditorApp.h"
#include "ImGui/ImGuizmo.h"
#include "Renderer/Renderer3D.h"
#include "UI/ImGuiLayer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <limits>

namespace Gini {

EditorApp::EditorApp()
    : Application([]() {
        EngineConfig config;
        config.windowTitle = "Gini Editor";
        config.windowWidth = 1920;
        config.windowHeight = 1080;
        config.vsync = true;
        config.enableRenderThread = false;
        config.enableAssetLoadingThread = true;
        config.enableNetworkThread = false;
        return config;
      }()) {}

void EditorApp::OnInit() {
  GINI_INFO("Gini Editor initialized!");

  // Initialize ImGui
  ImGuiLayer::Init();

  // Initialize 3D Renderer
  Renderer3D::Init();

  // Initialize Weather System
  WeatherSystem::Init();

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
  m_HierarchyPanel.SetVisible(true); // Show by default
  m_HierarchyPanel.SetSelectionCallback([this](Entity entity) {
    m_SelectedEntity = entity;
    m_PropertiesPanel.SetSelectedEntity(entity);
  });

  m_PropertiesPanel.SetScene(m_ActiveScene);
  m_PropertiesPanel.SetVisible(true); // Show by default
  m_ScenePropertiesPanel.SetScene(m_ActiveScene);
  m_ScenePropertiesPanel.SetVisible(true); // Show by default

  // Show other default panels
  m_StatsPanel.SetVisible(true);
  m_ConsolePanel.SetVisible(true);
  m_TerrainPanel.SetVisible(true);
  m_AssetBrowserPanel.SetVisible(true);
  m_WeatherPanel.SetVisible(true);

  // Create default terrain
  m_Terrain = Terrain::Create(128, 128, 50.0f);
  m_Terrain->GenerateFromNoise(0.03f, 50.0f, 4);
  m_TerrainPanel.SetTerrain(m_Terrain);

  // Project launcher will be shown automatically (m_IsOpen = true by default)
}

void EditorApp::OnShutdown() {
  WeatherSystem::Shutdown();
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

  // Store deltaTime for terrain painting (called in DrawViewport after bounds
  // are set)
  m_DeltaTime = deltaTime;

  // Update terrain editor window
  m_TerrainEditorWindow.OnUpdate(deltaTime);

  // Update weather panel
  m_WeatherPanel.OnUpdate(deltaTime);

  // Update thread analysis panel
  m_ThreadAnalysisPanel.OnUpdate(deltaTime);

  // Update scene
  if (m_SceneState == SceneState::Play && m_ActiveScene) {
    m_ActiveScene->OnUpdate(deltaTime);
  }

  // Update atmospheric sky for cloud animation
  if (m_ActiveScene && m_ActiveScene->IsAtmosphericSkyEnabled() &&
      m_ActiveScene->HasAtmosphericSky()) {
    m_ActiveScene->GetAtmosphericSky()->Update(deltaTime);
  }

  // Update weather system
  WeatherSystem::Update(deltaTime);
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

  // Render atmospheric sky if enabled
  if (m_ActiveScene && m_ActiveScene->IsAtmosphericSkyEnabled() &&
      m_ActiveScene->HasAtmosphericSky()) {
    m_ActiveScene->GetAtmosphericSky()->Render(*m_EditorCamera);
  }

  // Render terrain - prefer scene terrain if linked, otherwise use editor
  // terrain Pass sun settings from atmospheric sky if available
  if (m_ActiveScene && m_ActiveScene->HasTerrain()) {
    auto terrain = m_ActiveScene->GetTerrain();
    if (m_ActiveScene->IsAtmosphericSkyEnabled() &&
        m_ActiveScene->HasAtmosphericSky()) {
      auto sky = m_ActiveScene->GetAtmosphericSky();
      terrain->Render(*m_EditorCamera, sky->GetSunDirection(),
                      sky->GetSunColor(), 0.2f);
    } else {
      terrain->Render(*m_EditorCamera);
    }
  } else if (m_Terrain) {
    m_Terrain->Render(*m_EditorCamera);
  }

  if (m_ActiveScene) {
    // Draw grid (only if no terrain)
    for (int i = -10; i <= 10; i++) {
      Color gridColor =
          (i == 0) ? Color(0.5f, 0.5f, 0.5f) : Color(0.3f, 0.3f, 0.3f);
      Renderer3D::DrawLine(Vec3(i, 0, -10), Vec3(i, 0, 10), gridColor);
      Renderer3D::DrawLine(Vec3(-10, 0, i), Vec3(10, 0, i), gridColor);
    }
    // Draw axis lines
    Renderer3D::DrawLine(Vec3(0, 0, 0), Vec3(5, 0, 0),
                         Color(1.0f, 0.2f, 0.2f)); // X red
    Renderer3D::DrawLine(Vec3(0, 0, 0), Vec3(0, 5, 0),
                         Color(0.2f, 1.0f, 0.2f)); // Y green
    Renderer3D::DrawLine(Vec3(0, 0, 0), Vec3(0, 0, 5),
                         Color(0.2f, 0.2f, 1.0f)); // Z blue

    // Draw entities with transforms as cubes - keep original colors always
    auto &world = m_ActiveScene->GetWorld();
    auto view = world.GetRegistry().view<TransformComponent>();
    int entityIndex = 0;
    for (auto entity : view) {
      auto &transform = view.get<TransformComponent>(entity);

      // Fixed colors for each cube (based on creation order)
      Color cubeColor;
      switch (entityIndex % 3) {
      case 0:
        cubeColor = Color(0.8f, 0.3f, 0.3f);
        break; // Red
      case 1:
        cubeColor = Color(0.3f, 0.8f, 0.3f);
        break; // Green
      case 2:
        cubeColor = Color(0.3f, 0.3f, 0.8f);
        break; // Blue
      }

      // Draw cube at entity position
      Renderer3D::DrawCube(transform.position, transform.scale, cubeColor);

      // Draw selection wireframe for selected entity (yellow outline only)
      if (entity == m_SelectedEntity) {
        Vec3 halfSize =
            transform.scale * 0.55f; // Slightly larger for visibility
        Vec3 p = transform.position;
        Color wireColor(1.0f, 0.8f, 0.0f);
        // Bottom face
        Renderer3D::DrawLine(p + Vec3(-halfSize.x, -halfSize.y, -halfSize.z),
                             p + Vec3(halfSize.x, -halfSize.y, -halfSize.z),
                             wireColor);
        Renderer3D::DrawLine(p + Vec3(halfSize.x, -halfSize.y, -halfSize.z),
                             p + Vec3(halfSize.x, -halfSize.y, halfSize.z),
                             wireColor);
        Renderer3D::DrawLine(p + Vec3(halfSize.x, -halfSize.y, halfSize.z),
                             p + Vec3(-halfSize.x, -halfSize.y, halfSize.z),
                             wireColor);
        Renderer3D::DrawLine(p + Vec3(-halfSize.x, -halfSize.y, halfSize.z),
                             p + Vec3(-halfSize.x, -halfSize.y, -halfSize.z),
                             wireColor);
        // Top face
        Renderer3D::DrawLine(p + Vec3(-halfSize.x, halfSize.y, -halfSize.z),
                             p + Vec3(halfSize.x, halfSize.y, -halfSize.z),
                             wireColor);
        Renderer3D::DrawLine(p + Vec3(halfSize.x, halfSize.y, -halfSize.z),
                             p + Vec3(halfSize.x, halfSize.y, halfSize.z),
                             wireColor);
        Renderer3D::DrawLine(p + Vec3(halfSize.x, halfSize.y, halfSize.z),
                             p + Vec3(-halfSize.x, halfSize.y, halfSize.z),
                             wireColor);
        Renderer3D::DrawLine(p + Vec3(-halfSize.x, halfSize.y, halfSize.z),
                             p + Vec3(-halfSize.x, halfSize.y, -halfSize.z),
                             wireColor);
        // Vertical edges
        Renderer3D::DrawLine(p + Vec3(-halfSize.x, -halfSize.y, -halfSize.z),
                             p + Vec3(-halfSize.x, halfSize.y, -halfSize.z),
                             wireColor);
        Renderer3D::DrawLine(p + Vec3(halfSize.x, -halfSize.y, -halfSize.z),
                             p + Vec3(halfSize.x, halfSize.y, -halfSize.z),
                             wireColor);
        Renderer3D::DrawLine(p + Vec3(halfSize.x, -halfSize.y, halfSize.z),
                             p + Vec3(halfSize.x, halfSize.y, halfSize.z),
                             wireColor);
        Renderer3D::DrawLine(p + Vec3(-halfSize.x, -halfSize.y, halfSize.z),
                             p + Vec3(-halfSize.x, halfSize.y, halfSize.z),
                             wireColor);
      }

      entityIndex++;
    }
  }

  // Update rain position to follow camera for immersive effect
  WeatherSystem::UpdateRainPosition(m_EditorCamera->GetPosition());

  // Render weather effects before ending scene (so it goes to framebuffer)
  WeatherSystem::Render(m_EditorCamera->GetViewProjectionMatrix());

  Renderer3D::EndScene();
  m_Framebuffer->Unbind();

  // Render ImGui
  ImGuiLayer::Begin();

  // Show Project Launcher if no project is loaded
  if (m_ProjectLauncher.IsOpen()) {
    m_ProjectLauncher.OnImGuiRender();

    // Check if project was loaded
    if (m_ProjectLauncher.HasProjectLoaded()) {
      m_ActiveProject = m_ProjectLauncher.GetLoadedProject();
      OnProjectLoaded();
    }
  } else if (m_TerrainEditorWindow.IsOpen()) {
    // If Terrain Editor is open, only render it (fullscreen mode)
    m_TerrainEditorWindow.OnImGuiRender();
  } else if (m_MaterialEditorPanel.IsOpen()) {
    // If Material Editor is open, only render it (fullscreen mode)
    m_MaterialEditorPanel.OnImGuiRender();
  } else {
    // Normal editor mode
    SetupDockspace();
    DrawMenuBar();
    DrawToolbar();
    DrawViewport();

    // Draw panels
    m_HierarchyPanel.OnImGuiRender();
    m_PropertiesPanel.OnImGuiRender();
    m_StatsPanel.OnImGuiRender();
    m_ConsolePanel.OnImGuiRender();
    m_TerrainPanel.OnImGuiRender();
    m_AssetBrowserPanel.OnImGuiRender();
    m_ScenePropertiesPanel.OnImGuiRender();
    m_WeatherPanel.OnImGuiRender();
    m_ThreadAnalysisPanel.OnImGuiRender();
  }

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

  // Mouse click for entity selection
  if (event.GetType() == EventType::MouseButtonPressed && m_ViewportHovered) {
    auto &e = static_cast<MouseButtonPressedEvent &>(event);
    if (e.GetButton() == static_cast<i32>(MouseButton::Left)) {
      // Don't select if using gizmo or camera controls
      if (!Input::Get().IsKeyDown(Key::LeftAlt) && !ImGuizmo::IsOver()) {
        Vec2 mousePos = Input::Get().GetMousePosition();
        Entity picked = PickEntity(mousePos);
        m_SelectedEntity = picked;
        m_PropertiesPanel.SetSelectedEntity(picked);
        m_HierarchyPanel.SetSelectedEntity(picked);
      }
    }
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

    // Gizmo shortcuts (only when not typing in text field)
    if (!ImGui::GetIO().WantTextInput) {
      i32 keyCode = e.GetKeyCode();
      if (keyCode == static_cast<i32>(Key::W))
        m_GizmoOperation = GizmoOperation::Translate;
      else if (keyCode == static_cast<i32>(Key::E))
        m_GizmoOperation = GizmoOperation::Rotate;
      else if (keyCode == static_cast<i32>(Key::R))
        m_GizmoOperation = GizmoOperation::Scale;

      // Toggle snap with Ctrl
      if (keyCode == static_cast<i32>(Key::LeftControl) ||
          keyCode == static_cast<i32>(Key::RightControl))
        m_GizmoUsingSnap = true;
    }
  }

  // Release snap when Ctrl is released
  if (event.GetType() == EventType::KeyReleased) {
    auto &e = static_cast<KeyReleasedEvent &>(event);
    i32 keyCode = e.GetKeyCode();
    if (keyCode == static_cast<i32>(Key::LeftControl) ||
        keyCode == static_cast<i32>(Key::RightControl))
      m_GizmoUsingSnap = false;
  }
}

void EditorApp::SetupDockspace() {
  static bool dockspaceOpen = true;
  static bool firstTime = true;
  static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

  ImGuiWindowFlags windowFlags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  windowFlags |=
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace Demo", &dockspaceOpen, windowFlags);
  ImGui::PopStyleVar(3);

  // DockSpace
  ImGuiIO &io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
    ImGuiID dockspaceId = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

    // Setup default layout on first run
    if (firstTime) {
      firstTime = false;
      ImGui::DockBuilderRemoveNode(dockspaceId);
      ImGui::DockBuilderAddNode(dockspaceId,
                                dockspaceFlags | ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

      // Split the dockspace
      ImGuiID dockLeft, dockRight, dockBottom;
      ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.2f, &dockLeft,
                                  &dockRight);
      ImGuiID dockRightBottom;
      ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Down, 0.25f,
                                  &dockRightBottom, &dockRight);
      ImGuiID dockRightRight;
      ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Right, 0.25f,
                                  &dockRightRight, &dockRight);

      // Dock windows to nodes
      ImGui::DockBuilderDockWindow("Scene Hierarchy", dockLeft);
      ImGui::DockBuilderDockWindow("Properties", dockRightRight);
      ImGui::DockBuilderDockWindow("Viewport", dockRight);
      ImGui::DockBuilderDockWindow("Console", dockRightBottom);
      ImGui::DockBuilderDockWindow("Stats", dockRightBottom);
      ImGui::DockBuilderDockWindow("##toolbar", dockRight);

      ImGui::DockBuilderFinish(dockspaceId);
    }
  }

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

    if (ImGui::BeginMenu("Tools")) {
      if (ImGui::MenuItem("Terrain Editor")) {
        m_TerrainEditorWindow.Open();
      }
      if (ImGui::MenuItem("Material Editor")) {
        m_MaterialEditorPanel.NewMaterial();
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
      ImGui::MenuItem("Scene Properties", nullptr,
                      &m_ScenePropertiesPanel.m_Visible);
      ImGui::MenuItem("Stats", nullptr, &m_StatsPanel.m_Visible);
      ImGui::MenuItem("Console", nullptr, &m_ConsolePanel.m_Visible);
      ImGui::MenuItem("Asset Browser", nullptr, &m_AssetBrowserPanel.m_Visible);
      ImGui::MenuItem("Weather System", nullptr,
                      &m_WeatherPanel.GetVisibleRef());
      ImGui::MenuItem("Thread Analysis", nullptr,
                      &m_ThreadAnalysisPanel.m_Visible);
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
      bool canIncrease = (ImGuiLayer::m_fontSize + 2.0f) < 23.0f;
      bool canDecrease = (ImGuiLayer::m_fontSize - 2.0f) > 10.0f;

      if (ImGui::MenuItem("Increase Font Size", nullptr, false, canIncrease)) {
        ImGuiLayer::SetFontSize(ImGuiLayer::m_fontSize + 2.0f);
      }

      if (ImGui::MenuItem("Decrease Font Size", nullptr, false, canDecrease)) {
        ImGuiLayer::SetFontSize(ImGuiLayer::m_fontSize - 2.0f);
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
  ImVec2 imagePos = ImGui::GetCursorScreenPos();
  ImGui::Image((void *)(intptr_t)textureID,
               ImVec2(m_ViewportSize.x, m_ViewportSize.y), ImVec2(0, 1),
               ImVec2(1, 0));

  // Store viewport bounds for mouse picking (after image is placed)
  m_ViewportBounds[0] = {imagePos.x, imagePos.y};
  m_ViewportBounds[1] = {imagePos.x + m_ViewportSize.x,
                         imagePos.y + m_ViewportSize.y};

  // Handle terrain painting now that viewport bounds are set correctly
  HandleTerrainPainting(m_DeltaTime);

  // ImGuizmo
  if (m_SelectedEntity != NullEntity && m_ActiveScene) {
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();

    ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y,
                      m_ViewportBounds[1].x - m_ViewportBounds[0].x,
                      m_ViewportBounds[1].y - m_ViewportBounds[0].y);

    // Get camera matrices
    glm::mat4 cameraView = m_EditorCamera->GetViewMatrix();
    glm::mat4 cameraProjection = m_EditorCamera->GetProjectionMatrix();

    // Get entity transform
    auto &world = m_ActiveScene->GetWorld();
    if (world.HasComponent<TransformComponent>(m_SelectedEntity)) {
      auto &tc = world.GetComponent<TransformComponent>(m_SelectedEntity);
      glm::mat4 transform = tc.GetTransform();

      // Determine gizmo operation
      ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
      switch (m_GizmoOperation) {
      case GizmoOperation::Translate:
        operation = ImGuizmo::TRANSLATE;
        break;
      case GizmoOperation::Rotate:
        operation = ImGuizmo::ROTATE;
        break;
      case GizmoOperation::Scale:
        operation = ImGuizmo::SCALE;
        break;
      }

      // Snapping
      float snapValue = 0.0f;
      float snapValues[3] = {0.0f, 0.0f, 0.0f};
      if (m_GizmoUsingSnap) {
        if (m_GizmoOperation == GizmoOperation::Translate)
          snapValue = m_SnapTranslate;
        else if (m_GizmoOperation == GizmoOperation::Rotate)
          snapValue = m_SnapRotate;
        else if (m_GizmoOperation == GizmoOperation::Scale)
          snapValue = m_SnapScale;
        snapValues[0] = snapValues[1] = snapValues[2] = snapValue;
      }

      // Manipulate
      bool manipulated = ImGuizmo::Manipulate(
          glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
          operation, ImGuizmo::LOCAL, glm::value_ptr(transform), nullptr,
          m_GizmoUsingSnap ? snapValues : nullptr);

      if (manipulated) {
        // Decompose the matrix back to position, rotation, scale
        glm::vec3 translation, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(transform), glm::value_ptr(translation),
            glm::value_ptr(rotation), glm::value_ptr(scale));
        tc.position = translation;
        tc.rotation = glm::radians(rotation); // ImGuizmo returns degrees
        tc.scale = scale;
      }
    }
  }

  ImGui::End();
  ImGui::PopStyleVar();
}

void EditorApp::NewScene() {
  m_ActiveScene = CreateRef<Scene>("Untitled Scene");
  m_HierarchyPanel.SetScene(m_ActiveScene);
  m_PropertiesPanel.SetScene(m_ActiveScene);
  m_ScenePropertiesPanel.SetScene(m_ActiveScene);
  m_SelectedEntity = NullEntity;

  // Create some default entities to click on
  auto cube1 = m_ActiveScene->CreateEntity("Cube 1");
  auto &t1 = m_ActiveScene->GetWorld().GetComponent<TransformComponent>(cube1);
  t1.position = Vec3(-3.0f, 0.5f, 0.0f);

  auto cube2 = m_ActiveScene->CreateEntity("Cube 2");
  auto &t2 = m_ActiveScene->GetWorld().GetComponent<TransformComponent>(cube2);
  t2.position = Vec3(0.0f, 0.5f, 0.0f);

  auto cube3 = m_ActiveScene->CreateEntity("Cube 3");
  auto &t3 = m_ActiveScene->GetWorld().GetComponent<TransformComponent>(cube3);
  t3.position = Vec3(3.0f, 0.5f, 0.0f);

  GINI_INFO("New scene created with 3 cubes");
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

void EditorApp::OnProjectLoaded() {
  if (!m_ActiveProject)
    return;

  GINI_INFO("OnProjectLoaded: begin");

  // Set as active project globally
  GINI_INFO("OnProjectLoaded: set active project");
  Project::SetActive(m_ActiveProject);

  const auto &config = m_ActiveProject->GetConfig();
  GINI_INFO("Project loaded: ", config.name);
  GINI_INFO("Assets path: ", config.assetsPath.string());

  // Set up Asset Browser with project assets path
  GINI_INFO("OnProjectLoaded: set asset browser root start");
  m_AssetBrowserPanel.SetRootPath(config.assetsPath);
  GINI_INFO("OnProjectLoaded: set asset browser root complete");

  // Create default scene if none exists
  GINI_INFO("OnProjectLoaded: create new scene start");
  NewScene();
  GINI_INFO("OnProjectLoaded: create new scene complete");

  // Close the launcher
  GINI_INFO("OnProjectLoaded: close project launcher");
  m_ProjectLauncher.Close();
  GINI_INFO("OnProjectLoaded: complete");
}

Entity EditorApp::PickEntity(const Vec2 &mousePos) {
  if (!m_ActiveScene)
    return NullEntity;

  // Convert mouse position to normalized device coordinates
  float mouseX = mousePos.x - m_ViewportBounds[0].x;
  float mouseY = mousePos.y - m_ViewportBounds[0].y;
  float viewportWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
  float viewportHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

  // Normalize to [-1, 1]
  float ndcX = (2.0f * mouseX) / viewportWidth - 1.0f;
  float ndcY = 1.0f - (2.0f * mouseY) / viewportHeight; // Flip Y

  // Get inverse view-projection matrix
  Mat4 invViewProj = glm::inverse(m_EditorCamera->GetViewProjectionMatrix());

  // Create ray in world space
  Vec4 rayClipNear = Vec4(ndcX, ndcY, -1.0f, 1.0f);
  Vec4 rayClipFar = Vec4(ndcX, ndcY, 1.0f, 1.0f);

  Vec4 rayWorldNear = invViewProj * rayClipNear;
  Vec4 rayWorldFar = invViewProj * rayClipFar;

  rayWorldNear /= rayWorldNear.w;
  rayWorldFar /= rayWorldFar.w;

  Vec3 rayOrigin = Vec3(rayWorldNear);
  Vec3 rayDir = glm::normalize(Vec3(rayWorldFar) - Vec3(rayWorldNear));

  // Test intersection with all entities - collect all hits and sort by distance
  Entity closestEntity = NullEntity;
  float closestT = std::numeric_limits<float>::max();

  auto &world = m_ActiveScene->GetWorld();
  auto view = world.GetRegistry().view<TransformComponent, TagComponent>();

  for (auto entity : view) {
    auto &transform = view.get<TransformComponent>(entity);

    // Calculate AABB for entity (assuming unit cube scaled by transform.scale)
    Vec3 halfSize = transform.scale * 0.5f;
    Vec3 boxMin = transform.position - halfSize;
    Vec3 boxMax = transform.position + halfSize;

    float t;
    if (RayIntersectsAABB(rayOrigin, rayDir, boxMin, boxMax, t)) {
      if (t < closestT && t > 0.0f) {
        closestT = t;
        closestEntity = entity;
      }
    }
  }

  return closestEntity;
}

bool EditorApp::RayIntersectsAABB(const Vec3 &rayOrigin, const Vec3 &rayDir,
                                  const Vec3 &boxMin, const Vec3 &boxMax,
                                  float &t) {
  Vec3 invDir = 1.0f / rayDir;

  Vec3 t1 = (boxMin - rayOrigin) * invDir;
  Vec3 t2 = (boxMax - rayOrigin) * invDir;

  Vec3 tMin = glm::min(t1, t2);
  Vec3 tMax = glm::max(t1, t2);

  float tNear = glm::max(glm::max(tMin.x, tMin.y), tMin.z);
  float tFar = glm::min(glm::min(tMax.x, tMax.y), tMax.z);

  if (tNear > tFar || tFar < 0.0f) {
    return false;
  }

  t = tNear;
  return true;
}

Vec3 EditorApp::ScreenToWorldRay(const Vec2 &screenPos) {
  // Convert screen position to normalized device coordinates
  Vec2 viewportPos =
      screenPos - Vec2(m_ViewportBounds[0].x, m_ViewportBounds[0].y);
  Vec2 ndc;
  ndc.x = (2.0f * viewportPos.x) / m_ViewportSize.x - 1.0f;
  ndc.y = 1.0f - (2.0f * viewportPos.y) / m_ViewportSize.y;

  // Create ray in clip space
  Vec4 rayClip(ndc.x, ndc.y, -1.0f, 1.0f);

  // Transform to eye space
  Mat4 invProj = glm::inverse(m_EditorCamera->GetProjectionMatrix());
  Vec4 rayEye = invProj * rayClip;
  rayEye = Vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

  // Transform to world space
  Mat4 invView = glm::inverse(m_EditorCamera->GetViewMatrix());
  Vec4 rayWorld = invView * rayEye;

  return glm::normalize(Vec3(rayWorld));
}

void EditorApp::HandleTerrainPainting(f32 deltaTime) {
  if (!m_Terrain || !m_ViewportHovered) {
    m_TerrainHit = false;
    return;
  }

  auto &input = Input::Get();
  Vec2 mousePos = input.GetMousePosition();

  // Get ray from camera through mouse position
  Vec3 rayOrigin = m_EditorCamera->GetPosition();
  Vec3 rayDir = ScreenToWorldRay(mousePos);

  // Raycast against terrain
  m_TerrainHit = m_Terrain->Raycast(rayOrigin, rayDir, m_TerrainHitPoint);

  // Check if we should paint
  auto paintMode = m_TerrainPanel.GetPaintMode();
  if (paintMode == TerrainPanel::PaintMode::None) {
    return;
  }

  // Paint on left mouse button (but not when using camera controls)
  bool isLeftDown = input.IsMouseButtonDown(MouseButton::Left);
  bool isRightDown = input.IsMouseButtonDown(MouseButton::Right);
  bool isAltDown = input.IsKeyDown(Key::LeftAlt);

  if (isLeftDown && !isRightDown && !isAltDown && m_TerrainHit) {
    auto &brush = m_TerrainPanel.GetBrush();
    f32 strength = brush.strength * deltaTime * 10.0f; // Scale by deltaTime

    switch (paintMode) {
    case TerrainPanel::PaintMode::RaiseHeight:
      m_Terrain->PaintHeight(m_TerrainHitPoint.x, m_TerrainHitPoint.z,
                             brush.radius, strength, true);
      break;
    case TerrainPanel::PaintMode::LowerHeight:
      m_Terrain->PaintHeight(m_TerrainHitPoint.x, m_TerrainHitPoint.z,
                             brush.radius, strength, false);
      break;
    case TerrainPanel::PaintMode::Smooth:
      m_Terrain->SmoothHeight(m_TerrainHitPoint.x, m_TerrainHitPoint.z,
                              brush.radius, strength);
      break;
    case TerrainPanel::PaintMode::Flatten:
      m_Terrain->FlattenHeight(m_TerrainHitPoint.x, m_TerrainHitPoint.z,
                               brush.radius, m_TerrainHitPoint.y);
      break;
    case TerrainPanel::PaintMode::PaintMaterial:
      m_Terrain->PaintMaterial(m_TerrainHitPoint.x, m_TerrainHitPoint.z,
                               brush.radius, strength,
                               m_TerrainPanel.GetSelectedMaterialLayer());
      break;
    default:
      break;
    }
  }
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
