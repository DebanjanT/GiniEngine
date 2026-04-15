#include "EditorApp.h"
#include "Animation/Animation.h"
#include "ImGui/ImGuizmo.h"
#include "Renderer/IBL.h"
#include "Renderer/Light.h"
#include "Asset/AssimpMeshImporter.h"
#include "Assets/AssetManager.h"
#include "Asset/AssetRegistry.h"
#include "Renderer/MeshSource.h"
#include "Renderer/MaterialAsset.h"
#include "Renderer/PostProcess.h"
#include "Renderer/Renderer3D.h"
#include "Renderer/ShadowMap.h"
#include "Renderer/SSAO.h"
#include "Scene/SceneSerializer.h"
#include "UI/ImGuiLayer.h"
#include "Utils/FileDialog.h"

#include <glad/gl.h>

#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <limits>
#include <yaml-cpp/yaml.h>

namespace Gini {

EditorApp::EditorApp()
    : Application([]() {
        EngineConfig config;
        config.windowTitle = "Gini Editor";
        config.windowWidth = 1920;
        config.windowHeight = 1080;
        config.vsync = true;
        config.enableRenderThread = true;
        config.enableAssetLoadingThread = true;
        config.enableNetworkThread = true;
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

  // Create HDR framebuffer for 3D scene rendering
  FramebufferSpec hdrSpec;
  hdrSpec.width = 1280;
  hdrSpec.height = 720;
  hdrSpec.colorAttachments = {
      {FramebufferTextureFormat::RGBA16F},
      {FramebufferTextureFormat::RGB16F}
  };
  m_HDRFramebuffer = Framebuffer::Create(hdrSpec);

  // Create final LDR framebuffer for ImGui viewport display
  FramebufferSpec fbSpec;
  fbSpec.width = 1280;
  fbSpec.height = 720;
  fbSpec.colorAttachments = {{FramebufferTextureFormat::RGBA8}};
  m_Framebuffer = Framebuffer::Create(fbSpec);

  // Initialize post-processing, shadows, IBL, SSAO
  PostProcess::Init();
  ShadowMap::Init();
  IBL::Init();
  SSAO::Init(1280, 720);

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
  m_WeatherPanel.SetVisible(false);

  // No default terrain -- user creates/links terrain via Scene Properties or Terrain Editor
  m_Terrain = nullptr;

  // Project launcher will be shown automatically (m_IsOpen = true by default)
}

void EditorApp::OnShutdown() {
  SSAO::Shutdown();
  IBL::Shutdown();
  ShadowMap::Shutdown();
  PostProcess::Shutdown();
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

  WeatherSystem::Update(deltaTime);
}

void EditorApp::OnRender() {
  // -- Shadow pass --
  if (ShadowMap::IsInitialized()) {
    auto& lightMgr = LightManager::Get();
    if (lightMgr.HasDirectionalLight()) {
      ShadowMap::BeginShadowPass(*m_EditorCamera, lightMgr.GetDirectionalLight());

      auto depthShader = ShadowMap::GetDepthShader();
      if (depthShader) {
        const auto& matrices = ShadowMap::GetLightSpaceMatrices();
        for (u32 c = 0; c < static_cast<u32>(matrices.size()); c++) {
          glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                    ShadowMap::GetShadowMapTexture(), 0, c);
          glClear(GL_DEPTH_BUFFER_BIT);
          depthShader->Bind();
          depthShader->SetMat4("u_LightSpaceMatrix", matrices[c]);

          if (m_Terrain) {
            depthShader->SetMat4("u_Model", glm::translate(Mat4(1.0f), m_Terrain->GetWorldPosition()));
          }

          if (m_ActiveScene) {
            auto &shadowWorld = m_ActiveScene->GetWorld();
            auto shadowView = shadowWorld.GetRegistry().view<TransformComponent, StaticMeshComponent>();
            for (auto ent : shadowView) {
              auto &smc = shadowView.get<StaticMeshComponent>(ent);
              if (!smc.castShadows || !smc.visible) continue;
              auto &tc = shadowView.get<TransformComponent>(ent);
              Mat4 modelMat = tc.GetTransform();
              depthShader->SetMat4("u_Model", modelMat);

              // Render primitive types for shadows
              if (smc.primitiveType != MeshType::None) {
                Ref<Mesh> primMesh;
                switch (smc.primitiveType) {
                case MeshType::Cube: primMesh = Mesh::CreateCube(); break;
                case MeshType::Sphere: primMesh = Mesh::CreateSphere(); break;
                case MeshType::Plane: primMesh = Mesh::CreatePlane(); break;
                case MeshType::Cylinder: primMesh = Mesh::CreateCylinder(); break;
                default: break;
                }
                if (primMesh) primMesh->Draw();
              }
            }
          }
        }
      }
      ShadowMap::EndShadowPass();
    }
  }

  // -- IBL generation (once or when sky changes) --
  static bool iblGenerated = false;
  if (!iblGenerated && m_ActiveScene && m_ActiveScene->IsAtmosphericSkyEnabled() &&
      m_ActiveScene->HasAtmosphericSky()) {
    IBL::CaptureAtmosphericSky(m_ActiveScene->GetAtmosphericSky().get(), *m_EditorCamera);
    iblGenerated = true;
  }

  // -- Main HDR render pass --
  m_HDRFramebuffer->Bind();
  Renderer3D::SetViewport(0, 0, static_cast<u32>(m_ViewportSize.x),
                          static_cast<u32>(m_ViewportSize.y));
  Renderer3D::SetClearColor(Color(0.0f, 0.0f, 0.0f));
  Renderer3D::Clear();

  Renderer3D::BeginScene(*m_EditorCamera);

  // Render skybox if enabled (takes priority over atmospheric sky)
  if (m_ActiveScene && m_ActiveScene->IsSkyboxEnabled() &&
      m_ActiveScene->HasSkybox()) {
    m_ActiveScene->GetSkybox()->Render(*m_EditorCamera);
  }
  // Render atmospheric sky if enabled (fallback if skybox not enabled)
  if (m_ActiveScene && m_ActiveScene->IsAtmosphericSkyEnabled() &&
      m_ActiveScene->HasAtmosphericSky()) {
    m_ActiveScene->GetAtmosphericSky()->Render(*m_EditorCamera);
  }

  // Render terrain
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
    // Draw grid
    for (int i = -10; i <= 10; i++) {
      Color gridColor =
          (i == 0) ? Color(0.5f, 0.5f, 0.5f) : Color(0.3f, 0.3f, 0.3f);
      Renderer3D::DrawLine(Vec3(i, 0, -10), Vec3(i, 0, 10), gridColor);
      Renderer3D::DrawLine(Vec3(-10, 0, i), Vec3(10, 0, i), gridColor);
    }
    // Draw axis lines
    Renderer3D::DrawLine(Vec3(0, 0, 0), Vec3(5, 0, 0),
                         Color(1.0f, 0.2f, 0.2f));
    Renderer3D::DrawLine(Vec3(0, 0, 0), Vec3(0, 5, 0),
                         Color(0.2f, 1.0f, 0.2f));
    Renderer3D::DrawLine(Vec3(0, 0, 0), Vec3(0, 0, 5),
                         Color(0.2f, 0.2f, 1.0f));

    // Draw entities using StaticMeshComponent
    auto &world = m_ActiveScene->GetWorld();
    auto meshView = world.GetRegistry().view<TransformComponent, StaticMeshComponent>();
    for (auto entity : meshView) {
      auto &transform = meshView.get<TransformComponent>(entity);
      auto &smc = meshView.get<StaticMeshComponent>(entity);
      
      if (!smc.visible) continue;
      
      Mat4 modelMatrix = transform.GetTransform();

      Material3D mat3d;
      if (world.HasComponent<MaterialComponent>(entity)) {
        auto &matComp = world.GetComponent<MaterialComponent>(entity);
        mat3d.albedo = matComp.albedo;
        mat3d.metallic = matComp.metallic;
        mat3d.roughness = matComp.roughness;
        mat3d.ao = matComp.ao;
        mat3d.emissive = matComp.emissive;
      }

      // Render primitive types
      if (smc.primitiveType != MeshType::None) {
        Renderer3D::RenderPrimitive(smc.primitiveType, modelMatrix, mat3d);
      } else if (smc.meshSourceHandle != 0) {
        // Render MeshSource
        Renderer3D::RenderStaticMesh(smc.meshSourceHandle, modelMatrix,
                                     smc.submeshIndices, smc.materialOverrides);
      }
    }

    // Draw entities without StaticMeshComponent (fallback)
    auto view = world.GetRegistry().view<TransformComponent>(entt::exclude<StaticMeshComponent>);
    int entityIndex = 0;
    for (auto entity : view) {
      auto &transform = view.get<TransformComponent>(entity);
      if (false) {
        Color cubeColor;
        switch (entityIndex % 3) {
        case 0: cubeColor = Color(0.8f, 0.3f, 0.3f); break;
        case 1: cubeColor = Color(0.3f, 0.8f, 0.3f); break;
        case 2: cubeColor = Color(0.3f, 0.3f, 0.8f); break;
        }
        Renderer3D::DrawCube(transform.position, transform.scale, cubeColor);
      }

      if (entity == m_SelectedEntity) {
        Vec3 halfSize = transform.scale * 0.55f;
        Vec3 p = transform.position;
        Color wireColor(1.0f, 0.8f, 0.0f);
        Renderer3D::DrawLine(p + Vec3(-halfSize.x, -halfSize.y, -halfSize.z),
                             p + Vec3(halfSize.x, -halfSize.y, -halfSize.z), wireColor);
        Renderer3D::DrawLine(p + Vec3(halfSize.x, -halfSize.y, -halfSize.z),
                             p + Vec3(halfSize.x, -halfSize.y, halfSize.z), wireColor);
        Renderer3D::DrawLine(p + Vec3(halfSize.x, -halfSize.y, halfSize.z),
                             p + Vec3(-halfSize.x, -halfSize.y, halfSize.z), wireColor);
        Renderer3D::DrawLine(p + Vec3(-halfSize.x, -halfSize.y, halfSize.z),
                             p + Vec3(-halfSize.x, -halfSize.y, -halfSize.z), wireColor);
        Renderer3D::DrawLine(p + Vec3(-halfSize.x, halfSize.y, -halfSize.z),
                             p + Vec3(halfSize.x, halfSize.y, -halfSize.z), wireColor);
        Renderer3D::DrawLine(p + Vec3(halfSize.x, halfSize.y, -halfSize.z),
                             p + Vec3(halfSize.x, halfSize.y, halfSize.z), wireColor);
        Renderer3D::DrawLine(p + Vec3(halfSize.x, halfSize.y, halfSize.z),
                             p + Vec3(-halfSize.x, halfSize.y, halfSize.z), wireColor);
        Renderer3D::DrawLine(p + Vec3(-halfSize.x, halfSize.y, halfSize.z),
                             p + Vec3(-halfSize.x, halfSize.y, -halfSize.z), wireColor);
        Renderer3D::DrawLine(p + Vec3(-halfSize.x, -halfSize.y, -halfSize.z),
                             p + Vec3(-halfSize.x, halfSize.y, -halfSize.z), wireColor);
        Renderer3D::DrawLine(p + Vec3(halfSize.x, -halfSize.y, -halfSize.z),
                             p + Vec3(halfSize.x, halfSize.y, -halfSize.z), wireColor);
        Renderer3D::DrawLine(p + Vec3(halfSize.x, -halfSize.y, halfSize.z),
                             p + Vec3(halfSize.x, halfSize.y, halfSize.z), wireColor);
        Renderer3D::DrawLine(p + Vec3(-halfSize.x, -halfSize.y, halfSize.z),
                             p + Vec3(-halfSize.x, halfSize.y, halfSize.z), wireColor);
      }

      entityIndex++;
    }
  }

