#include "TerrainEditorWindow.h"
#include "Core/Input.h"
#include "Core/Logger.h"
#include "Project/Project.h"
#include "Renderer/Material.h"
#include "Renderer/PostProcess.h"
#include "Renderer/Renderer3D.h"
#include "Utils/FileDialog.h"

#include <cstring>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <yaml-cpp/yaml.h>

namespace Gini {

TerrainEditorWindow::TerrainEditorWindow() {
  // Initialize default materials
  m_Materials[0] = {"Grass", "",      "",
                    nullptr, nullptr, Vec3(0.2f, 0.5f, 0.1f),
                    15.0f,   0.0f,    0.9f};
  m_Materials[1] = {"Dirt",  "",      "",
                    nullptr, nullptr, Vec3(0.4f, 0.3f, 0.2f),
                    15.0f,   0.0f,    0.95f};
  m_Materials[2] = {"Rock",  "",      "",
                    nullptr, nullptr, Vec3(0.4f, 0.4f, 0.4f),
                    10.0f,   0.0f,    0.85f};
  m_Materials[3] = {"Road",  "",      "",
                    nullptr, nullptr, Vec3(0.3f, 0.3f, 0.35f),
                    8.0f,    0.0f,    0.7f};
}

void TerrainEditorWindow::Open() {
  if (m_IsOpen)
    return;

  m_IsOpen = true;
  m_NeedsLayoutReset = true; // Reset layout every time editor is opened

  // Create HDR framebuffer for terrain rendering
  FramebufferSpec hdrSpec;
  hdrSpec.width = 800;
  hdrSpec.height = 600;
  hdrSpec.samples = 1;
  hdrSpec.colorAttachments = {{FramebufferTextureFormat::RGBA16F}};
  m_HDRFramebuffer = Framebuffer::Create(hdrSpec);

  // Create LDR framebuffer for ImGui display
  FramebufferSpec spec;
  spec.width = 800;
  spec.height = 600;
  spec.samples = 1;
  spec.colorAttachments = {{FramebufferTextureFormat::RGBA8}};
  m_Framebuffer = Framebuffer::Create(spec);

  // Create camera
  m_Camera = CreateScope<Camera3D>(45.0f, 800.0f / 600.0f, 0.1f, 1000.0f);
  m_Camera->SetPosition(Vec3(50.0f, 50.0f, 50.0f));
  m_Camera->LookAt(Vec3(0.0f, 0.0f, 0.0f));

  m_CameraController = CreateScope<OrbitCameraController>(m_Camera.get());
  m_CameraController->SetDistance(100.0f);

  // Create default terrain if none exists
  if (!m_Terrain) {
    CreateNewTerrain();
  }

  GINI_INFO("Terrain Editor opened");
}

void TerrainEditorWindow::Close() {
  m_IsOpen = false;
  GINI_INFO("Terrain Editor closed");
}

void TerrainEditorWindow::CreateNewTerrain() {
  m_Terrain = Terrain::Create(m_TerrainWidth, m_TerrainHeight, m_TerrainScale);
  m_Terrain->SetHeightScale(m_TerrainMaxHeight);
  m_Terrain->GenerateFlat();

  // Add 4 layers with materials
  for (u32 i = 0; i < 4; i++) {
    TerrainLayer layer;
    layer.name = m_Materials[i].name;
    layer.color = m_Materials[i].fallbackColor;
    layer.tiling = Vec2(m_Materials[i].tiling);
    layer.albedoMap = m_Materials[i].albedoTexture;
    layer.normalMap = m_Materials[i].normalTexture;
    m_Terrain->AddLayer(layer);
  }

  GINI_INFO("Created new terrain {}x{} with height scale {}", m_TerrainWidth,
            m_TerrainHeight, m_TerrainMaxHeight);
}

void TerrainEditorWindow::OnUpdate(f32 deltaTime) {
  if (!m_IsOpen)
    return;

  m_DeltaTime = deltaTime;
  m_CameraController->OnUpdate(deltaTime);
  // HandlePainting is called in DrawViewport after viewport bounds are set
}

void TerrainEditorWindow::OnImGuiRender() {
  if (!m_IsOpen) {
    return;
  }

  // Make Terrain Editor fullscreen
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  ImGuiWindowFlags mainWindowFlags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove;

  if (ImGui::Begin("Terrain Editor", &m_IsOpen, mainWindowFlags)) {
    ImGui::PopStyleVar(); // Pop WindowPadding

    // Menu bar
    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Terrain")) {
          CreateNewTerrain();
        }
        if (ImGui::MenuItem("Open Terrain...")) {
          OpenTerrain();
        }
        if (ImGui::MenuItem("Save Terrain")) {
          SaveTerrain();
        }
        if (ImGui::MenuItem("Update Terrain")) {
          UpdateTerrain();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Export Terrain...")) {
          ExportTerrain();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Close")) {
          Close();
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Generate")) {
        if (ImGui::MenuItem("Flat")) {
          if (m_Terrain)
            m_Terrain->GenerateFlat();
        }
        if (ImGui::MenuItem("Noise")) {
          if (m_Terrain)
            m_Terrain->GenerateFromNoise(0.02f, 30.0f, 4);
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    }

    // Use child windows for a fixed layout (no docking issues)
    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    float leftPanelWidth = 220.0f;
    float rightPanelWidth = 260.0f;
    float viewportWidth = contentSize.x - leftPanelWidth - rightPanelWidth;

    // Left panel - Tools
    ImGui::BeginChild("##ToolsPanel", ImVec2(leftPanelWidth, contentSize.y),
                      true);
    ImGui::Text("TERRAIN TOOLS");
    ImGui::Separator();
    DrawToolbar();
    ImGui::Separator();
    DrawBrushPanel();
    ImGui::EndChild();

    ImGui::SameLine();

    // Center - Viewport
    ImGui::BeginChild(
        "##ViewportPanel", ImVec2(viewportWidth, contentSize.y), true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    DrawViewport();
    ImGui::EndChild();

    ImGui::SameLine();

    // Right panel - Layers and Settings
    ImGui::BeginChild("##LayersPanel", ImVec2(rightPanelWidth, contentSize.y),
                      true);
    DrawLayersPanel();
    ImGui::Separator();
    DrawSettingsPanel();
    ImGui::EndChild();
  } else {
    ImGui::PopStyleVar(); // Pop WindowPadding if Begin failed
  }
  ImGui::End();
}

void TerrainEditorWindow::DrawToolbar() {
  ImGui::Text("Mode:");
  if (ImGui::RadioButton("Sculpt", m_BrushMode == BrushMode::Sculpt)) {
    m_BrushMode = BrushMode::Sculpt;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Paint", m_BrushMode == BrushMode::Paint)) {
    m_BrushMode = BrushMode::Paint;
  }

  ImGui::Separator();

  if (m_BrushMode == BrushMode::Sculpt) {
    ImGui::Text("Sculpt Mode:");
    if (ImGui::RadioButton("Raise", m_SculptMode == SculptMode::Raise)) {
      m_SculptMode = SculptMode::Raise;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Lower", m_SculptMode == SculptMode::Lower)) {
      m_SculptMode = SculptMode::Lower;
    }
    if (ImGui::RadioButton("Smooth", m_SculptMode == SculptMode::Smooth)) {
      m_SculptMode = SculptMode::Smooth;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Flatten", m_SculptMode == SculptMode::Flatten)) {
      m_SculptMode = SculptMode::Flatten;
    }
  } else {
    ImGui::Text("Paint Layer:");
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s",
                       m_Materials[m_SelectedLayer].name.c_str());
  }
}

void TerrainEditorWindow::DrawViewport() {
  ImGui::Text("TERRAIN VIEWPORT");
  ImGui::Separator();

  m_ViewportFocused = ImGui::IsWindowFocused();
  m_ViewportHovered = ImGui::IsWindowHovered();

  ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

  // Ensure minimum size
  if (viewportPanelSize.x < 100 || viewportPanelSize.y < 100) {
    ImGui::Text("Viewport too small");
    return;
  }

  if (m_ViewportSize.x != viewportPanelSize.x ||
      m_ViewportSize.y != viewportPanelSize.y) {
    m_ViewportSize = Vec2(viewportPanelSize.x, viewportPanelSize.y);
    if (m_ViewportSize.x > 0 && m_ViewportSize.y > 0) {
      m_HDRFramebuffer->Resize(static_cast<u32>(m_ViewportSize.x),
                               static_cast<u32>(m_ViewportSize.y));
      m_Framebuffer->Resize(static_cast<u32>(m_ViewportSize.x),
                            static_cast<u32>(m_ViewportSize.y));
      m_Camera->SetAspectRatio(m_ViewportSize.x / m_ViewportSize.y);
    }
  }

  // Skip rendering if framebuffer is not valid
  if (m_ViewportSize.x <= 0 || m_ViewportSize.y <= 0) {
    return;
  }

  // Render terrain to HDR framebuffer
  m_HDRFramebuffer->Bind();
  Renderer3D::SetViewport(0, 0, static_cast<u32>(m_ViewportSize.x),
                          static_cast<u32>(m_ViewportSize.y));
  Renderer3D::SetClearColor(Color(0.1f, 0.15f, 0.2f));
  Renderer3D::Clear();

  Renderer3D::BeginScene(*m_Camera);

  // Draw terrain
  if (m_Terrain) {
    m_Terrain->Render(*m_Camera);

    // Draw brush indicator
    if (m_IsHit) {
      Color brushColor = (m_BrushMode == BrushMode::Sculpt)
                             ? Color(1.0f, 0.5f, 0.0f)
                             : Color(0.0f, 1.0f, 0.5f);

      // Draw brush circle
      const int segments = 32;
      for (int i = 0; i < segments; i++) {
        float angle1 = (float)i / segments * 2.0f * 3.14159f;
        float angle2 = (float)(i + 1) / segments * 2.0f * 3.14159f;

        Vec3 p1 = m_HitPoint + Vec3(cos(angle1) * m_BrushRadius, 0.1f,
                                    sin(angle1) * m_BrushRadius);
        Vec3 p2 = m_HitPoint + Vec3(cos(angle2) * m_BrushRadius, 0.1f,
                                    sin(angle2) * m_BrushRadius);

        // Adjust Y to terrain height
        p1.y = m_Terrain->GetHeightAtPosition(p1.x, p1.z) + 0.2f;
        p2.y = m_Terrain->GetHeightAtPosition(p2.x, p2.z) + 0.2f;

        Renderer3D::DrawLine(p1, p2, brushColor);
      }
    }
  }

  // Draw grid
  for (int i = -50; i <= 50; i += 10) {
    Color gridColor(0.3f, 0.3f, 0.3f);
    Renderer3D::DrawLine(Vec3(i, 0, -50), Vec3(i, 0, 50), gridColor);
    Renderer3D::DrawLine(Vec3(-50, 0, i), Vec3(50, 0, i), gridColor);
  }

  Renderer3D::EndScene();
  m_HDRFramebuffer->Unbind();

  // Resolve HDR to LDR with tonemapping
  m_Framebuffer->Bind();
  Renderer3D::SetViewport(0, 0, static_cast<u32>(m_ViewportSize.x),
                          static_cast<u32>(m_ViewportSize.y));
  PostProcess::Resolve(m_HDRFramebuffer->GetColorAttachment(0), 0, 1.0f, 2.2f);
  m_Framebuffer->Unbind();

  // Display framebuffer texture
  u32 textureID = m_Framebuffer->GetColorAttachment();
  ImVec2 imagePos = ImGui::GetCursorScreenPos();
  ImGui::Image((ImTextureID)(intptr_t)textureID, viewportPanelSize,
               ImVec2(0, 1), ImVec2(1, 0));

  // Store viewport bounds for mouse picking (after image is placed)
  m_ViewportBounds[0] = Vec2(imagePos.x, imagePos.y);
  m_ViewportBounds[1] =
      Vec2(imagePos.x + viewportPanelSize.x, imagePos.y + viewportPanelSize.y);

  // Handle painting now that viewport bounds are set correctly
  HandlePainting(m_DeltaTime);

  // Handle camera controls
  if (m_ViewportHovered || m_ViewportFocused) {
    auto &input = Input::Get();

    if (input.IsMouseButtonDown(MouseButton::Right)) {
      // Mouse look
      Vec2 delta = input.GetMouseDelta();
      m_CameraController->OnMouseMove(delta.x, delta.y, true, false);

      // WASD movement while right-click is held
      Vec3 forward = m_Camera->GetForward();
      Vec3 right = m_Camera->GetRight();
      Vec3 movement(0.0f);
      float moveSpeed = 50.0f;

      if (input.IsKeyDown(Key::W)) {
        movement += forward;
      }
      if (input.IsKeyDown(Key::S)) {
        movement -= forward;
      }
      if (input.IsKeyDown(Key::A)) {
        movement -= right;
      }
      if (input.IsKeyDown(Key::D)) {
        movement += right;
      }
      if (input.IsKeyDown(Key::E) || input.IsKeyDown(Key::Space)) {
        movement.y += 1.0f;
      }
      if (input.IsKeyDown(Key::Q) || input.IsKeyDown(Key::LeftControl)) {
        movement.y -= 1.0f;
      }

      // Shift for faster movement
      if (input.IsKeyDown(Key::LeftShift)) {
        moveSpeed *= 2.5f;
      }

      if (glm::length(movement) > 0.0f) {
        movement = glm::normalize(movement) * moveSpeed *
                   0.016f; // Approximate delta time
        Vec3 newPos = m_Camera->GetPosition() + movement;
        m_Camera->SetPosition(newPos);
        m_CameraController->SetTarget(
            newPos + forward * m_CameraController->GetDistance());
      }
    }

    if (input.IsMouseButtonDown(MouseButton::Middle)) {
      Vec2 delta = input.GetMouseDelta();
      m_CameraController->OnMouseMove(delta.x, delta.y, false, true);
    }
  }
}

void TerrainEditorWindow::DrawLayersPanel() {
  ImGui::Text("MATERIAL LAYERS");
  ImGui::Separator();
  ImGui::Text("Terrain Layers (4 max):");
  ImGui::Separator();

  for (u32 i = 0; i < 4; i++) {
    ImGui::PushID(i);

    bool selected = (m_SelectedLayer == i);

    // Color preview
    ImVec4 col(m_Materials[i].fallbackColor.x, m_Materials[i].fallbackColor.y,
               m_Materials[i].fallbackColor.z, 1.0f);

    // Texture preview or color
    if (m_Materials[i].albedoTexture) {
      u32 texId = m_Materials[i].albedoTexture->GetID();
      if (ImGui::ImageButton(("##tex" + std::to_string(i)).c_str(),
                             (ImTextureID)(intptr_t)texId, ImVec2(48, 48))) {
        m_SelectedLayer = i;
      }
    } else {
      if (ImGui::ColorButton("##preview", col, ImGuiColorEditFlags_NoTooltip,
                             ImVec2(48, 48))) {
        m_SelectedLayer = i;
      }
    }

    ImGui::SameLine();

    ImGui::BeginGroup();
    if (ImGui::Selectable(m_Materials[i].name.c_str(), selected, 0,
                          ImVec2(150, 0))) {
      m_SelectedLayer = i;
    }

    if (m_Materials[i].albedoTexture) {
      ImGui::TextDisabled("Texture loaded");
    } else {
      ImGui::TextDisabled("Using color");
    }
    ImGui::EndGroup();

    ImGui::PopID();
  }

  ImGui::Separator();

  // Edit selected layer
  if (m_SelectedLayer < 4) {
    TerrainMaterial &mat = m_Materials[m_SelectedLayer];

    ImGui::Text("Edit Layer %d:", m_SelectedLayer);

    char nameBuf[64];
    strncpy(nameBuf, mat.name.c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
      mat.name = nameBuf;
    }

    float color[3] = {mat.fallbackColor.x, mat.fallbackColor.y,
                      mat.fallbackColor.z};
    if (ImGui::ColorEdit3("Fallback Color", color)) {
      mat.fallbackColor = Vec3(color[0], color[1], color[2]);
      // Update terrain layer
      if (m_Terrain && m_SelectedLayer < m_Terrain->GetLayerCount()) {
        m_Terrain->GetLayer(m_SelectedLayer).color = mat.fallbackColor;
      }
    }

    if (ImGui::DragFloat("Tiling", &mat.tiling, 0.5f, 1.0f, 100.0f)) {
      if (m_Terrain && m_SelectedLayer < m_Terrain->GetLayerCount()) {
        m_Terrain->GetLayer(m_SelectedLayer).tiling = Vec2(mat.tiling);
      }
    }

    ImGui::Separator();

    // Texture loading
    ImGui::Text("Albedo: %s",
                mat.albedoPath.empty() ? "(none)" : mat.albedoPath.c_str());
    if (ImGui::Button("Load Albedo Texture", ImVec2(-1, 0))) {
      LoadTextureForLayer(m_SelectedLayer);
    }

    if (mat.albedoTexture) {
      if (ImGui::Button("Clear Texture", ImVec2(-1, 0))) {
        mat.albedoTexture = nullptr;
        mat.albedoPath = "";
        if (m_Terrain && m_SelectedLayer < m_Terrain->GetLayerCount()) {
          m_Terrain->GetLayer(m_SelectedLayer).albedoMap = nullptr;
        }
      }
    }

    ImGui::Separator();
    ImGui::Text("Load Material:");

    // Load .gmat material file
    if (ImGui::Button("Load Material File (.gmat)", ImVec2(-1, 0))) {
      std::vector<FileDialogFilter> filters = {{"Gini Material", "gmat"}};

      std::string filepath = FileDialog::OpenFile(filters);

      if (!filepath.empty()) {
        LoadGmatMaterial(m_SelectedLayer, filepath);
      }
    }

    // Drag-drop target for materials
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload *payload =
              ImGui::AcceptDragDropPayload("ASSET_MATERIAL")) {
        const char *path = (const char *)payload->Data;
        LoadGmatMaterial(m_SelectedLayer, path);
      }
      ImGui::EndDragDropTarget();
    }

