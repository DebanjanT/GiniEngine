#include "MaterialEditorPanel.h"
#include "Core/Logger.h"
#include "Project/Project.h"
#include "Renderer/Renderer3D.h"
#include "Utils/FileDialog.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <glad/gl.h>
#include <imgui.h>
#include <yaml-cpp/yaml.h>

namespace Gini {

MaterialEditorPanel::MaterialEditorPanel() : EditorPanel("Material Editor") {

  // Create preview framebuffer
  FramebufferSpec spec;
  spec.width = 256;
  spec.height = 256;
  spec.samples = 1;
  m_PreviewFramebuffer = Framebuffer::Create(spec);

  // Create preview camera
  m_PreviewCamera = CreateScope<Camera3D>(45.0f, 1.0f, 0.1f, 100.0f);
  m_PreviewCamera->SetPosition(Vec3(0, 0, 3));
  m_PreviewCamera->LookAt(Vec3(0, 0, 0));
}

void MaterialEditorPanel::OnImGuiRender() {
  if (!m_Visible)
    return;

  // Fullscreen window like Terrain Editor
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  ImGuiWindowFlags mainWindowFlags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove;

  // Build window title with material name
  std::string windowTitle = "Material Editor";
  if (m_Material) {
    windowTitle += " - " + m_Material->GetName();
    if (m_IsDirty) {
      windowTitle += "*";
    }
  }

  if (ImGui::Begin(windowTitle.c_str(), &m_Visible, mainWindowFlags)) {
    ImGui::PopStyleVar(); // Pop WindowPadding

    DrawMenuBar();

    // Main layout: Palette | Node Graph | Inspector + Preview
    float paletteWidth = m_ShowPalette ? 180.0f : 0.0f;
    float inspectorWidth = m_ShowInspector ? 280.0f : 0.0f;

    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    float graphWidth = contentSize.x - paletteWidth - inspectorWidth;

    // Left: Node Palette
    if (m_ShowPalette) {
      ImGui::BeginChild("NodePalette", ImVec2(paletteWidth, contentSize.y),
                        true);
      DrawNodePalette();
      ImGui::EndChild();
      ImGui::SameLine();
    }

    // Center: Node Graph
    ImGui::BeginChild("NodeGraph", ImVec2(graphWidth, contentSize.y), true,
                      ImGuiWindowFlags_NoScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);
    DrawNodeGraph();
    ImGui::EndChild();

    ImGui::SameLine();

    // Right: Inspector + Preview
    if (m_ShowInspector) {
      ImGui::BeginChild("InspectorArea", ImVec2(inspectorWidth, contentSize.y),
                        true);

      // Preview at top
      if (m_ShowPreview) {
        ImGui::Text("Preview");
        ImGui::Separator();
        DrawPreview();
        ImGui::Separator();
      }

      // Inspector below
      ImGui::Text("Inspector");
      ImGui::Separator();
      DrawNodeInspector();

      ImGui::EndChild();
    }
  } else {
    ImGui::PopStyleVar(); // Pop WindowPadding if Begin failed
  }
  ImGui::End();
}

void MaterialEditorPanel::DrawMenuBar() {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Material")) {
        NewMaterial();
      }
      if (ImGui::MenuItem("Open Material...", "Ctrl+O")) {
        std::vector<FileDialogFilter> filters = {{"Gini Material", "gmat"}};
        std::string filepath = FileDialog::OpenFile(filters);
        if (!filepath.empty()) {
          LoadMaterialFromFile(filepath);
        }
      }
      if (ImGui::MenuItem("Save", "Ctrl+S")) {
        SaveMaterial();
      }
      if (ImGui::MenuItem("Save As...")) {
        std::vector<FileDialogFilter> filters = {{"Gini Material", "gmat"}};
        std::string filepath = FileDialog::SaveFile(filters);
        if (!filepath.empty()) {
          if (filepath.find(".gmat") == std::string::npos) {
            filepath += ".gmat";
          }
          m_MaterialPath = filepath;
          SaveMaterial();
        }
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Close")) {
        m_Visible = false;
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Compile", "F5")) {
        CompileMaterial();
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Node Palette", nullptr, &m_ShowPalette);
      ImGui::MenuItem("Inspector", nullptr, &m_ShowInspector);
      ImGui::MenuItem("Preview", nullptr, &m_ShowPreview);
      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }
}

void MaterialEditorPanel::DrawNodePalette() {
  ImGui::Text("NODES");
  ImGui::Separator();

  // Helper to get random spawn position
  auto getSpawnPos = [this]() {
    static int spawnCount = 0;
    spawnCount++;
    // Offset each new node so they don't overlap
    float x = 100.0f + (spawnCount % 5) * 180.0f;
    float y = 100.0f + (spawnCount / 5) * 150.0f;
    return Vec2(x - m_ViewOffset.x, y - m_ViewOffset.y);
  };

  // Node categories
  if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::Selectable("Texture Sample")) {
      CreateNode(MaterialNodeType::TextureSample, getSpawnPos());
    }
    if (ImGui::Selectable("Tex Coord")) {
      CreateNode(MaterialNodeType::TexCoord, getSpawnPos());
    }
    if (ImGui::Selectable("Panner")) {
      CreateNode(MaterialNodeType::Panner, getSpawnPos());
    }
  }

  if (ImGui::CollapsingHeader("Constants", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::Selectable("Constant")) {
      CreateNode(MaterialNodeType::Constant, getSpawnPos());
    }
    if (ImGui::Selectable("Time")) {
      CreateNode(MaterialNodeType::Time, getSpawnPos());
    }
  }

  if (ImGui::CollapsingHeader("Math", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::Selectable("Add")) {
      CreateNode(MaterialNodeType::Add, getSpawnPos());
    }
    if (ImGui::Selectable("Multiply")) {
      CreateNode(MaterialNodeType::Multiply, getSpawnPos());
    }
    if (ImGui::Selectable("Lerp")) {
      CreateNode(MaterialNodeType::Lerp, getSpawnPos());
    }
    if (ImGui::Selectable("Power")) {
      CreateNode(MaterialNodeType::Power, getSpawnPos());
    }
    if (ImGui::Selectable("Clamp")) {
      CreateNode(MaterialNodeType::Clamp, getSpawnPos());
    }
    if (ImGui::Selectable("One Minus")) {
      CreateNode(MaterialNodeType::OneMinus, getSpawnPos());
    }
  }

  if (ImGui::CollapsingHeader("Utility", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::Selectable("Fresnel")) {
      CreateNode(MaterialNodeType::Fresnel, getSpawnPos());
    }
    if (ImGui::Selectable("Desaturate")) {
      CreateNode(MaterialNodeType::Desaturate, getSpawnPos());
    }
    if (ImGui::Selectable("Normal")) {
      CreateNode(MaterialNodeType::Normal, getSpawnPos());
    }
  }
}

