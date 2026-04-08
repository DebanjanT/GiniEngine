#pragma once

#include "Core/Types.h"
#include <string>
#include <vector>

namespace Gini {

static constexpr u32 MAX_BONE_INFLUENCE = 4;

struct Vertex3D {
  Vec3 position;
  Vec3 normal;
  Vec2 texCoords;
  Vec3 tangent;
  Vec3 bitangent;
};

struct SkinnedVertex3D {
  Vec3 position;
  Vec3 normal;
  Vec2 texCoords;
  Vec3 tangent;
  Vec3 bitangent;
  i32 boneIDs[MAX_BONE_INFLUENCE] = {-1, -1, -1, -1};
  f32 boneWeights[MAX_BONE_INFLUENCE] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct MeshData {
  std::vector<Vertex3D> vertices;
  std::vector<u32> indices;
  std::string name;
};

class Mesh {
public:
  Mesh() = default;
  Mesh(const std::vector<Vertex3D> &vertices, const std::vector<u32> &indices);
  Mesh(const std::vector<SkinnedVertex3D> &vertices,
       const std::vector<u32> &indices);
  ~Mesh();

  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;
  Mesh(Mesh &&other) noexcept;
  Mesh &operator=(Mesh &&other) noexcept;

  void Create(const std::vector<Vertex3D> &vertices,
              const std::vector<u32> &indices);
  void CreateSkinned(const std::vector<SkinnedVertex3D> &vertices,
                     const std::vector<u32> &indices);
  void Destroy();

  void Bind() const;
  void Unbind() const;
  void Draw() const;

  u32 GetVertexCount() const { return m_VertexCount; }
  u32 GetIndexCount() const { return m_IndexCount; }
  u32 GetVAO() const { return m_VAO; }
  bool IsSkinned() const { return m_Skinned; }

  static Ref<Mesh> CreateCube(f32 size = 1.0f);
  static Ref<Mesh> CreateSphere(f32 radius = 1.0f, u32 segments = 32,
                                u32 rings = 16);
  static Ref<Mesh> CreatePlane(f32 width = 1.0f, f32 height = 1.0f);
  static Ref<Mesh> CreateCylinder(f32 radius = 0.5f, f32 height = 1.0f,
                                  u32 segments = 32);

private:
  u32 m_VAO = 0;
  u32 m_VBO = 0;
  u32 m_EBO = 0;
  u32 m_VertexCount = 0;
  u32 m_IndexCount = 0;
  bool m_Skinned = false;
};

} // namespace Gini
