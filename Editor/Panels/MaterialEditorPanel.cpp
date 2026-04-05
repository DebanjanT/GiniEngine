#include "MaterialEditorPanel.h"
#include "Core/Logger.h"
#include <cmath>
#include <imgui.h>

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

  if (ImGui::Begin("Material Editor", &m_Visible, mainWindowFlags)) {
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
      if (ImGui::MenuItem("Open Material...")) {
        // TODO: File dialog
      }
      if (ImGui::MenuItem("Save", "Ctrl+S")) {
        SaveMaterial();
      }
      if (ImGui::MenuItem("Save As...")) {
        // TODO: File dialog
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

  // Node-specific properties
  switch (node->type) {
  case MaterialNodeType::Constant:
    ImGui::ColorEdit4("Value", &node->constantValue.x);
    break;

  case MaterialNodeType::TextureSample:
    ImGui::Text("Texture: %s", node->texturePath.empty()
                                   ? "(none)"
                                   : node->texturePath.c_str());
    if (ImGui::Button("Load Texture...")) {
      // TODO: File dialog
    }
    break;

  default:
    ImGui::Text("No editable properties");
    break;
  }
}

void MaterialEditorPanel::DrawPreview() {
  ImVec2 previewSize(200, 200);

  // Render preview sphere
  // TODO: Render material preview

  // For now, just show a placeholder
  ImGui::Button("Preview", previewSize);

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
    node.size = Vec2(150, 140);
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

  CompileMaterial();

  // TODO: Save material to file
  GINI_INFO("Material saved");
  m_IsDirty = false;
}

void MaterialEditorPanel::CompileMaterial() {
  if (!m_Material)
    return;

  // TODO: Compile node graph to material properties
  GINI_INFO("Material compiled");
}

} // namespace Gini