    // Quick load from broken_down_concrete folder
    if (ImGui::Button("Load Concrete Material (Legacy)", ImVec2(-1, 0))) {
      // Get material path from active project or fallback to default
      std::filesystem::path materialPath;
      auto activeProject = Project::GetActive();
      if (activeProject) {
        materialPath = activeProject->GetConfig().assetsPath / "textures" /
                       "terrain" / "broken_down_concrete";
      } else {
        // Fallback for when no project is loaded (use editor assets)
        materialPath =
            "../../Editor/assets/textures/terrain/broken_down_concrete";
      }
      Ref<Material> pbrMat = Material::LoadFromDirectory(materialPath);
      if (pbrMat) {
        mat.name = pbrMat->GetName();
        mat.albedoTexture = pbrMat->GetAlbedoTexture();
        mat.normalTexture = pbrMat->GetNormalTexture();
        mat.albedoPath = materialPath.string();
        mat.roughness = pbrMat->GetRoughness();
        mat.metallic = pbrMat->GetMetallic();

        // Update terrain layer
        if (m_Terrain && m_SelectedLayer < m_Terrain->GetLayerCount()) {
          auto &layer = m_Terrain->GetLayer(m_SelectedLayer);
          layer.albedoMap = pbrMat->GetAlbedoTexture();
          layer.normalMap = pbrMat->GetNormalTexture();
          layer.roughness = pbrMat->GetRoughness();
          layer.metallic = pbrMat->GetMetallic();
        }
        GINI_INFO("Loaded PBR material: ", mat.name);
      }
    }
  }
}

