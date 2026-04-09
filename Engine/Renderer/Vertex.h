#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Gini {

// Improved vertex structure based on Hazel's design
struct Vertex {
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec3 Tangent;
  glm::vec3 Binormal;
  glm::vec2 Texcoord;

  // Animation data (optional)
  glm::vec4 BoneWeights = glm::vec4(0.0f);
  glm::ivec4 BoneIndices = glm::ivec4(0);

  Vertex() = default;

  Vertex(const glm::vec3 &position, const glm::vec3 &normal,
         const glm::vec2 &texcoord)
      : Position(position), Normal(normal), Texcoord(texcoord) {}
};

// Submesh structure matching Hazel's architecture
struct Submesh {
  uint32_t BaseVertex = 0;
  uint32_t BaseIndex = 0;
  uint32_t MaterialIndex = 0;
  uint32_t VertexCount = 0;
  uint32_t IndexCount = 0;

  // Transforms (Hazel-style)
  glm::mat4 Transform = glm::mat4(1.0f);      // World transform
  glm::mat4 LocalTransform = glm::mat4(1.0f); // Local transform

  // Names
  std::string NodeName;
  std::string MeshName;

  // Animation support (Hazel-style)
  bool IsRigged = false;

  // Bounding box for this submesh
  struct AABB {
    glm::vec3 Min = glm::vec3(FLT_MAX);
    glm::vec3 Max = glm::vec3(-FLT_MAX);
  } BoundingBox;
};

// Index structure for triangles
struct Index {
  uint32_t V1, V2, V3;

  Index() = default;
  Index(uint32_t v1, uint32_t v2, uint32_t v3) : V1(v1), V2(v2), V3(v3) {}
};

// Bone information for animation - use BoneInfo from Animation.h

// Bone influence for vertex animation
struct BoneInfluence {
  uint32_t BoneIndices[4] = {0, 0, 0, 0};
  float Weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  void AddBoneData(uint32_t boneIndex, float weight) {
    if (weight <= 0.0f)
      return;

    for (size_t i = 0; i < 4; i++) {
      if (Weights[i] == 0.0f) {
        BoneIndices[i] = boneIndex;
        Weights[i] = weight;
        return;
      }
    }

    // If we get here, the vertex has more than 4 bone influences
    // Find the weakest weight and replace it if the new weight is stronger
    float minWeight = Weights[0];
    size_t minIndex = 0;
    for (size_t i = 1; i < 4; i++) {
      if (Weights[i] < minWeight) {
        minWeight = Weights[i];
        minIndex = i;
      }
    }

    if (weight > minWeight) {
      BoneIndices[minIndex] = boneIndex;
      Weights[minIndex] = weight;
    }
  }

  void NormalizeWeights() {
    float sumWeights = 0.0f;
    for (size_t i = 0; i < 4; i++) {
      sumWeights += Weights[i];
    }

    if (sumWeights > 0.0f) {
      for (size_t i = 0; i < 4; i++) {
        Weights[i] /= sumWeights;
      }
    }
  }
};

} // namespace Gini