void MaterialEditorPanel::DrawNodeGraph() {
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  ImVec2 canvasPos = ImGui::GetCursorScreenPos();
  ImVec2 canvasSize = ImGui::GetContentRegionAvail();

  // Draw grid background
  ImU32 gridColor = IM_COL32(50, 50, 50, 255);
  ImU32 gridColorLight = IM_COL32(70, 70, 70, 255);
  float gridStep = 32.0f * m_ViewZoom;

  for (float x = fmodf(m_ViewOffset.x, gridStep); x < canvasSize.x;
       x += gridStep) {
    drawList->AddLine(ImVec2(canvasPos.x + x, canvasPos.y),
                      ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y),
                      gridColor);
  }
  for (float y = fmodf(m_ViewOffset.y, gridStep); y < canvasSize.y;
       y += gridStep) {
    drawList->AddLine(ImVec2(canvasPos.x, canvasPos.y + y),
                      ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + y),
                      gridColor);
  }

  // Draw connections
  DrawConnections();

  // Draw nodes
  for (auto &node : m_Nodes) {
    DrawNode(node);
  }

  // Handle interaction
  HandleNodeInteraction();

  // Draw connection being dragged
  if (m_IsDraggingConnection) {
    ImVec2 mousePos = ImGui::GetMousePos();
    MaterialNode *node = FindNode(m_DragConnectionNodeId);
    if (node) {
      // Find pin index
      int pinIndex = 0;
      bool found = false;

      if (m_DragConnectionIsOutput) {
        for (size_t i = 0; i < node->outputs.size(); i++) {
          if (node->outputs[i].id == m_DragConnectionPinId) {
            pinIndex = (int)i;
            found = true;
            break;
          }
        }
      } else {
        for (size_t i = 0; i < node->inputs.size(); i++) {
          if (node->inputs[i].id == m_DragConnectionPinId) {
            pinIndex = (int)i;
            found = true;
            break;
          }
        }
      }

      if (found) {
        ImVec2 pinPos;
        float pinY = canvasPos.y + node->position.y + m_ViewOffset.y + 35 +
                     pinIndex * 20;

        if (m_DragConnectionIsOutput) {
          pinPos = ImVec2(canvasPos.x + node->position.x + node->size.x +
                              m_ViewOffset.x,
                          pinY);
        } else {
          pinPos =
              ImVec2(canvasPos.x + node->position.x + m_ViewOffset.x, pinY);
        }

        ImU32 lineColor = IM_COL32(255, 200, 100, 255);
        float tangentLen = std::abs(mousePos.x - pinPos.x) * 0.5f + 30.0f;

        if (m_DragConnectionIsOutput) {
          drawList->AddBezierCubic(pinPos,
                                   ImVec2(pinPos.x + tangentLen, pinPos.y),
                                   ImVec2(mousePos.x - tangentLen, mousePos.y),
                                   mousePos, lineColor, 3.0f);
        } else {
          drawList->AddBezierCubic(pinPos,
                                   ImVec2(pinPos.x - tangentLen, pinPos.y),
                                   ImVec2(mousePos.x + tangentLen, mousePos.y),
                                   mousePos, lineColor, 3.0f);
        }
      }
    }
  }
}

void MaterialEditorPanel::DrawNode(MaterialNode &node) {
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  ImVec2 canvasPos = ImGui::GetCursorScreenPos();

  ImVec2 nodePos(canvasPos.x + node.position.x + m_ViewOffset.x,
                 canvasPos.y + node.position.y + m_ViewOffset.y);
  ImVec2 nodeSize(node.size.x, node.size.y);

  ImGui::PushID(node.id);

  // Node colors
  ImU32 bgColor = IM_COL32(60, 60, 60, 255);
  ImU32 headerColor;
  switch (node.type) {
  case MaterialNodeType::Output:
    headerColor = IM_COL32(150, 50, 50, 255);
    break;
  case MaterialNodeType::TextureSample:
    headerColor = IM_COL32(50, 100, 150, 255);
    break;
  case MaterialNodeType::Constant:
    headerColor = IM_COL32(100, 100, 50, 255);
    break;
  default:
    headerColor = IM_COL32(80, 80, 80, 255);
    break;
  }
  ImU32 borderColor = (m_SelectedNodeId == node.id)
                          ? IM_COL32(255, 200, 100, 255)
                          : IM_COL32(100, 100, 100, 255);

  // Draw node background
  drawList->AddRectFilled(
      nodePos, ImVec2(nodePos.x + nodeSize.x, nodePos.y + nodeSize.y), bgColor,
      4.0f);

  // Draw header
  drawList->AddRectFilled(nodePos,
                          ImVec2(nodePos.x + nodeSize.x, nodePos.y + 25),
                          headerColor, 4.0f, ImDrawFlags_RoundCornersTop);

  // Draw border
  drawList->AddRect(nodePos,
                    ImVec2(nodePos.x + nodeSize.x, nodePos.y + nodeSize.y),
                    borderColor, 4.0f, 0, 2.0f);

  // Draw title
  drawList->AddText(ImVec2(nodePos.x + 8, nodePos.y + 5),
                    IM_COL32(255, 255, 255, 255), node.name.c_str());

  // Draw input pins
  float pinY = nodePos.y + 30;
  for (auto &pin : node.inputs) {
    DrawNodePin(node, pin, false);
    pinY += 20;
  }

  // Draw output pins
  pinY = nodePos.y + 30;
  for (auto &pin : node.outputs) {
    DrawNodePin(node, pin, true);
    pinY += 20;
  }

  ImGui::PopID();
}

