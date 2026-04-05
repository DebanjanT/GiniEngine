#pragma once

#include "EditorPanel.h"
#include "Renderer/Camera3D.h"
#include "Renderer/Framebuffer.h"
#include "Renderer/Material.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace Gini {

// Node types for material graph
enum class MaterialNodeType {
  Output,        // Final material output
  TextureSample, // Sample a texture
  Constant,      // Constant value (float, vec2, vec3, vec4)
  Multiply,      // Multiply two values
  Add,           // Add two values
  Lerp,          // Linear interpolation
  Fresnel,       // Fresnel effect
  Normal,        // Normal map processing
  Desaturate,    // Remove color saturation
  Power,         // Power function
  Clamp,         // Clamp value
  OneMinus,      // 1 - x
  TexCoord,      // Texture coordinates
  Time,          // Time value for animation
  Panner,        // UV panning
};

// Pin types for connections
enum class PinType { Float, Vec2, Vec3, Vec4, Texture, Any };

struct NodePin {
  u32 id;
  std::string name;
  PinType type;
  bool isInput;
  Vec4 defaultValue = Vec4(0.0f);
  u32 connectedNodeId = 0;
  u32 connectedPinId = 0;
};

struct MaterialNode {
  u32 id;
  MaterialNodeType type;
  std::string name;
  Vec2 position;
  Vec2 size = Vec2(150, 100);
  std::vector<NodePin> inputs;
  std::vector<NodePin> outputs;

  // Node-specific data
  std::string texturePath;
  Ref<Texture2D> texture;
  Vec4 constantValue = Vec4(1.0f);
  f32 floatValue = 1.0f;
};

struct NodeConnection {
  u32 outputNodeId;
  u32 outputPinId;
  u32 inputNodeId;
  u32 inputPinId;
};

class MaterialEditorPanel : public EditorPanel {
public:
  MaterialEditorPanel();
  ~MaterialEditorPanel() = default;

  void OnImGuiRender() override;

  void OpenMaterial(Ref<Material> material);
  void NewMaterial();
  void SaveMaterial();
  void CompileMaterial();

  Ref<Material> GetMaterial() const { return m_Material; }
  bool IsOpen() const { return m_Visible; }
  void SetOpen(bool open) { m_Visible = open; }

private:
  void DrawMenuBar();
  void DrawNodePalette();
  void DrawNodeGraph();
  void DrawNodeInspector();
  void DrawPreview();

  void DrawNode(MaterialNode &node);
  void DrawNodePin(MaterialNode &node, NodePin &pin, bool isOutput);
  void DrawConnections();
  void HandleNodeInteraction();

  MaterialNode *CreateNode(MaterialNodeType type, Vec2 position);
  void DeleteNode(u32 nodeId);
  void CreateConnection(u32 outputNodeId, u32 outputPinId, u32 inputNodeId,
                        u32 inputPinId);
  void DeleteConnection(u32 inputNodeId, u32 inputPinId);

  MaterialNode *FindNode(u32 id);
  NodePin *FindPin(MaterialNode *node, u32 pinId);

  Vec4 GetPinColor(PinType type);
  const char *GetNodeTypeName(MaterialNodeType type);

  u32 GenerateId();

  // Material being edited
  Ref<Material> m_Material;
  std::string m_MaterialPath;
  bool m_IsDirty = false;

  // Node graph
  std::vector<MaterialNode> m_Nodes;
  std::vector<NodeConnection> m_Connections;
  u32 m_NextId = 1;

  // Interaction state
  u32 m_SelectedNodeId = 0;
  u32 m_DraggingNodeId = 0;
  Vec2 m_DragOffset;
  bool m_IsDraggingConnection = false;
  u32 m_DragConnectionNodeId = 0;
  u32 m_DragConnectionPinId = 0;
  bool m_DragConnectionIsOutput = false;

  // View state
  Vec2 m_ViewOffset = Vec2(0, 0);
  f32 m_ViewZoom = 1.0f;

  // Preview
  Ref<Framebuffer> m_PreviewFramebuffer;
  Scope<Camera3D> m_PreviewCamera;
  f32 m_PreviewRotation = 0.0f;

  // UI state
  bool m_ShowPalette = true;
  bool m_ShowInspector = true;
  bool m_ShowPreview = true;
};

} // namespace Gini