void TerrainEditorWindow::DrawBrushPanel() {
  ImGui::Text("BRUSH SETTINGS");
  ImGui::Separator();

  ImGui::DragFloat("Radius", &m_BrushRadius, 0.5f, 0.5f, 50.0f);
  ImGui::DragFloat("Strength", &m_BrushStrength, 0.01f, 0.01f, 1.0f);
  ImGui::DragFloat("Falloff", &m_BrushFalloff, 0.05f, 0.0f, 1.0f);

  // Brush preview
  ImGui::Separator();
  ImGui::Text("Brush Preview:");

  ImVec2 canvasSize(120, 120);
  ImVec2 canvasPos = ImGui::GetCursorScreenPos();
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  drawList->AddRectFilled(
      canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
      IM_COL32(40, 40, 40, 255));

  ImVec2 center(canvasPos.x + canvasSize.x * 0.5f,
                canvasPos.y + canvasSize.y * 0.5f);
  float maxRadius = canvasSize.x * 0.4f;
  float innerRadius = maxRadius * (1.0f - m_BrushFalloff);

  ImU32 brushCol = (m_BrushMode == BrushMode::Sculpt)
                       ? IM_COL32(255, 150, 50, 150)
                       : IM_COL32(50, 255, 150, 150);
  ImU32 brushColInner = (m_BrushMode == BrushMode::Sculpt)
                            ? IM_COL32(255, 150, 50, 220)
                            : IM_COL32(50, 255, 150, 220);

  drawList->AddCircleFilled(center, maxRadius, brushCol);
  drawList->AddCircleFilled(center, innerRadius, brushColInner);
  drawList->AddCircle(center, maxRadius, IM_COL32(255, 255, 255, 200), 32,
                      2.0f);

  ImGui::Dummy(canvasSize);
}