void MaterialEditorPanel::DrawNodePin(MaterialNode &node, NodePin &pin,
                                      bool isOutput) {
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  ImVec2 canvasPos = ImGui::GetCursorScreenPos();

  ImVec2 nodePos(canvasPos.x + node.position.x + m_ViewOffset.x,
                 canvasPos.y + node.position.y + m_ViewOffset.y);

  // Find pin index
  int pinIndex = 0;
  auto &pinList = isOutput ? node.outputs : node.inputs;
  for (size_t i = 0; i < pinList.size(); i++) {
    if (pinList[i].id == pin.id) {
      pinIndex = (int)i;
      break;
    }
  }

  float pinY = nodePos.y + 35 + pinIndex * 20;
  float pinX = isOutput ? nodePos.x + node.size.x : nodePos.x;

  ImVec2 pinPos(pinX, pinY);
  float pinRadius = 6.0f;

  Vec4 pinColorVec = GetPinColor(pin.type);
  ImU32 pinColor =
      IM_COL32((int)(pinColorVec.x * 255), (int)(pinColorVec.y * 255),
               (int)(pinColorVec.z * 255), 255);

  // Draw pin circle
  bool connected = pin.connectedNodeId != 0;
  if (connected) {
    drawList->AddCircleFilled(pinPos, pinRadius, pinColor);
  } else {
    drawList->AddCircle(pinPos, pinRadius, pinColor, 12, 2.0f);
  }

  // Draw pin name
  if (isOutput) {
    ImVec2 textSize = ImGui::CalcTextSize(pin.name.c_str());
    drawList->AddText(ImVec2(pinX - textSize.x - 10, pinY - 7),
                      IM_COL32(200, 200, 200, 255), pin.name.c_str());
  } else {
    drawList->AddText(ImVec2(pinX + 10, pinY - 7), IM_COL32(200, 200, 200, 255),
                      pin.name.c_str());
  }
}

void MaterialEditorPanel::DrawConnections() {
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  ImVec2 canvasPos = ImGui::GetCursorScreenPos();

  for (const auto &conn : m_Connections) {
    MaterialNode *outputNode = FindNode(conn.outputNodeId);
    MaterialNode *inputNode = FindNode(conn.inputNodeId);

    if (!outputNode || !inputNode)
      continue;

    // Find pin positions
    ImVec2 startPos, endPos;

    // Output pin position
    int outputPinIndex = 0;
    for (size_t i = 0; i < outputNode->outputs.size(); i++) {
      if (outputNode->outputs[i].id == conn.outputPinId) {
        outputPinIndex = (int)i;
        break;
      }
    }
    startPos = ImVec2(canvasPos.x + outputNode->position.x +
                          outputNode->size.x + m_ViewOffset.x,
                      canvasPos.y + outputNode->position.y + 35 +
                          outputPinIndex * 20 + m_ViewOffset.y);

    // Input pin position
    int inputPinIndex = 0;
    for (size_t i = 0; i < inputNode->inputs.size(); i++) {
      if (inputNode->inputs[i].id == conn.inputPinId) {
        inputPinIndex = (int)i;
        break;
      }
    }
    endPos = ImVec2(canvasPos.x + inputNode->position.x + m_ViewOffset.x,
                    canvasPos.y + inputNode->position.y + 35 +
                        inputPinIndex * 20 + m_ViewOffset.y);

    // Draw bezier curve
    ImU32 lineColor = IM_COL32(200, 200, 200, 255);
    float tangentLength = std::abs(endPos.x - startPos.x) * 0.5f;
    drawList->AddBezierCubic(
        startPos, ImVec2(startPos.x + tangentLength, startPos.y),
        ImVec2(endPos.x - tangentLength, endPos.y), endPos, lineColor, 2.0f);
  }
}

void MaterialEditorPanel::HandleNodeInteraction() {
  ImVec2 canvasPos = ImGui::GetCursorScreenPos();
  ImVec2 canvasSize = ImGui::GetContentRegionAvail();

  // Check if mouse is in canvas
  ImVec2 mousePos = ImGui::GetMousePos();
  bool inCanvas =
      mousePos.x >= canvasPos.x && mousePos.x < canvasPos.x + canvasSize.x &&
      mousePos.y >= canvasPos.y && mousePos.y < canvasPos.y + canvasSize.y;

  if (!inCanvas)
    return;

  // Delete selected node with Delete or Backspace key
  if (m_SelectedNodeId != 0 && (ImGui::IsKeyPressed(ImGuiKey_Delete) ||
                                ImGui::IsKeyPressed(ImGuiKey_Backspace))) {
    MaterialNode *node = FindNode(m_SelectedNodeId);
    if (node && node->type != MaterialNodeType::Output) {
      DeleteNode(m_SelectedNodeId);
      m_SelectedNodeId = 0;
    }
  }

  // Pan view with middle mouse or right mouse
  if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
    ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
    m_ViewOffset.x += delta.x;
    m_ViewOffset.y += delta.y;
    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
  }

  if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
    ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
    m_ViewOffset.x += delta.x;
    m_ViewOffset.y += delta.y;
    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
  }

  float pinRadius = 8.0f;

  // Check for pin clicks first (before node selection)
  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    bool clickedPin = false;

    for (auto &node : m_Nodes) {
      ImVec2 nodePos(canvasPos.x + node.position.x + m_ViewOffset.x,
                     canvasPos.y + node.position.y + m_ViewOffset.y);

      // Check output pins
      for (size_t i = 0; i < node.outputs.size(); i++) {
        float pinX = nodePos.x + node.size.x;
        float pinY = nodePos.y + 35 + i * 20;

        float dx = mousePos.x - pinX;
        float dy = mousePos.y - pinY;
        if (dx * dx + dy * dy <= pinRadius * pinRadius) {
          // Start dragging connection from output
          m_IsDraggingConnection = true;
          m_DragConnectionNodeId = node.id;
          m_DragConnectionPinId = node.outputs[i].id;
          m_DragConnectionIsOutput = true;
          clickedPin = true;
          break;
        }
      }

      if (clickedPin)
        break;

      // Check input pins
      for (size_t i = 0; i < node.inputs.size(); i++) {
        float pinX = nodePos.x;
        float pinY = nodePos.y + 35 + i * 20;

        float dx = mousePos.x - pinX;
        float dy = mousePos.y - pinY;
        if (dx * dx + dy * dy <= pinRadius * pinRadius) {
          // Start dragging connection from input
          m_IsDraggingConnection = true;
          m_DragConnectionNodeId = node.id;
          m_DragConnectionPinId = node.inputs[i].id;
          m_DragConnectionIsOutput = false;
          clickedPin = true;
          break;
        }
      }

      if (clickedPin)
        break;
    }

    // If didn't click a pin, check for node selection
    if (!clickedPin) {
      m_SelectedNodeId = 0;

      for (auto &node : m_Nodes) {
        ImVec2 nodePos(canvasPos.x + node.position.x + m_ViewOffset.x,
                       canvasPos.y + node.position.y + m_ViewOffset.y);
        ImVec2 nodeEnd(nodePos.x + node.size.x, nodePos.y + node.size.y);

        if (mousePos.x >= nodePos.x && mousePos.x < nodeEnd.x &&
            mousePos.y >= nodePos.y && mousePos.y < nodeEnd.y) {
          m_SelectedNodeId = node.id;
          m_DraggingNodeId = node.id;
          m_DragOffset = Vec2(mousePos.x - nodePos.x, mousePos.y - nodePos.y);
          break;
        }
      }
    }
  }

  // Drag node
  if (m_DraggingNodeId != 0 && !m_IsDraggingConnection &&
      ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    MaterialNode *node = FindNode(m_DraggingNodeId);
    if (node) {
      node->position.x =
          mousePos.x - canvasPos.x - m_ViewOffset.x - m_DragOffset.x;
      node->position.y =
          mousePos.y - canvasPos.y - m_ViewOffset.y - m_DragOffset.y;
    }
  }

  // Handle connection drop
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    if (m_IsDraggingConnection) {
      // Check if dropped on a compatible pin
      for (auto &node : m_Nodes) {
        if (node.id == m_DragConnectionNodeId)
          continue; // Can't connect to self

        ImVec2 nodePos(canvasPos.x + node.position.x + m_ViewOffset.x,
                       canvasPos.y + node.position.y + m_ViewOffset.y);

        if (m_DragConnectionIsOutput) {
          // Dragging from output, look for input pins
          for (size_t i = 0; i < node.inputs.size(); i++) {
            float pinX = nodePos.x;
            float pinY = nodePos.y + 35 + i * 20;

            float dx = mousePos.x - pinX;
            float dy = mousePos.y - pinY;
            if (dx * dx + dy * dy <= pinRadius * pinRadius) {
              CreateConnection(m_DragConnectionNodeId, m_DragConnectionPinId,
                               node.id, node.inputs[i].id);
              break;
            }
          }
        } else {
          // Dragging from input, look for output pins
          for (size_t i = 0; i < node.outputs.size(); i++) {
            float pinX = nodePos.x + node.size.x;
            float pinY = nodePos.y + 35 + i * 20;

            float dx = mousePos.x - pinX;
            float dy = mousePos.y - pinY;
            if (dx * dx + dy * dy <= pinRadius * pinRadius) {
              CreateConnection(node.id, node.outputs[i].id,
                               m_DragConnectionNodeId, m_DragConnectionPinId);
              break;
            }
          }
        }
      }
    }

    m_DraggingNodeId = 0;
    m_IsDraggingConnection = false;
  }

  // Delete selected node
  if (m_SelectedNodeId != 0 && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
    DeleteNode(m_SelectedNodeId);
    m_SelectedNodeId = 0;
  }
}