  WeatherSystem::UpdateRainPosition(m_EditorCamera->GetPosition());
  WeatherSystem::Render(m_EditorCamera->GetViewProjectionMatrix());

  Renderer3D::EndScene();
  m_HDRFramebuffer->Unbind();

  // -- SSAO pass --
  u32 ssaoTexture = 0;
  if (SSAO::IsInitialized()) {
    SSAO::Render(m_HDRFramebuffer->GetDepthAttachment(),
                 m_HDRFramebuffer->GetColorAttachment(1),
                 m_EditorCamera->GetProjectionMatrix(),
                 m_EditorCamera->GetViewMatrix());
    ssaoTexture = SSAO::GetSSAOTexture();
  }

  // Post-process: resolve HDR to LDR framebuffer with ACES tonemapping
  m_Framebuffer->Bind();
  Renderer3D::SetViewport(0, 0, static_cast<u32>(m_ViewportSize.x),
                          static_cast<u32>(m_ViewportSize.y));
  PostProcess::Resolve(m_HDRFramebuffer->GetColorAttachment(0), ssaoTexture, 1.0f, 2.2f);
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

    m_ModelImportDialog.OnImGuiRender();
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

  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

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
  ImGui::Begin("GiniDockHost", &dockspaceOpen, windowFlags);
  ImGui::PopStyleVar(3);