void TerrainEditorWindow::DrawSettingsPanel() {
  ImGui::Text("TERRAIN SETTINGS");
  ImGui::Separator();

  ImGui::Text("New Terrain Settings:");
  ImGui::DragInt("Width", &m_TerrainWidth, 1, 64, 1024);
  ImGui::DragInt("Height", &m_TerrainHeight, 1, 64, 1024);
  ImGui::DragFloat("Scale", &m_TerrainScale, 0.1f, 0.1f, 10.0f);
  ImGui::DragFloat("Max Height", &m_TerrainMaxHeight, 1.0f, 1.0f, 200.0f);

  if (ImGui::Button("Create New Terrain", ImVec2(-1, 0))) {
    CreateNewTerrain();
  }

  ImGui::Separator();

  if (m_Terrain) {
    ImGui::Text("Current Terrain:");
    ImGui::Text("Size: %dx%d", m_Terrain->GetWidth(), m_Terrain->GetHeight());
    ImGui::Text("Scale: %.2f", m_Terrain->GetScale());

    float heightScale = m_Terrain->GetHeightScale();
    if (ImGui::DragFloat("Height Scale", &heightScale, 0.1f, 0.1f, 10.0f)) {
      m_Terrain->SetHeightScale(heightScale);
    }
  }

  ImGui::Separator();

  if (ImGui::Button("Save Terrain", ImVec2(-1, 0))) {
    SaveTerrain();
  }

  if (ImGui::Button("Export as OBJ", ImVec2(-1, 0))) {
    ExportTerrain();
  }
}