void MaterialEditorPanel::DrawNodeInspector() {
  if (m_SelectedNodeId == 0) {
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select a node to edit");
    return;
  }

  MaterialNode *node = FindNode(m_SelectedNodeId);
  if (!node)
    return;

  ImGui::Text("Node: %s", node->name.c_str());
  ImGui::Text("Type: %s", GetNodeTypeName(node->type));
  ImGui::Separator();

  // Rename node (not for Output node)
  if (node->type != MaterialNodeType::Output) {
    char nameBuffer[256];
    strncpy(nameBuffer, node->name.c_str(), sizeof(nameBuffer) - 1);
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';

    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
      node->name = nameBuffer;
      m_IsDirty = true;
    }

    // Delete node button (red - semantic for destructive action)
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.9f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Delete Node", ImVec2(-1, 0))) {
      DeleteNode(node->id);
      m_SelectedNodeId = 0;
      ImGui::PopStyleColor(3);
      return;
    }
    ImGui::PopStyleColor(3);
    ImGui::Separator();
  }

  // Material-level properties (shown when Output node is selected)
  if (node->type == MaterialNodeType::Output && m_Material) {
    ImGui::Separator();
    ImGui::Text("Material Properties");

    Vec3 albedo = m_Material->GetAlbedoColor();
    if (ImGui::ColorEdit3("Albedo Color", &albedo.x)) {
      m_Material->SetAlbedoColor(albedo);
      m_IsDirty = true;
    }

    f32 roughness = m_Material->GetRoughness();
    if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) {
      m_Material->SetRoughness(roughness);
      m_IsDirty = true;
    }

    f32 metallic = m_Material->GetMetallic();
    if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f)) {
      m_Material->SetMetallic(metallic);
      m_IsDirty = true;
    }

    f32 ao = m_Material->GetAO();
    if (ImGui::SliderFloat("AO", &ao, 0.0f, 1.0f)) {
      m_Material->SetAO(ao);
      m_IsDirty = true;
    }

    f32 heightScale = m_Material->GetHeightScale();
    if (ImGui::SliderFloat("Height Scale", &heightScale, 0.0f, 0.3f, "%.3f")) {
      m_Material->SetHeightScale(heightScale);
      m_IsDirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("?##HeightHelp")) {
      ImGui::OpenPopup("HeightHelp");
    }
    if (ImGui::BeginPopup("HeightHelp")) {
      ImGui::Text("Controls parallax depth intensity.");
      ImGui::Text("Typical values: 0.01 - 0.1");
      ImGui::Text("Connect a Height texture to the");
      ImGui::Text("Output node's Height pin to enable.");
      ImGui::EndPopup();
    }

    Vec2 tiling = m_Material->GetTiling();
    if (ImGui::DragFloat2("Tiling", &tiling.x, 0.1f, 0.1f, 100.0f)) {
      m_Material->SetTiling(tiling);
      m_IsDirty = true;
    }

    ImGui::Separator();
  }

  // Node-specific properties
  switch (node->type) {
  case MaterialNodeType::Constant:
    ImGui::ColorEdit4("Value", &node->constantValue.x);
    break;

  case MaterialNodeType::TextureSample: {
    ImGui::Text("Texture: %s", node->texturePath.empty()
                                   ? "(none)"
                                   : node->texturePath.c_str());

    // Show texture preview if loaded
    if (node->texture) {
      ImGui::Image((ImTextureID)(intptr_t)node->texture->GetID(),
                   ImVec2(64, 64));
    }

    if (ImGui::Button("Load Texture...")) {
      std::vector<FileDialogFilter> filters = {{"Image Files", "png"},
                                               {"Image Files", "jpg"},
                                               {"Image Files", "jpeg"},
                                               {"Image Files", "tga"},
                                               {"Image Files", "bmp"}};

      std::string filepath = FileDialog::OpenFile(filters);

      if (!filepath.empty()) {
        std::filesystem::path srcPath(filepath);
        std::filesystem::path destPath;

        // Check if file is already in project assets
        auto activeProject = Project::GetActive();
        if (activeProject) {
          std::filesystem::path assetsPath =
              activeProject->GetConfig().assetsPath;
          std::filesystem::path texturesPath = assetsPath / "textures";

          // Create textures directory if it doesn't exist
          if (!std::filesystem::exists(texturesPath)) {
            std::filesystem::create_directories(texturesPath);
          }

          // Check if file is already in assets
          std::string srcPathStr = srcPath.string();
          std::string assetsPathStr = assetsPath.string();

          if (srcPathStr.find(assetsPathStr) == 0) {
            // Already in assets, use relative path
            destPath = srcPath;
          } else {
            // Copy to project textures folder
            destPath = texturesPath / srcPath.filename();

            try {
              std::filesystem::copy_file(
                  srcPath, destPath,
                  std::filesystem::copy_options::overwrite_existing);
              GINI_INFO("Copied texture to project: ", destPath.string());
            } catch (const std::exception &e) {
              GINI_ERROR("Failed to copy texture: ", e.what());
              destPath = srcPath; // Use original path as fallback
            }
          }
        } else {
          destPath = srcPath;
        }

        // Load the texture
        node->texturePath = destPath.string();
        node->texture = Texture2D::Create(destPath.string());

        if (node->texture) {
          GINI_INFO("Loaded texture: ", destPath.string());
          m_IsDirty = true;
        } else {
          GINI_ERROR("Failed to load texture: ", destPath.string());
        }
      }
    }
    break;
  }

  default:
    ImGui::Text("No editable properties");
    break;
  }
}