  ImGuiIO &io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
    ImGuiID dockspaceId = ImGui::GetID("GiniDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

    if (firstTime) {
      firstTime = false;
      ImGui::DockBuilderRemoveNode(dockspaceId);
      ImGui::DockBuilderAddNode(dockspaceId,
                                dockspaceFlags | ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

      // --- Professional layout ---
      // Step 1: carve bottom strip (28%) for console/asset browser
      ImGuiID dockMain, dockBottom;
      ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Down, 0.28f,
                                  &dockBottom, &dockMain);

      // Step 2: carve left panel (18%) for hierarchy
      ImGuiID dockLeft, dockCenter;
      ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.18f,
                                  &dockLeft, &dockCenter);

      // Step 3: carve right panel (22%) for properties
      ImGuiID dockRight;
      ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Right, 0.22f,
                                  &dockRight, &dockCenter);

      // Step 4: split bottom into left (console/stats) and right (asset browser)
      ImGuiID dockBottomLeft, dockBottomRight;
      ImGui::DockBuilderSplitNode(dockBottom, ImGuiDir_Left, 0.35f,
                                  &dockBottomLeft, &dockBottomRight);

      // Dock windows
      ImGui::DockBuilderDockWindow("Scene Hierarchy", dockLeft);
      ImGui::DockBuilderDockWindow("Viewport", dockCenter);
      ImGui::DockBuilderDockWindow("Properties", dockRight);
      ImGui::DockBuilderDockWindow("Scene Properties", dockRight);
      ImGui::DockBuilderDockWindow("Console", dockBottomLeft);
      ImGui::DockBuilderDockWindow("Stats", dockBottomLeft);
      ImGui::DockBuilderDockWindow("Asset Browser", dockBottomRight);
      ImGui::DockBuilderDockWindow("Weather System", dockRight);
      ImGui::DockBuilderDockWindow("Thread Analysis", dockBottomLeft);