void TerrainEditorWindow::HandlePainting(f32 deltaTime) {
  if (!m_Terrain || !m_ViewportHovered) {
    m_IsHit = false;
    return;
  }

  auto &input = Input::Get();
  Vec2 mousePos = input.GetMousePosition();

  Vec3 rayOrigin = m_Camera->GetPosition();
  Vec3 rayDir = ScreenToWorldRay(mousePos);

  m_IsHit = m_Terrain->Raycast(rayOrigin, rayDir, m_HitPoint);

  // Paint on left click (not when using camera)
  bool isLeftDown = input.IsMouseButtonDown(MouseButton::Left);
  bool isRightDown = input.IsMouseButtonDown(MouseButton::Right);
  bool isMiddleDown = input.IsMouseButtonDown(MouseButton::Middle);

  if (isLeftDown && !isRightDown && !isMiddleDown && m_IsHit) {
    f32 strength = m_BrushStrength * deltaTime * 10.0f;

    if (m_BrushMode == BrushMode::Sculpt) {
      switch (m_SculptMode) {
      case SculptMode::Raise:
        m_Terrain->PaintHeight(m_HitPoint.x, m_HitPoint.z, m_BrushRadius,
                               strength, true);
        break;
      case SculptMode::Lower:
        m_Terrain->PaintHeight(m_HitPoint.x, m_HitPoint.z, m_BrushRadius,
                               strength, false);
        break;
      case SculptMode::Smooth:
        m_Terrain->SmoothHeight(m_HitPoint.x, m_HitPoint.z, m_BrushRadius,
                                strength);
        break;
      case SculptMode::Flatten:
        m_Terrain->FlattenHeight(m_HitPoint.x, m_HitPoint.z, m_BrushRadius,
                                 m_HitPoint.y);
        break;
      }
    } else {
      // Paint material
      m_Terrain->PaintMaterial(m_HitPoint.x, m_HitPoint.z, m_BrushRadius,
                               strength, m_SelectedLayer);
    }
  }
}