void MaterialEditorPanel::DrawPreview() {
  ImVec2 previewSize(200, 200);

  // Render material preview to framebuffer
  if (m_PreviewFramebuffer && m_Material) {
    m_PreviewFramebuffer->Bind();

    glViewport(0, 0, 256, 256);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Update preview camera
    float radians = glm::radians(m_PreviewRotation);
    Vec3 camPos = Vec3(sin(radians) * 3.0f, 1.0f, cos(radians) * 3.0f);
    m_PreviewCamera->SetPosition(camPos);
    m_PreviewCamera->LookAt(Vec3(0, 0, 0));

    // Render a sphere with the material color
    Renderer3D::BeginScene(*m_PreviewCamera);

    // Get material albedo color for preview
    Vec4 albedo = m_Material->GetAlbedo();
    Color previewColor(albedo.r, albedo.g, albedo.b, albedo.a);

    // Draw sphere at origin with material color
    Renderer3D::DrawSphere(Vec3(0, 0, 0), 1.0f, previewColor);

    Renderer3D::EndScene();

    m_PreviewFramebuffer->Unbind();

    // Display the preview texture
    u32 textureID = m_PreviewFramebuffer->GetColorAttachment();
    ImGui::Image((ImTextureID)(intptr_t)textureID, previewSize, ImVec2(0, 1),
                 ImVec2(1, 0));
  } else {
    // Placeholder if no material
    ImGui::Button("Preview", previewSize);
  }

  // Rotation slider
  ImGui::SliderFloat("Rotation", &m_PreviewRotation, 0.0f, 360.0f);
}

MaterialNode *MaterialEditorPanel::CreateNode(MaterialNodeType type,
                                              Vec2 position) {
  MaterialNode node;
  node.id = GenerateId();
  node.type = type;
  node.position = position;
  node.name = GetNodeTypeName(type);

  // Setup pins based on type
  switch (type) {
  case MaterialNodeType::Output:
    node.inputs.push_back({GenerateId(), "Base Color", PinType::Vec3, true});
    node.inputs.push_back({GenerateId(), "Metallic", PinType::Float, true});
    node.inputs.push_back({GenerateId(), "Roughness", PinType::Float, true});
    node.inputs.push_back({GenerateId(), "Normal", PinType::Vec3, true});
    node.inputs.push_back({GenerateId(), "AO", PinType::Float, true});
    node.inputs.push_back({GenerateId(), "Height", PinType::Float, true});
    node.size = Vec2(150, 160);
    break;

  case MaterialNodeType::TextureSample:
    node.inputs.push_back({GenerateId(), "UV", PinType::Vec2, true});
    node.outputs.push_back({GenerateId(), "RGB", PinType::Vec3, false});
    node.outputs.push_back({GenerateId(), "R", PinType::Float, false});
    node.outputs.push_back({GenerateId(), "G", PinType::Float, false});
    node.outputs.push_back({GenerateId(), "B", PinType::Float, false});
    node.outputs.push_back({GenerateId(), "A", PinType::Float, false});
    node.size = Vec2(150, 140);
    break;

  case MaterialNodeType::Constant:
    node.outputs.push_back({GenerateId(), "Value", PinType::Vec4, false});
    node.size = Vec2(120, 60);
    break;

  case MaterialNodeType::Multiply:
  case MaterialNodeType::Add:
    node.inputs.push_back({GenerateId(), "A", PinType::Any, true});
    node.inputs.push_back({GenerateId(), "B", PinType::Any, true});
    node.outputs.push_back({GenerateId(), "Result", PinType::Any, false});
    node.size = Vec2(120, 80);
    break;

  case MaterialNodeType::Lerp:
    node.inputs.push_back({GenerateId(), "A", PinType::Any, true});
    node.inputs.push_back({GenerateId(), "B", PinType::Any, true});
    node.inputs.push_back({GenerateId(), "Alpha", PinType::Float, true});
    node.outputs.push_back({GenerateId(), "Result", PinType::Any, false});
    node.size = Vec2(120, 100);
    break;

  case MaterialNodeType::TexCoord:
    node.outputs.push_back({GenerateId(), "UV", PinType::Vec2, false});
    node.size = Vec2(100, 50);
    break;

  case MaterialNodeType::Time:
    node.outputs.push_back({GenerateId(), "Time", PinType::Float, false});
    node.size = Vec2(100, 50);
    break;

  default:
    node.outputs.push_back({GenerateId(), "Out", PinType::Any, false});
    node.size = Vec2(120, 60);
    break;
  }

  m_Nodes.push_back(node);
  m_IsDirty = true;

  GINI_INFO("Created node: ", node.name);
  return &m_Nodes.back();
}

