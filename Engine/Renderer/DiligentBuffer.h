#pragma once

#include "../Core/Types.h"
#include <memory>
#include <vector>

// Diligent Engine includes
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"

namespace Gini {

// Vertex structure for Diligent Engine
struct DiligentVertex {
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TexCoords;
  glm::vec3 Tangent;
  glm::vec3 Bitangent;
  glm::vec4 Color;
};

// Diligent Engine vertex buffer wrapper
class DiligentVertexBuffer {
public:
  DiligentVertexBuffer();
  ~DiligentVertexBuffer() = default;

  // Buffer creation
  bool Create(const void *data, u32 size, bool dynamic = false);
  bool Create(const std::vector<DiligentVertex> &vertices,
              bool dynamic = false);

  // Buffer operations
  void UpdateData(const void *data, u32 size, u32 offset = 0);
  void Bind(u32 slot = 0) const;
  void Unbind() const;

  // Getters
  Diligent::IBuffer *GetBuffer() const { return m_Buffer; }
  u32 GetSize() const { return m_Size; }
  u32 GetVertexCount() const { return m_VertexCount; }
  bool IsValid() const { return m_Buffer != nullptr; }

private:
  Diligent::RefCntAutoPtr<Diligent::IBuffer> m_Buffer;
  u32 m_Size;
  u32 m_VertexCount;
  bool m_Dynamic;

  bool CreateInternal(const void *data, u32 size, bool dynamic);
};

// Diligent Engine index buffer wrapper
class DiligentIndexBuffer {
public:
  DiligentIndexBuffer();
  ~DiligentIndexBuffer() = default;

  // Buffer creation
  bool Create(const u32 *indices, u32 count, bool dynamic = false);
  bool Create(const std::vector<u32> &indices, bool dynamic = false);
  bool Create(const u16 *indices, u32 count, bool dynamic = false);
  bool Create(const std::vector<u16> &indices, bool dynamic = false);

  // Buffer operations
  void UpdateData(const u32 *indices, u32 count, u32 offset = 0);
  void Bind() const;
  void Unbind() const;

  // Getters
  Diligent::IBuffer *GetBuffer() const { return m_Buffer; }
  u32 GetIndexCount() const { return m_IndexCount; }
  u32 GetIndexSize() const { return m_IndexSize; }
  bool IsValid() const { return m_Buffer != nullptr; }

private:
  Diligent::RefCntAutoPtr<Diligent::IBuffer> m_Buffer;
  u32 m_IndexCount;
  u32 m_IndexSize;
  bool m_Dynamic;

  bool CreateInternal(const void *indices, u32 count, u32 indexSize,
                      bool dynamic);
};

// Vertex array object wrapper (combines vertex and index buffers)
class DiligentVertexArray {
public:
  DiligentVertexArray();
  ~DiligentVertexArray() = default;

  // Buffer management
  void SetVertexBuffer(const Ref<DiligentVertexBuffer> &vertexBuffer);
  void SetIndexBuffer(const Ref<DiligentIndexBuffer> &indexBuffer);

  // Rendering operations
  void Bind() const;
  void Unbind() const;
  void Draw(u32 vertexCount = 0) const;
  void DrawIndexed(u32 indexCount = 0) const;
  void DrawInstanced(u32 instanceCount, u32 vertexCount = 0) const;
  void DrawIndexedInstanced(u32 instanceCount, u32 indexCount = 0) const;

  // Getters
  Ref<DiligentVertexBuffer> GetVertexBuffer() const { return m_VertexBuffer; }
  Ref<DiligentIndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
  bool IsValid() const { return m_VertexBuffer && m_VertexBuffer->IsValid(); }

private:
  Ref<DiligentVertexBuffer> m_VertexBuffer;
  Ref<DiligentIndexBuffer> m_IndexBuffer;

  mutable bool m_Bound;
};

// Uniform buffer wrapper for constant data
class DiligentUniformBuffer {
public:
  DiligentUniformBuffer();
  ~DiligentUniformBuffer() = default;

  // Buffer creation
  bool Create(u32 size, bool dynamic = true);

  // Buffer operations
  void UpdateData(const void *data, u32 size, u32 offset = 0);
  void Bind(u32 slot = 0) const;
  void Unbind() const;

  // Getters
  Diligent::IBuffer *GetBuffer() const { return m_Buffer; }
  u32 GetSize() const { return m_Size; }
  bool IsValid() const { return m_Buffer != nullptr; }

private:
  Diligent::RefCntAutoPtr<Diligent::IBuffer> m_Buffer;
  u32 m_Size;
  bool m_Dynamic;
};

// Buffer factory for creating common buffer types
class DiligentBufferFactory {
public:
  static Ref<DiligentVertexBuffer> CreateQuad();
  static Ref<DiligentVertexBuffer> CreateCube();
  static Ref<DiligentVertexBuffer> CreateSphere(u32 segments = 32);
  static Ref<DiligentVertexBuffer> CreatePlane(u32 width = 1, u32 height = 1,
                                               u32 segments = 1);

  static Ref<DiligentVertexArray> CreateQuadVAO();
  static Ref<DiligentVertexArray> CreateCubeVAO();
  static Ref<DiligentVertexArray> CreateSphereVAO(u32 segments = 32);
  static Ref<DiligentVertexArray> CreatePlaneVAO(u32 width = 1, u32 height = 1,
                                                 u32 segments = 1);

private:
  static void GenerateCubeVertices(std::vector<DiligentVertex> &vertices,
                                   std::vector<u32> &indices);
  static void GenerateSphereVertices(std::vector<DiligentVertex> &vertices,
                                     std::vector<u32> &indices, u32 segments);
  static void GeneratePlaneVertices(std::vector<DiligentVertex> &vertices,
                                    std::vector<u32> &indices, u32 width,
                                    u32 height, u32 segments);
};

} // namespace Gini