      ImGui::DockBuilderFinish(dockspaceId);
    }
  }

  ImGui::End();
}

void EditorApp::DrawMenuBar() {
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
  if (ImGui::BeginMainMenuBar()) {
    // Engine name badge
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.56f, 0.72f, 1.0f));
    ImGui::Text("GINI");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 16);

    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Scene", "Ctrl+N"))
        NewScene();
      if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
        OpenScene();
      ImGui::Separator();
      if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
        SaveScene();
      if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
        SaveSceneAs();
      ImGui::Separator();
      if (ImGui::MenuItem("Exit"))
        GetWindow().Close();
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {}
      if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {}
      ImGui::Separator();
      if (ImGui::MenuItem("Preferences", nullptr, false, false)) {}
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Entity")) {
      if (ImGui::MenuItem("Create Empty Entity")) {
        if (m_ActiveScene)
          m_ActiveScene->CreateEntity("Empty Entity");
      }
      ImGui::Separator();
      if (ImGui::BeginMenu("3D Object")) {
        if (ImGui::MenuItem("Cube")) {
          if (m_ActiveScene) {
            auto e = m_ActiveScene->CreateEntity("Cube");
            auto &smc = m_ActiveScene->GetWorld().AddComponent<StaticMeshComponent>(e);
            smc.primitiveType = MeshType::Cube;
            m_ActiveScene->GetWorld().AddComponent<MaterialComponent>(e);
          }
        }
        if (ImGui::MenuItem("Sphere")) {
          if (m_ActiveScene) {
            auto e = m_ActiveScene->CreateEntity("Sphere");
            auto &smc = m_ActiveScene->GetWorld().AddComponent<StaticMeshComponent>(e);
            smc.primitiveType = MeshType::Sphere;
            m_ActiveScene->GetWorld().AddComponent<MaterialComponent>(e);
          }
        }
        if (ImGui::MenuItem("Plane")) {
          if (m_ActiveScene) {
            auto e = m_ActiveScene->CreateEntity("Plane");
            auto &smc = m_ActiveScene->GetWorld().AddComponent<StaticMeshComponent>(e);
            smc.primitiveType = MeshType::Plane;
            m_ActiveScene->GetWorld().AddComponent<MaterialComponent>(e);
          }
        }
        if (ImGui::MenuItem("Cylinder")) {
          if (m_ActiveScene) {
            auto e = m_ActiveScene->CreateEntity("Cylinder");
            auto &smc = m_ActiveScene->GetWorld().AddComponent<StaticMeshComponent>(e);
            smc.primitiveType = MeshType::Cylinder;
            m_ActiveScene->GetWorld().AddComponent<MaterialComponent>(e);
          }
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Light")) {
        if (ImGui::MenuItem("Directional Light")) {
          if (m_ActiveScene) {
            auto e = m_ActiveScene->CreateEntity("Directional Light");
            auto &lc = m_ActiveScene->GetWorld().AddComponent<LightComponent>(e);
            lc.type = 0;
          }
        }
        if (ImGui::MenuItem("Point Light")) {
          if (m_ActiveScene) {
            auto e = m_ActiveScene->CreateEntity("Point Light");
            auto &lc = m_ActiveScene->GetWorld().AddComponent<LightComponent>(e);
            lc.type = 1;
          }
        }
        if (ImGui::MenuItem("Spot Light")) {
          if (m_ActiveScene) {
            auto e = m_ActiveScene->CreateEntity("Spot Light");
            auto &lc = m_ActiveScene->GetWorld().AddComponent<LightComponent>(e);
            lc.type = 2;
          }
        }
        ImGui::EndMenu();
      }
      if (ImGui::MenuItem("Camera")) {
        if (m_ActiveScene) {
          auto e = m_ActiveScene->CreateEntity("Camera");
          m_ActiveScene->GetWorld().AddComponent<CameraComponent>(e);
        }
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Tools")) {
      if (ImGui::MenuItem("Import Model...")) {
        auto path = FileDialog::OpenFile({{"3D Models (*.fbx, *.obj, *.gltf, *.glb, *.dae)", "fbx,obj,gltf,glb,dae"}});
        if (!path.empty()) {
          m_ModelImportDialog.Open(path);
        }
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Terrain Editor"))
        m_TerrainEditorWindow.Open();
      if (ImGui::MenuItem("Material Editor"))
        m_MaterialEditorPanel.NewMaterial();
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      ImGui::TextDisabled("Panels");
      ImGui::Separator();
      ImGui::MenuItem("Scene Hierarchy", nullptr, &m_HierarchyPanel.m_Visible);
      ImGui::MenuItem("Properties", nullptr, &m_PropertiesPanel.m_Visible);
      ImGui::MenuItem("Scene Properties", nullptr,
                      &m_ScenePropertiesPanel.m_Visible);
      ImGui::MenuItem("Console", nullptr, &m_ConsolePanel.m_Visible);
      ImGui::MenuItem("Asset Browser", nullptr, &m_AssetBrowserPanel.m_Visible);
      ImGui::MenuItem("Stats", nullptr, &m_StatsPanel.m_Visible);
      ImGui::Separator();
      ImGui::TextDisabled("Systems");
      ImGui::Separator();
      ImGui::MenuItem("Weather System", nullptr,
                      &m_WeatherPanel.GetVisibleRef());
      ImGui::MenuItem("Thread Analysis", nullptr,
                      &m_ThreadAnalysisPanel.m_Visible);
      ImGui::Separator();
      ImGui::TextDisabled("Debug");
      ImGui::Separator();
      ImGui::MenuItem("ImGui Demo", nullptr, &m_ShowDemoWindow);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("About Gini Engine")) {}
      ImGui::Separator();
      bool canIncrease = (ImGuiLayer::m_fontSize + 2.0f) < 23.0f;
      bool canDecrease = (ImGuiLayer::m_fontSize - 2.0f) > 10.0f;
      if (ImGui::MenuItem("Increase Font Size", "Ctrl+=", false, canIncrease))
        ImGuiLayer::SetFontSize(ImGuiLayer::m_fontSize + 2.0f);
      if (ImGui::MenuItem("Decrease Font Size", "Ctrl+-", false, canDecrease))
        ImGuiLayer::SetFontSize(ImGuiLayer::m_fontSize - 2.0f);
      ImGui::EndMenu();
    }

    // Right-aligned FPS display
    float fpsWidth = ImGui::CalcTextSize("FPS: 999.9").x + 16.0f;
    ImGui::SameLine(ImGui::GetWindowWidth() - fpsWidth);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.56f, 0.58f, 1.0f));
    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    ImGui::PopStyleColor();

    ImGui::EndMainMenuBar();
  }
  ImGui::PopStyleVar();
}