void MaterialEditorPanel::DeleteNode(u32 nodeId) {
  // Remove connections to/from this node
  m_Connections.erase(std::remove_if(m_Connections.begin(), m_Connections.end(),
                                     [nodeId](const NodeConnection &conn) {
                                       return conn.inputNodeId == nodeId ||
                                              conn.outputNodeId == nodeId;
                                     }),
                      m_Connections.end());

  // Remove node
  m_Nodes.erase(std::remove_if(m_Nodes.begin(), m_Nodes.end(),
                               [nodeId](const MaterialNode &node) {
                                 return node.id == nodeId;
                               }),
                m_Nodes.end());

  m_IsDirty = true;
}

void MaterialEditorPanel::CreateConnection(u32 outputNodeId, u32 outputPinId,
                                           u32 inputNodeId, u32 inputPinId) {
  // Remove existing connection to input pin
  DeleteConnection(inputNodeId, inputPinId);

  NodeConnection conn;
  conn.outputNodeId = outputNodeId;
  conn.outputPinId = outputPinId;
  conn.inputNodeId = inputNodeId;
  conn.inputPinId = inputPinId;
  m_Connections.push_back(conn);

  // Update pin connection info
  MaterialNode *inputNode = FindNode(inputNodeId);
  if (inputNode) {
    for (auto &pin : inputNode->inputs) {
      if (pin.id == inputPinId) {
        pin.connectedNodeId = outputNodeId;
        pin.connectedPinId = outputPinId;
        break;
      }
    }
  }

  m_IsDirty = true;
}

void MaterialEditorPanel::DeleteConnection(u32 inputNodeId, u32 inputPinId) {
  m_Connections.erase(
      std::remove_if(m_Connections.begin(), m_Connections.end(),
                     [inputNodeId, inputPinId](const NodeConnection &conn) {
                       return conn.inputNodeId == inputNodeId &&
                              conn.inputPinId == inputPinId;
                     }),
      m_Connections.end());

  // Update pin connection info
  MaterialNode *node = FindNode(inputNodeId);
  if (node) {
    for (auto &pin : node->inputs) {
      if (pin.id == inputPinId) {
        pin.connectedNodeId = 0;
        pin.connectedPinId = 0;
        break;
      }
    }
  }
}

MaterialNode *MaterialEditorPanel::FindNode(u32 id) {
  for (auto &node : m_Nodes) {
    if (node.id == id)
      return &node;
  }
  return nullptr;
}

NodePin *MaterialEditorPanel::FindPin(MaterialNode *node, u32 pinId) {
  if (!node)
    return nullptr;

  for (auto &pin : node->inputs) {
    if (pin.id == pinId)
      return &pin;
  }
  for (auto &pin : node->outputs) {
    if (pin.id == pinId)
      return &pin;
  }
  return nullptr;
}

Vec4 MaterialEditorPanel::GetPinColor(PinType type) {
  switch (type) {
  case PinType::Float:
    return Vec4(0.5f, 0.8f, 0.5f, 1.0f);
  case PinType::Vec2:
    return Vec4(0.5f, 0.5f, 0.8f, 1.0f);
  case PinType::Vec3:
    return Vec4(0.8f, 0.8f, 0.5f, 1.0f);
  case PinType::Vec4:
    return Vec4(0.8f, 0.5f, 0.8f, 1.0f);
  case PinType::Texture:
    return Vec4(0.8f, 0.5f, 0.5f, 1.0f);
  case PinType::Any:
    return Vec4(0.7f, 0.7f, 0.7f, 1.0f);
  default:
    return Vec4(0.5f, 0.5f, 0.5f, 1.0f);
  }
}

const char *MaterialEditorPanel::GetNodeTypeName(MaterialNodeType type) {
  switch (type) {
  case MaterialNodeType::Output:
    return "Material Output";
  case MaterialNodeType::TextureSample:
    return "Texture Sample";
  case MaterialNodeType::Constant:
    return "Constant";
  case MaterialNodeType::Multiply:
    return "Multiply";
  case MaterialNodeType::Add:
    return "Add";
  case MaterialNodeType::Lerp:
    return "Lerp";
  case MaterialNodeType::Fresnel:
    return "Fresnel";
  case MaterialNodeType::Normal:
    return "Normal";
  case MaterialNodeType::Desaturate:
    return "Desaturate";
  case MaterialNodeType::Power:
    return "Power";
  case MaterialNodeType::Clamp:
    return "Clamp";
  case MaterialNodeType::OneMinus:
    return "One Minus";
  case MaterialNodeType::TexCoord:
    return "Tex Coord";
  case MaterialNodeType::Time:
    return "Time";
  case MaterialNodeType::Panner:
    return "Panner";
  default:
    return "Unknown";
  }
}

u32 MaterialEditorPanel::GenerateId() { return m_NextId++; }

void MaterialEditorPanel::OpenMaterial(Ref<Material> material) {
  m_Material = material;
  m_Nodes.clear();
  m_Connections.clear();
  m_NextId = 1;

  // Create output node
  CreateNode(MaterialNodeType::Output, Vec2(500, 200));

  // TODO: Parse material and create nodes from it

  m_IsDirty = false;
  m_Visible = true;
}