Vec3 TerrainEditorWindow::ScreenToWorldRay(const Vec2 &screenPos) {
  Vec2 viewportPos =
      screenPos - Vec2(m_ViewportBounds[0].x, m_ViewportBounds[0].y);
  Vec2 ndc;
  ndc.x = (2.0f * viewportPos.x) / m_ViewportSize.x - 1.0f;
  ndc.y = 1.0f - (2.0f * viewportPos.y) / m_ViewportSize.y;

  Vec4 rayClip(ndc.x, ndc.y, -1.0f, 1.0f);

  Mat4 invProj = glm::inverse(m_Camera->GetProjectionMatrix());
  Vec4 rayEye = invProj * rayClip;
  rayEye = Vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

  Mat4 invView = glm::inverse(m_Camera->GetViewMatrix());
  Vec4 rayWorld = invView * rayEye;

  return glm::normalize(Vec3(rayWorld));
}

void TerrainEditorWindow::LoadTextureForLayer(u32 layerIndex) {
  // For now, show a popup with instructions
  // In a real implementation, you'd use a file dialog
  GINI_INFO("To load a texture, place it in Editor/assets/textures/terrain/");
  GINI_INFO("Supported formats: PNG, JPG, TGA");

  // Try to load a default texture based on layer name
  std::string basePath = "assets/textures/terrain/";
  std::string textureName = m_Materials[layerIndex].name + ".png";
  std::string fullPath = basePath + textureName;

  // Try to load the texture
  m_Materials[layerIndex].albedoTexture = Texture2D::Create(fullPath);
  if (m_Materials[layerIndex].albedoTexture) {
    m_Materials[layerIndex].albedoPath = fullPath;

    // Update terrain layer
    if (m_Terrain && layerIndex < m_Terrain->GetLayerCount()) {
      m_Terrain->GetLayer(layerIndex).albedoMap =
          m_Materials[layerIndex].albedoTexture;
    }

    GINI_INFO("Loaded texture: {}", fullPath);
  } else {
    GINI_WARN("Could not load texture: {}", fullPath);
  }
}

