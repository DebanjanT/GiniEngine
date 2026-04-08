#include "Mesh.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cmath>
#include <glad/gl.h>
#include <glm/glm.hpp>

namespace {
constexpr float PI = 3.14159265358979323846f;
}

namespace Gini {

Mesh::Mesh(const std::vector<Vertex3D> &vertices,
           const std::vector<u32> &indices) {
  Create(vertices, indices);
}

Mesh::Mesh(const std::vector<SkinnedVertex3D> &vertices,
           const std::vector<u32> &indices) {
  CreateSkinned(vertices, indices);
}

Mesh::~Mesh() { Destroy(); }

Mesh::Mesh(Mesh &&other) noexcept
    : m_VAO(other.m_VAO), m_VBO(other.m_VBO), m_EBO(other.m_EBO),
      m_VertexCount(other.m_VertexCount), m_IndexCount(other.m_IndexCount) {
  other.m_VAO = 0;
  other.m_VBO = 0;
  other.m_EBO = 0;
}

Mesh &Mesh::operator=(Mesh &&other) noexcept {
  if (this != &other) {
    Destroy();
    m_VAO = other.m_VAO;
    m_VBO = other.m_VBO;
    m_EBO = other.m_EBO;
    m_VertexCount = other.m_VertexCount;
    m_IndexCount = other.m_IndexCount;
    other.m_VAO = 0;
    other.m_VBO = 0;
    other.m_EBO = 0;
  }
  return *this;
}

void Mesh::Create(const std::vector<Vertex3D> &vertices,
                  const std::vector<u32> &indices) {
  m_VertexCount = static_cast<u32>(vertices.size());
  m_IndexCount = static_cast<u32>(indices.size());

  glGenVertexArrays(1, &m_VAO);
  glGenBuffers(1, &m_VBO);
  glGenBuffers(1, &m_EBO);

  glBindVertexArray(m_VAO);

  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex3D),
               vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(u32),
               indices.data(), GL_STATIC_DRAW);

  // Position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                        (void *)offsetof(Vertex3D, position));

  // Normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                        (void *)offsetof(Vertex3D, normal));

  // TexCoords
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                        (void *)offsetof(Vertex3D, texCoords));

  // Tangent
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                        (void *)offsetof(Vertex3D, tangent));

  // Bitangent
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                        (void *)offsetof(Vertex3D, bitangent));

  // Vertex color
  glEnableVertexAttribArray(7);
  glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                        (void *)offsetof(Vertex3D, color));

  glBindVertexArray(0);
}

void Mesh::CreateSkinned(const std::vector<SkinnedVertex3D> &vertices,
                         const std::vector<u32> &indices) {
  m_Skinned = true;
  m_VertexCount = static_cast<u32>(vertices.size());
  m_IndexCount = static_cast<u32>(indices.size());

  glGenVertexArrays(1, &m_VAO);
  glGenBuffers(1, &m_VBO);
  glGenBuffers(1, &m_EBO);

  glBindVertexArray(m_VAO);

  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(SkinnedVertex3D),
               vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(u32),
               indices.data(), GL_STATIC_DRAW);

  // Position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex3D),
                        (void *)offsetof(SkinnedVertex3D, position));

  // Normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex3D),
                        (void *)offsetof(SkinnedVertex3D, normal));

  // TexCoords
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex3D),
                        (void *)offsetof(SkinnedVertex3D, texCoords));

  // Tangent
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex3D),
                        (void *)offsetof(SkinnedVertex3D, tangent));

  // Bitangent
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex3D),
                        (void *)offsetof(SkinnedVertex3D, bitangent));

  // Bone IDs (ivec4 as integer attribs)
  glEnableVertexAttribArray(5);
  glVertexAttribIPointer(5, 4, GL_INT, sizeof(SkinnedVertex3D),
                         (void *)offsetof(SkinnedVertex3D, boneIDs));

  // Bone Weights
  glEnableVertexAttribArray(6);
  glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex3D),
                        (void *)offsetof(SkinnedVertex3D, boneWeights));

  // Vertex color
  glEnableVertexAttribArray(7);
  glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex3D),
                        (void *)offsetof(SkinnedVertex3D, color));

  glBindVertexArray(0);
}

void Mesh::Destroy() {
  if (m_VAO) {
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
    m_VAO = 0;
    m_VBO = 0;
    m_EBO = 0;
  }
}

void Mesh::Bind() const { glBindVertexArray(m_VAO); }

void Mesh::Unbind() const { glBindVertexArray(0); }

void Mesh::Draw() const {
  glBindVertexArray(m_VAO);
  glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}