void MaterialEditorPanel::LoadMaterialFromFile(const std::string &filepath) {
  try {
    YAML::Node data = YAML::LoadFile(filepath);
    if (!data["Material"]) {
      GINI_ERROR("Invalid material file: {}", filepath);
      return;
    }

    auto matData = data["Material"];

    // Create new material
    std::string name =
        matData["Name"] ? matData["Name"].as<std::string>() : "Loaded Material";
    m_Material = Material::Create(name);
    m_MaterialPath = filepath;

    // Load material properties
    if (matData["Albedo"]) {
      auto albedo = matData["Albedo"];
      if (albedo.IsSequence() && albedo.size() >= 3) {
        Vec4 color(albedo[0].as<float>(), albedo[1].as<float>(),
                   albedo[2].as<float>(),
                   albedo.size() > 3 ? albedo[3].as<float>() : 1.0f);
        m_Material->SetAlbedo(color);
      }
    }
    if (matData["Roughness"]) {
      m_Material->SetRoughness(matData["Roughness"].as<float>());
    }
    if (matData["Metallic"]) {
      m_Material->SetMetallic(matData["Metallic"].as<float>());
    }

    // Load textures
    if (matData["AlbedoTexture"]) {
      std::string texPath = matData["AlbedoTexture"].as<std::string>();
      if (std::filesystem::exists(texPath)) {
        m_Material->SetAlbedoTexture(Texture2D::Create(texPath));
        m_Material->SetAlbedoTexturePath(texPath);
      }
    }
    if (matData["NormalTexture"]) {
      std::string texPath = matData["NormalTexture"].as<std::string>();
      if (std::filesystem::exists(texPath)) {
        m_Material->SetNormalTexture(Texture2D::Create(texPath));
        m_Material->SetNormalTexturePath(texPath);
      }
    }

    // Reset node graph
    m_Nodes.clear();
    m_Connections.clear();
    m_NextId = 1;

    // Load nodes from file
    if (matData["Nodes"]) {
      u32 maxId = 0;
      for (const auto &nodeData : matData["Nodes"]) {
        u32 id = nodeData["ID"].as<u32>();
        int typeInt = nodeData["Type"].as<int>();
        MaterialNodeType type = static_cast<MaterialNodeType>(typeInt);
        std::string nodeName = nodeData["Name"].as<std::string>();

        Vec2 position(0, 0);
        if (nodeData["Position"]) {
          auto pos = nodeData["Position"];
          position = Vec2(pos[0].as<float>(), pos[1].as<float>());
        }

        // Create the node with proper type
        MaterialNode *node = CreateNode(type, position);
        if (node) {
          // Override the auto-generated ID with the saved ID
          node->id = id;
          node->name = nodeName;

          // Load node-specific data
          if (type == MaterialNodeType::Constant && nodeData["ConstantValue"]) {
            auto cv = nodeData["ConstantValue"];
            node->constantValue = Vec4(cv[0].as<float>(), cv[1].as<float>(),
                                       cv[2].as<float>(), cv[3].as<float>());
          }
          if (type == MaterialNodeType::TextureSample &&
              nodeData["TexturePath"]) {
            node->texturePath = nodeData["TexturePath"].as<std::string>();
            if (std::filesystem::exists(node->texturePath)) {
              node->texture = Texture2D::Create(node->texturePath);
            }
          }

          // Restore input pin IDs
          if (nodeData["InputPins"]) {
            auto inputPins = nodeData["InputPins"];
            for (size_t i = 0; i < inputPins.size() && i < node->inputs.size();
                 i++) {
              u32 pinId = inputPins[i].as<u32>();
              node->inputs[i].id = pinId;
              if (pinId > maxId)
                maxId = pinId;
            }
          }

          // Restore output pin IDs
          if (nodeData["OutputPins"]) {
            auto outputPins = nodeData["OutputPins"];
            for (size_t i = 0;
                 i < outputPins.size() && i < node->outputs.size(); i++) {
              u32 pinId = outputPins[i].as<u32>();
              node->outputs[i].id = pinId;
              if (pinId > maxId)
                maxId = pinId;
            }
          }

          if (id > maxId)
            maxId = id;
        }
      }
      m_NextId = maxId + 1;
    } else {
      // No saved nodes, create default output node
      CreateNode(MaterialNodeType::Output, Vec2(500, 200));
    }

    // Load connections
    if (matData["Connections"]) {
      for (const auto &connData : matData["Connections"]) {
        u32 outputNodeId = connData["OutputNode"].as<u32>();
        u32 outputPinId = connData["OutputPin"].as<u32>();
        u32 inputNodeId = connData["InputNode"].as<u32>();
        u32 inputPinId = connData["InputPin"].as<u32>();

        // Find the nodes and update pin connections
        MaterialNode *inputNode = FindNode(inputNodeId);
        if (inputNode) {
          for (auto &pin : inputNode->inputs) {
            if (pin.id == inputPinId) {
              pin.connectedNodeId = outputNodeId;
              pin.connectedPinId = outputPinId;
              break;
            }
          }
        }

        // Add connection
        NodeConnection conn;
        conn.outputNodeId = outputNodeId;
        conn.outputPinId = outputPinId;
        conn.inputNodeId = inputNodeId;
        conn.inputPinId = inputPinId;
        m_Connections.push_back(conn);
      }
    }

    m_IsDirty = false;
    m_Visible = true;

    GINI_INFO("Loaded material: {}", filepath);

  } catch (const std::exception &e) {
    GINI_ERROR("Failed to load material: {}", e.what());
  }
}

void MaterialEditorPanel::NewMaterial() {
  m_Material = Material::Create("New Material");
  m_Nodes.clear();
  m_Connections.clear();
  m_NextId = 1;

  // Create output node
  CreateNode(MaterialNodeType::Output, Vec2(500, 200));

  m_IsDirty = true;
  m_Visible = true; // Make sure the panel is visible
}