void TerrainEditorWindow::LoadGmatMaterial(u32 layerIndex,
                                           const std::string &filepath) {
  if (layerIndex >= 4)
    return;

  try {
    YAML::Node data = YAML::LoadFile(filepath);
    if (!data["Material"]) {
      GINI_ERROR("Invalid .gmat file: ", filepath);
      return;
    }

    auto material = data["Material"];
    TerrainMaterial &mat = m_Materials[layerIndex];

    // Load material name
    if (material["Name"]) {
      mat.name = material["Name"].as<std::string>();
    }

    // Load albedo color as fallback
    if (material["Albedo"]) {
      auto albedo = material["Albedo"];
      if (albedo.IsSequence() && albedo.size() >= 3) {
        mat.fallbackColor = Vec3(albedo[0].as<float>(), albedo[1].as<float>(),
                                 albedo[2].as<float>());
      }
    }

    // Load roughness and metallic
    if (material["Roughness"]) {
      mat.roughness = material["Roughness"].as<float>();
    }
    if (material["Metallic"]) {
      mat.metallic = material["Metallic"].as<float>();
    }

    // Load albedo texture
    if (material["AlbedoTexture"]) {
      std::string texturePath = material["AlbedoTexture"].as<std::string>();
      if (std::filesystem::exists(texturePath)) {
        mat.albedoTexture = Texture2D::Create(texturePath);
        mat.albedoPath = texturePath;
      }
    }

    // Load normal texture
    if (material["NormalTexture"]) {
      std::string texturePath = material["NormalTexture"].as<std::string>();
      if (std::filesystem::exists(texturePath)) {
        mat.normalTexture = Texture2D::Create(texturePath);
        mat.normalPath = texturePath;
      }
    }

    // Update terrain layer
    if (m_Terrain && layerIndex < m_Terrain->GetLayerCount()) {
      auto &layer = m_Terrain->GetLayer(layerIndex);
      layer.color = mat.fallbackColor;
      layer.albedoMap = mat.albedoTexture;
      layer.normalMap = mat.normalTexture;
      layer.roughness = mat.roughness;
      layer.metallic = mat.metallic;
      layer.tiling = Vec2(mat.tiling);
    }

    GINI_INFO("Loaded .gmat material: ", mat.name, " for layer ", layerIndex);

  } catch (const std::exception &e) {
    GINI_ERROR("Failed to load .gmat material: ", e.what());
  }
}