Ref<Mesh> Mesh::CreateCube(f32 size) {
  f32 s = size * 0.5f;

  std::vector<Vertex3D> vertices = {
      // Front face
      {{-s, -s, s}, {0, 0, 1}, {0, 0}, {1, 0, 0}, {0, 1, 0}},
      {{s, -s, s}, {0, 0, 1}, {1, 0}, {1, 0, 0}, {0, 1, 0}},
      {{s, s, s}, {0, 0, 1}, {1, 1}, {1, 0, 0}, {0, 1, 0}},
      {{-s, s, s}, {0, 0, 1}, {0, 1}, {1, 0, 0}, {0, 1, 0}},
      // Back face
      {{s, -s, -s}, {0, 0, -1}, {0, 0}, {-1, 0, 0}, {0, 1, 0}},
      {{-s, -s, -s}, {0, 0, -1}, {1, 0}, {-1, 0, 0}, {0, 1, 0}},
      {{-s, s, -s}, {0, 0, -1}, {1, 1}, {-1, 0, 0}, {0, 1, 0}},
      {{s, s, -s}, {0, 0, -1}, {0, 1}, {-1, 0, 0}, {0, 1, 0}},
      // Top face
      {{-s, s, s}, {0, 1, 0}, {0, 0}, {1, 0, 0}, {0, 0, -1}},
      {{s, s, s}, {0, 1, 0}, {1, 0}, {1, 0, 0}, {0, 0, -1}},
      {{s, s, -s}, {0, 1, 0}, {1, 1}, {1, 0, 0}, {0, 0, -1}},
      {{-s, s, -s}, {0, 1, 0}, {0, 1}, {1, 0, 0}, {0, 0, -1}},
      // Bottom face
      {{-s, -s, -s}, {0, -1, 0}, {0, 0}, {1, 0, 0}, {0, 0, 1}},
      {{s, -s, -s}, {0, -1, 0}, {1, 0}, {1, 0, 0}, {0, 0, 1}},
      {{s, -s, s}, {0, -1, 0}, {1, 1}, {1, 0, 0}, {0, 0, 1}},
      {{-s, -s, s}, {0, -1, 0}, {0, 1}, {1, 0, 0}, {0, 0, 1}},
      // Right face
      {{s, -s, s}, {1, 0, 0}, {0, 0}, {0, 0, -1}, {0, 1, 0}},
      {{s, -s, -s}, {1, 0, 0}, {1, 0}, {0, 0, -1}, {0, 1, 0}},
      {{s, s, -s}, {1, 0, 0}, {1, 1}, {0, 0, -1}, {0, 1, 0}},
      {{s, s, s}, {1, 0, 0}, {0, 1}, {0, 0, -1}, {0, 1, 0}},
      // Left face
      {{-s, -s, -s}, {-1, 0, 0}, {0, 0}, {0, 0, 1}, {0, 1, 0}},
      {{-s, -s, s}, {-1, 0, 0}, {1, 0}, {0, 0, 1}, {0, 1, 0}},
      {{-s, s, s}, {-1, 0, 0}, {1, 1}, {0, 0, 1}, {0, 1, 0}},
      {{-s, s, -s}, {-1, 0, 0}, {0, 1}, {0, 0, 1}, {0, 1, 0}},
  };

  std::vector<u32> indices = {
      0,  1,  2,  2,  3,  0,  // Front
      4,  5,  6,  6,  7,  4,  // Back
      8,  9,  10, 10, 11, 8,  // Top
      12, 13, 14, 14, 15, 12, // Bottom
      16, 17, 18, 18, 19, 16, // Right
      20, 21, 22, 22, 23, 20  // Left
  };

  return CreateRef<Mesh>(vertices, indices);
}

Ref<Mesh> Mesh::CreateSphere(f32 radius, u32 segments, u32 rings) {
  std::vector<Vertex3D> vertices;
  std::vector<u32> indices;

  for (u32 y = 0; y <= rings; y++) {
    for (u32 x = 0; x <= segments; x++) {
      f32 xSegment = static_cast<f32>(x) / static_cast<f32>(segments);
      f32 ySegment = static_cast<f32>(y) / static_cast<f32>(rings);
      f32 xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
      f32 yPos = std::cos(ySegment * PI);
      f32 zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

      Vertex3D vertex;
      vertex.position = Vec3(xPos, yPos, zPos) * radius;
      vertex.normal = Vec3(xPos, yPos, zPos);
      vertex.texCoords = Vec2(xSegment, ySegment);
      vertex.tangent = glm::normalize(Vec3(-std::sin(xSegment * 2.0f * PI), 0,
                                           std::cos(xSegment * 2.0f * PI)));
      vertex.bitangent = glm::cross(vertex.normal, vertex.tangent);
      vertices.push_back(vertex);
    }
  }

  for (u32 y = 0; y < rings; y++) {
    for (u32 x = 0; x < segments; x++) {
      u32 current = y * (segments + 1) + x;
      u32 next = current + segments + 1;

      indices.push_back(current);
      indices.push_back(next);
      indices.push_back(current + 1);

      indices.push_back(current + 1);
      indices.push_back(next);
      indices.push_back(next + 1);
    }
  }

  return CreateRef<Mesh>(vertices, indices);
}