void MaterialEditorPanel::SaveMaterial() {
  if (!m_Material)
    return;

  // Compile the material first
  CompileMaterial();

  // Get save path
  auto activeProject = Project::GetActive();
  std::filesystem::path savePath;

  if (activeProject) {
    std::filesystem::path materialsPath =
        activeProject->GetConfig().materialsPath;
    if (!std::filesystem::exists(materialsPath)) {
      std::filesystem::create_directories(materialsPath);
    }

    // Use file dialog to get save location
    std::vector<FileDialogFilter> filters = {{"Gini Material", "gmat"}};

    std::string filepath =
        FileDialog::SaveFile(filters, m_Material->GetName() + ".gmat");

    if (filepath.empty()) {
      return; // User cancelled
    }

    savePath = filepath;
  } else {
    GINI_WARN("No active project - material not saved");
    return;
  }

  // Save material to YAML file
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;
  out << YAML::Key << "Name" << YAML::Value << m_Material->GetName();

  // Save material properties
  out << YAML::Key << "Albedo" << YAML::Value << YAML::Flow << YAML::BeginSeq
      << m_Material->GetAlbedo().x << m_Material->GetAlbedo().y
      << m_Material->GetAlbedo().z << m_Material->GetAlbedo().w << YAML::EndSeq;
  out << YAML::Key << "Roughness" << YAML::Value << m_Material->GetRoughness();
  out << YAML::Key << "Metallic" << YAML::Value << m_Material->GetMetallic();

  // Save texture paths
  if (m_Material->GetAlbedoTexture()) {
    out << YAML::Key << "AlbedoTexture" << YAML::Value
        << m_Material->GetAlbedoTexturePath();
  }
  if (m_Material->GetNormalTexture()) {
    out << YAML::Key << "NormalTexture" << YAML::Value
        << m_Material->GetNormalTexturePath();
  }

  // Save node graph
  out << YAML::Key << "Nodes" << YAML::Value << YAML::BeginSeq;
  for (const auto &node : m_Nodes) {
    out << YAML::BeginMap;
    out << YAML::Key << "ID" << YAML::Value << node.id;
    out << YAML::Key << "Type" << YAML::Value << (int)node.type;
    out << YAML::Key << "Name" << YAML::Value << node.name;
    out << YAML::Key << "Position" << YAML::Value << YAML::Flow
        << YAML::BeginSeq << node.position.x << node.position.y << YAML::EndSeq;

    if (node.type == MaterialNodeType::Constant) {
      out << YAML::Key << "ConstantValue" << YAML::Value << YAML::Flow
          << YAML::BeginSeq << node.constantValue.x << node.constantValue.y
          << node.constantValue.z << node.constantValue.w << YAML::EndSeq;
    }
    if (node.type == MaterialNodeType::TextureSample &&
        !node.texturePath.empty()) {
      out << YAML::Key << "TexturePath" << YAML::Value << node.texturePath;
    }

    // Save input pin IDs
    out << YAML::Key << "InputPins" << YAML::Value << YAML::Flow
        << YAML::BeginSeq;
    for (const auto &pin : node.inputs) {
      out << pin.id;
    }
    out << YAML::EndSeq;

    // Save output pin IDs
    out << YAML::Key << "OutputPins" << YAML::Value << YAML::Flow
        << YAML::BeginSeq;
    for (const auto &pin : node.outputs) {
      out << pin.id;
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;
  }
  out << YAML::EndSeq;

  // Save connections
  out << YAML::Key << "Connections" << YAML::Value << YAML::BeginSeq;
  for (const auto &conn : m_Connections) {
    out << YAML::BeginMap;
    out << YAML::Key << "OutputNode" << YAML::Value << conn.outputNodeId;
    out << YAML::Key << "OutputPin" << YAML::Value << conn.outputPinId;
    out << YAML::Key << "InputNode" << YAML::Value << conn.inputNodeId;
    out << YAML::Key << "InputPin" << YAML::Value << conn.inputPinId;
    out << YAML::EndMap;
  }
  out << YAML::EndSeq;

  out << YAML::EndMap; // Material
  out << YAML::EndMap; // Root

  std::ofstream fout(savePath);
  if (fout.is_open()) {
    fout << out.c_str();
    fout.close();
    GINI_INFO("Material saved to: ", savePath.string());
    m_IsDirty = false;
  } else {
    GINI_ERROR("Failed to save material to: ", savePath.string());
  }
}

void MaterialEditorPanel::CompileMaterial() {
  if (!m_Material)
    return;

  GINI_INFO("Compiling material...");

  // Find the output node
  MaterialNode *outputNode = nullptr;
  for (auto &node : m_Nodes) {
    if (node.type == MaterialNodeType::Output) {
      outputNode = &node;
      break;
    }
  }

  if (!outputNode) {
    GINI_WARN("No output node found in material graph");
    return;
  }

  // Process each input pin of the output node
  for (const auto &pin : outputNode->inputs) {
    // Find connection to this pin
    NodeConnection *conn = nullptr;
    for (auto &c : m_Connections) {
      if (c.inputNodeId == outputNode->id && c.inputPinId == pin.id) {
        conn = &c;
        break;
      }
    }

    if (!conn)
      continue;

    // Find the source node
    MaterialNode *sourceNode = FindNode(conn->outputNodeId);
    if (!sourceNode)
      continue;

    // Apply based on pin name
    if (pin.name == "Base Color") {
      if (sourceNode->type == MaterialNodeType::TextureSample &&
          sourceNode->texture) {
        m_Material->SetAlbedoTexture(sourceNode->texture);
        m_Material->SetAlbedoTexturePath(sourceNode->texturePath);
        GINI_INFO("Set albedo texture from node");
      } else if (sourceNode->type == MaterialNodeType::Constant) {
        Vec4 color = sourceNode->constantValue;
        m_Material->SetAlbedo(color);
        GINI_INFO("Set albedo color: ", color.x, ", ", color.y, ", ", color.z);
      }
    } else if (pin.name == "Normal") {
      if (sourceNode->type == MaterialNodeType::TextureSample &&
          sourceNode->texture) {
        m_Material->SetNormalTexture(sourceNode->texture);
        m_Material->SetNormalTexturePath(sourceNode->texturePath);
        GINI_INFO("Set normal texture from node");
      }
    } else if (pin.name == "Roughness") {
      if (sourceNode->type == MaterialNodeType::Constant) {
        m_Material->SetRoughness(sourceNode->constantValue.x);
        GINI_INFO("Set roughness: ", sourceNode->constantValue.x);
      } else if (sourceNode->type == MaterialNodeType::TextureSample &&
                 sourceNode->texture) {
        m_Material->SetRoughnessTexture(sourceNode->texture);
        GINI_INFO("Set roughness texture from node");
      }
    } else if (pin.name == "Metallic") {
      if (sourceNode->type == MaterialNodeType::Constant) {
        m_Material->SetMetallic(sourceNode->constantValue.x);
        GINI_INFO("Set metallic: ", sourceNode->constantValue.x);
      } else if (sourceNode->type == MaterialNodeType::TextureSample &&
                 sourceNode->texture) {
        m_Material->SetMetallicTexture(sourceNode->texture);
        GINI_INFO("Set metallic texture from node");
      }
    } else if (pin.name == "AO") {
      if (sourceNode->type == MaterialNodeType::Constant) {
        m_Material->SetAO(sourceNode->constantValue.x);
        GINI_INFO("Set AO: ", sourceNode->constantValue.x);
      } else if (sourceNode->type == MaterialNodeType::TextureSample &&
                 sourceNode->texture) {
        m_Material->SetAOTexture(sourceNode->texture);
        GINI_INFO("Set AO texture from node");
      }
    } else if (pin.name == "Height") {
      if (sourceNode->type == MaterialNodeType::TextureSample &&
          sourceNode->texture) {
        m_Material->SetHeightTexture(sourceNode->texture);
        GINI_INFO("Set height texture from node");
      } else if (sourceNode->type == MaterialNodeType::Constant) {
        m_Material->SetHeightScale(sourceNode->constantValue.x);
        GINI_INFO("Set height scale: ", sourceNode->constantValue.x);
      }
    }
  }

  GINI_INFO("Material compiled successfully");
}

} // namespace Gini