void TerrainEditorWindow::SaveTerrain() {
  if (!m_Terrain)
    return;

  std::vector<FileDialogFilter> filters = {{"Gini Terrain", "gterrain"}};

  std::string filepath = FileDialog::SaveFile(filters);
  if (filepath.empty())
    return;

  // Ensure .gterrain extension
  if (filepath.find(".gterrain") == std::string::npos) {
    filepath += ".gterrain";
  }

  m_Terrain->SaveTerrain(filepath);
  m_CurrentFilePath = filepath;
}

void TerrainEditorWindow::ExportTerrain() {
  if (!m_Terrain)
    return;

  // Use folder dialog - user selects export folder
  std::vector<FileDialogFilter> filters = {};
  std::string filepath = FileDialog::SaveFile(filters, "terrain_export");
  if (filepath.empty())
    return;

  // Extract folder and terrain name from path
  std::filesystem::path path(filepath);
  std::string exportFolder = path.parent_path().string();
  std::string terrainName = path.stem().string();

  if (terrainName.empty()) {
    terrainName = "terrain";
  }

  // Unified export - creates all files in the folder
  m_Terrain->ExportTerrain(exportFolder, terrainName);

  // Store the path for future updates
  m_CurrentFilePath = exportFolder + "/" + terrainName + ".gterrain";
  GINI_INFO("Terrain exported. Use 'Update Terrain' to save changes to: {}",
            m_CurrentFilePath);
}

void TerrainEditorWindow::OpenTerrain() {
  std::vector<FileDialogFilter> filters = {{"Gini Terrain", "gterrain"}};
  std::string filepath = FileDialog::OpenFile(filters);
  if (filepath.empty())
    return;

  // Create new terrain and load from file
  m_Terrain = Terrain::Create();
  m_Terrain->LoadTerrain(filepath);
  m_CurrentFilePath = filepath;

  // Update editor materials from loaded terrain layers
  for (u32 i = 0; i < m_Terrain->GetLayerCount() && i < 4; i++) {
    TerrainLayer &layer = m_Terrain->GetLayer(i);
    m_Materials[i].name = layer.name;
    m_Materials[i].fallbackColor = layer.color;
    m_Materials[i].tiling = layer.tiling.x;
    m_Materials[i].roughness = layer.roughness;
    m_Materials[i].metallic = layer.metallic;
    m_Materials[i].albedoTexture = layer.albedoMap;
    m_Materials[i].normalTexture = layer.normalMap;
    if (layer.albedoMap) {
      m_Materials[i].albedoPath = layer.albedoMap->GetPath();
    }
    if (layer.normalMap) {
      m_Materials[i].normalPath = layer.normalMap->GetPath();
    }
  }

  // Update terrain settings from loaded data
  m_TerrainWidth = m_Terrain->GetWidth();
  m_TerrainHeight = m_Terrain->GetHeight();
  m_TerrainScale = m_Terrain->GetScale();
  m_TerrainMaxHeight = m_Terrain->GetHeightScale();

  GINI_INFO("Opened terrain: {}", filepath);
}

void TerrainEditorWindow::UpdateTerrain() {
  if (!m_Terrain)
    return;

  if (m_CurrentFilePath.empty()) {
    GINI_WARN("No terrain file loaded. Use 'Export Terrain' first or 'Open "
              "Terrain' to load an existing terrain.");
    return;
  }

  // Extract folder and terrain name from current path
  std::filesystem::path path(m_CurrentFilePath);
  std::string exportFolder = path.parent_path().string();
  std::string terrainName = path.stem().string();

  // Re-export all files to update them
  m_Terrain->ExportTerrain(exportFolder, terrainName);

  GINI_INFO("Terrain updated: {}", m_CurrentFilePath);
}

} // namespace Gini