void EditorApp::DrawToolbar() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.11f, 0.12f, 1.0f));

  ImGuiViewport *vp = ImGui::GetMainViewport();
  float toolbarHeight = 34.0f;
  ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y));
  ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, toolbarHeight));

  ImGui::Begin("##toolbar", nullptr,
               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDocking |
                   ImGuiWindowFlags_NoMove);

  float btnH = 24.0f;
  float btnW = 28.0f;

  auto ToolButton = [&](const char* label, bool selected) -> bool {
    if (selected) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.56f, 0.72f, 0.40f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.56f, 0.72f, 0.55f));
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.88f, 1.0f, 1.0f));
    } else {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.24f, 0.27f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.72f, 1.0f));
    }
    bool clicked = ImGui::Button(label, ImVec2(btnW, btnH));
    ImGui::PopStyleColor(3);
    return clicked;
  };

  // -- Gizmo mode buttons (left) --
  ImGui::SetCursorPosY((toolbarHeight - btnH) * 0.5f);
  if (ToolButton("W", m_GizmoOperation == GizmoOperation::Translate))
    m_GizmoOperation = GizmoOperation::Translate;
  ImGui::SameLine(0, 2);
  if (ToolButton("E", m_GizmoOperation == GizmoOperation::Rotate))
    m_GizmoOperation = GizmoOperation::Rotate;
  ImGui::SameLine(0, 2);
  if (ToolButton("R", m_GizmoOperation == GizmoOperation::Scale))
    m_GizmoOperation = GizmoOperation::Scale;

  ImGui::SameLine(0, 12);
  ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.30f, 0.30f, 0.32f, 0.50f));
  ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
  ImGui::PopStyleColor();
  ImGui::SameLine(0, 12);

  // Snap toggle
  ImGui::SetCursorPosY((toolbarHeight - btnH) * 0.5f);
  if (ToolButton("Snap", m_GizmoUsingSnap))
    m_GizmoUsingSnap = !m_GizmoUsingSnap;

  // -- Play/Stop (centered) --
  bool isPlaying = m_SceneState == SceneState::Play;
  float centerX = ImGui::GetWindowWidth() * 0.5f;
  float playBtnW = 70.0f;
  ImGui::SameLine(centerX - playBtnW * 0.5f);
  ImGui::SetCursorPosY((toolbarHeight - btnH) * 0.5f);

  if (isPlaying) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.22f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.30f, 0.30f, 1.0f));
  } else {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.56f, 0.72f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.65f, 0.82f, 1.0f));
  }
  if (ImGui::Button(isPlaying ? "  Stop  " : "  Play  ", ImVec2(playBtnW, btnH))) {
    if (m_SceneState == SceneState::Edit) {
      m_SceneState = SceneState::Play;
      if (m_ActiveScene) m_ActiveScene->OnStart();
    } else {
      m_SceneState = SceneState::Edit;
      if (m_ActiveScene) m_ActiveScene->OnStop();
    }
  }
  ImGui::PopStyleColor(2);

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
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
    m_HDRFramebuffer->Resize(static_cast<u32>(m_ViewportSize.x),
                             static_cast<u32>(m_ViewportSize.y));
    m_Framebuffer->Resize(static_cast<u32>(m_ViewportSize.x),
                          static_cast<u32>(m_ViewportSize.y));
    if (SSAO::IsInitialized()) {
      SSAO::Resize(static_cast<u32>(m_ViewportSize.x),
                    static_cast<u32>(m_ViewportSize.y));
    }
    m_EditorCamera->SetAspectRatio(m_ViewportSize.x / m_ViewportSize.y);
  }

  u32 textureID = m_Framebuffer->GetColorAttachment();
  ImVec2 imagePos = ImGui::GetCursorScreenPos();
  ImGui::Image((void *)(intptr_t)textureID,
               ImVec2(m_ViewportSize.x, m_ViewportSize.y), ImVec2(0, 1),
               ImVec2(1, 0));

  // Drag-drop target: accept mesh assets dragged from Asset Browser
  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload *payload =
            ImGui::AcceptDragDropPayload(AssetBrowserPanel::PAYLOAD_MESH)) {
      std::string meshPath(static_cast<const char *>(payload->Data));
      if (m_ActiveScene) {
        std::filesystem::path p(meshPath);
        std::string ext = p.extension().string();
        for (auto &c : ext) c = static_cast<char>(std::tolower(c));

        std::string modelFilePath = meshPath;
        if (ext == ".gmesh") {
          try {
            YAML::Node gmesh = YAML::LoadFile(meshPath);
            if (gmesh["SourceFile"]) {
              modelFilePath =
                  (p.parent_path() / gmesh["SourceFile"].as<std::string>())
                      .string();
            }
          } catch (...) {}
        }

        std::string name = p.stem().string();
        auto entity = m_ActiveScene->CreateEntity(name);
        auto &smc =
            m_ActiveScene->GetWorld().AddComponent<StaticMeshComponent>(entity);

        // Import mesh using AssimpMeshImporter
        AssimpMeshImporter importer(modelFilePath);
        auto result = importer.Import();
        if (result.success && result.meshSource) {
          // Add to MeshSourceLibrary with a UUID-based handle
          u64 handle = AssetRegistry::Get().RegisterAsset(p, AssetType::MeshSource);
          MeshSourceLibrary::Get().Add(handle, result.meshSource);
          smc.meshSourceHandle = handle;
          smc.primitiveType = MeshType::None; // Not using built-in primitive

          // Store imported materials in MaterialAssetLibrary
          for (size_t i = 0; i < result.materials.size(); ++i) {
            // Generate unique handle for each material
            std::string matName = result.materials[i]->GetName();
            std::filesystem::path matPath = p / (matName + ".ginimat");
            u64 matHandle = AssetRegistry::Get().RegisterAsset(matPath, AssetType::MaterialAsset);
            MaterialAssetLibrary::Get().Add(matHandle, result.materials[i]);
            smc.materialOverrides.push_back(matHandle);
            GINI_INFO("Registered material '", matName, "' with handle ", matHandle);
          }
        } else {
          // Fallback to cube if import fails
          smc.primitiveType = MeshType::Cube;
        }

        m_ActiveScene->GetWorld().AddComponent<MaterialComponent>(entity);

        m_SelectedEntity = entity;
        m_PropertiesPanel.SetSelectedEntity(entity);
        m_HierarchyPanel.SetSelectedEntity(entity);
      }
    }
    if (const ImGuiPayload *payload =
            ImGui::AcceptDragDropPayload(AssetBrowserPanel::PAYLOAD_ASSET)) {
      std::string assetPath(static_cast<const char *>(payload->Data));
      std::filesystem::path p(assetPath);
      std::string ext = p.extension().string();
      for (auto &c : ext) c = static_cast<char>(std::tolower(c));
      if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb" ||
          ext == ".dae") {
        if (m_ActiveScene) {
          std::string name = p.stem().string();
          auto entity = m_ActiveScene->CreateEntity(name);
          auto &smc =
              m_ActiveScene->GetWorld().AddComponent<StaticMeshComponent>(entity);

          // Import mesh using AssimpMeshImporter
          AssimpMeshImporter importer(assetPath);
          auto result = importer.Import();
          if (result.success && result.meshSource) {
            // Add to MeshSourceLibrary with a UUID-based handle
            u64 handle = AssetRegistry::Get().RegisterAsset(p, AssetType::MeshSource);
            MeshSourceLibrary::Get().Add(handle, result.meshSource);
            smc.meshSourceHandle = handle;
            smc.primitiveType = MeshType::None; // Not using built-in primitive

            // Store imported materials in MaterialAssetLibrary
            for (size_t i = 0; i < result.materials.size(); ++i) {
              // Generate unique handle for each material
              std::string matName = result.materials[i]->GetName();
              std::filesystem::path matPath = p / (matName + ".ginimat");
              u64 matHandle = AssetRegistry::Get().RegisterAsset(matPath, AssetType::MaterialAsset);
              MaterialAssetLibrary::Get().Add(matHandle, result.materials[i]);
              smc.materialOverrides.push_back(matHandle);
              GINI_INFO("Registered material '", matName, "' with handle ", matHandle);
            }
          } else {
            // Fallback to cube if import fails
            smc.primitiveType = MeshType::Cube;
          }

          m_ActiveScene->GetWorld().AddComponent<MaterialComponent>(entity);

          m_SelectedEntity = entity;
          m_PropertiesPanel.SetSelectedEntity(entity);
          m_HierarchyPanel.SetSelectedEntity(entity);
        }
      }
    }
    ImGui::EndDragDropTarget();
  }

  // Store viewport bounds for mouse picking (after image is placed)
  m_ViewportBounds[0] = {imagePos.x, imagePos.y};
  m_ViewportBounds[1] = {imagePos.x + m_ViewportSize.x,
                         imagePos.y + m_ViewportSize.y};

  // Viewport overlay (top-left: gizmo mode / camera info)
  {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 overlayPos(imagePos.x + 8, imagePos.y + 8);
    ImU32 bgCol = IM_COL32(0, 0, 0, 140);
    ImU32 textCol = IM_COL32(200, 200, 205, 220);

    const char* gizmoName = "Move";
    if (m_GizmoOperation == GizmoOperation::Rotate) gizmoName = "Rotate";
    else if (m_GizmoOperation == GizmoOperation::Scale) gizmoName = "Scale";

    Vec3 camPos = m_EditorCamera->GetPosition();
    char overlay[128];
    snprintf(overlay, sizeof(overlay), "%s | Cam: %.1f, %.1f, %.1f",
             gizmoName, camPos.x, camPos.y, camPos.z);

    ImVec2 textSize = ImGui::CalcTextSize(overlay);
    dl->AddRectFilled(overlayPos,
                      ImVec2(overlayPos.x + textSize.x + 12, overlayPos.y + textSize.y + 8),
                      bgCol, 4.0f);
    dl->AddText(ImVec2(overlayPos.x + 6, overlayPos.y + 4), textCol, overlay);
  }

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
  auto path = FileDialog::OpenFile({{"Gini Scene", "gscene"}});
  if (path.empty())
    return;

  auto newScene = CreateRef<Scene>();
  SceneSerializer serializer(newScene);
  if (serializer.Deserialize(path)) {
    m_ActiveScene = newScene;
    m_EditorScene = newScene;
    m_SelectedEntity = NullEntity;
    m_HierarchyPanel.SetScene(m_ActiveScene);
    m_PropertiesPanel.SetScene(m_ActiveScene);
    m_PropertiesPanel.SetSelectedEntity(NullEntity);
    m_HierarchyPanel.SetSelectedEntity(NullEntity);
    GINI_INFO("Opened scene: ", path);
  }
}

void EditorApp::SaveScene() {
  if (!m_ActiveScene)
    return;

  auto &filepath = m_ActiveScene->GetFilepath();
  if (filepath.empty()) {
    SaveSceneAs();
    return;
  }

  SceneSerializer serializer(m_ActiveScene);
  serializer.Serialize(filepath);
}

void EditorApp::SaveSceneAs() {
  auto path = FileDialog::SaveFile({{"Gini Scene", "gscene"}}, "scene.gscene");
  if (path.empty())
    return;

  SceneSerializer serializer(m_ActiveScene);
  serializer.Serialize(path);
  GINI_INFO("Scene saved as: ", path);
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