Ref<Mesh> Mesh::CreatePlane(f32 width, f32 height) {
  f32 w = width * 0.5f;
  f32 h = height * 0.5f;

  std::vector<Vertex3D> vertices = {
      {{-w, 0, -h}, {0, 1, 0}, {0, 0}, {1, 0, 0}, {0, 0, 1}},
      {{w, 0, -h}, {0, 1, 0}, {1, 0}, {1, 0, 0}, {0, 0, 1}},
      {{w, 0, h}, {0, 1, 0}, {1, 1}, {1, 0, 0}, {0, 0, 1}},
      {{-w, 0, h}, {0, 1, 0}, {0, 1}, {1, 0, 0}, {0, 0, 1}},
  };

  std::vector<u32> indices = {0, 1, 2, 2, 3, 0};

  return CreateRef<Mesh>(vertices, indices);
}

Ref<Mesh> Mesh::CreateCylinder(f32 radius, f32 height, u32 segments) {
  std::vector<Vertex3D> vertices;
  std::vector<u32> indices;

  f32 halfHeight = height * 0.5f;

  // Side vertices
  for (u32 i = 0; i <= segments; i++) {
    f32 angle = static_cast<f32>(i) / static_cast<f32>(segments) * 2.0f * PI;
    f32 x = std::cos(angle);
    f32 z = std::sin(angle);

    // Bottom vertex
    vertices.push_back({{x * radius, -halfHeight, z * radius},
                        {x, 0, z},
                        {static_cast<f32>(i) / segments, 0},
                        {-z, 0, x},
                        {0, 1, 0}});

    // Top vertex
    vertices.push_back({{x * radius, halfHeight, z * radius},
                        {x, 0, z},
                        {static_cast<f32>(i) / segments, 1},
                        {-z, 0, x},
                        {0, 1, 0}});
  }

  // Side indices
  for (u32 i = 0; i < segments; i++) {
    u32 base = i * 2;
    indices.push_back(base);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
    indices.push_back(base + 2);
    indices.push_back(base + 1);
    indices.push_back(base + 3);
  }

  // Top and bottom caps
  u32 topCenter = static_cast<u32>(vertices.size());
  vertices.push_back(
      {{0, halfHeight, 0}, {0, 1, 0}, {0.5f, 0.5f}, {1, 0, 0}, {0, 0, 1}});

  u32 bottomCenter = static_cast<u32>(vertices.size());
  vertices.push_back(
      {{0, -halfHeight, 0}, {0, -1, 0}, {0.5f, 0.5f}, {1, 0, 0}, {0, 0, -1}});

  for (u32 i = 0; i <= segments; i++) {
    f32 angle = static_cast<f32>(i) / static_cast<f32>(segments) * 2.0f * PI;
    f32 x = std::cos(angle);
    f32 z = std::sin(angle);

    // Top cap vertex
    vertices.push_back({{x * radius, halfHeight, z * radius},
                        {0, 1, 0},
                        {x * 0.5f + 0.5f, z * 0.5f + 0.5f},
                        {1, 0, 0},
                        {0, 0, 1}});

    // Bottom cap vertex
    vertices.push_back({{x * radius, -halfHeight, z * radius},
                        {0, -1, 0},
                        {x * 0.5f + 0.5f, z * 0.5f + 0.5f},
                        {1, 0, 0},
                        {0, 0, -1}});
  }

  u32 capStart = topCenter + 2;
  for (u32 i = 0; i < segments; i++) {
    // Top cap
    indices.push_back(topCenter);
    indices.push_back(capStart + i * 2);
    indices.push_back(capStart + (i + 1) * 2);

    // Bottom cap
    indices.push_back(bottomCenter);
    indices.push_back(capStart + (i + 1) * 2 + 1);
    indices.push_back(capStart + i * 2 + 1);
  }

  return CreateRef<Mesh>(vertices, indices);
}

} // namespace Gini
