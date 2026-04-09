#pragma once

#include "Animation/Animation.h"
#include "Material3D.h"
#include "Mesh.h"
#include "Vertex.h"
#include <filesystem>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Gini {

// Forward declarations for Diligent Engine integration
class DiligentPipeline;
class DiligentBuffer;

// Forward declarations
class Mesh3D;
class MaterialAsset;

// Improved Model class based on Hazel's MeshSource design with static/skeletal
// support
class Model3D {
public:
  static Ref<Model3D> Create(const std::string &filepath) {
    return CreateRef<Model3D>(filepath);
  }

  Model3D(const std::string &filepath);
  ~Model3D() = default;

  // Loading
  bool Load();
  bool LoadFromFile(const std::string &filepath);

  // Mesh type detection
  enum class MeshType {
    Static,  // No animation, flattened hierarchy
    Skeletal // Has animation, retains hierarchy
  };
  MeshType GetMeshType() const { return m_MeshType; }
  bool IsStatic() const { return m_MeshType == MeshType::Static; }
  bool IsSkeletal() const { return m_MeshType == MeshType::Skeletal; }

  // Vertex and index data access
  const std::vector<Vertex> &GetVertices() const { return m_Vertices; }
  const std::vector<Index> &GetIndices() const { return m_Indices; }
  const std::vector<Submesh> &GetSubmeshes() const { return m_Submeshes; }

  // Material access
  const std::vector<Ref<MaterialAsset>> &GetMaterials() const {
    return m_Materials;
  }
  Ref<MaterialAsset> GetMaterial(uint32_t index) const;
  uint32_t GetMaterialCount() const {
    return static_cast<uint32_t>(m_Materials.size());
  }

  // Mesh access for rendering
  const std::vector<Ref<Mesh3D>> &GetMeshes() const { return m_Meshes; }

  // Animation support (Hazel-style)
  bool HasSkeleton() const { return m_HasSkeleton; }
  bool IsSubmeshRigged(uint32_t submeshIndex) const {
    return submeshIndex < m_Submeshes.size()
               ? m_Submeshes[submeshIndex].IsRigged
               : false;
  }
  const std::vector<BoneInfo> &GetBoneInfo() const { return m_BoneInfo; }
  const std::vector<BoneInfluence> &GetBoneInfluences() const {
    return m_BoneInfluences;
  }

  // Animation names (deferred loading like Hazel)
  const std::vector<std::string> &GetAnimationNames() const {
    return m_AnimationNames;
  }

  // Bounding box
  struct AABB {
    glm::vec3 Min = glm::vec3(FLT_MAX);
    glm::vec3 Max = glm::vec3(-FLT_MAX);
  };
  const AABB &GetBoundingBox() const { return m_BoundingBox; }

  // File path
  const std::string &GetFilePath() const { return m_FilePath; }
  const std::string &GetFileName() const { return m_FileName; }

  // Validation
  bool IsValid() const { return m_IsValid; }

  // Diligent Engine integration methods
  void CreateDiligentBuffers();
  void CreateDiligentPipeline(Ref<DiligentPipeline> pipeline);
  void UpdateDiligentBuffers();
  void RenderWithDiligentPipeline(const glm::mat4 &transform);

  // Hybrid rendering support
  bool HasDiligentResources() const { return m_HasDiligentResources; }
  void RegisterWithHybridManager();

  // Hazel-style node hierarchy
  struct MeshNode {
    uint32_t Parent = 0xffffffff;
    std::vector<uint32_t> Children;
    std::vector<uint32_t> Submeshes;
    std::string Name;
    glm::mat4 LocalTransform = glm::mat4(1.0f);

    bool IsRoot() const { return Parent == 0xffffffff; }
  };
  MeshNode GetRootNode() const {
    return m_Nodes.empty() ? MeshNode{} : m_Nodes[0];
  }
  const std::vector<MeshNode> &GetNodes() const { return m_Nodes; }

private:
  // Core data
  std::string m_FilePath;
  std::string m_FileName;
  bool m_IsValid = false;
  MeshType m_MeshType = MeshType::Static;

  // Geometry data (Hazel-style)
  std::vector<Vertex> m_Vertices;
  std::vector<Index> m_Indices;
  std::vector<Submesh> m_Submeshes;

  // Materials
  std::vector<Ref<MaterialAsset>> m_Materials;

  // Rendering meshes
  std::vector<Ref<Mesh3D>> m_Meshes;

  // Animation data (Hazel-style)
  bool m_HasSkeleton = false;
  std::vector<BoneInfo> m_BoneInfo;
  std::vector<BoneInfluence> m_BoneInfluences;
  std::vector<std::string> m_AnimationNames; // Deferred loading like Hazel

  // Node hierarchy (Hazel-style)
  std::vector<MeshNode> m_Nodes;

  // Bounding box
  AABB m_BoundingBox;

  // Diligent Engine resources
  bool m_HasDiligentResources = false;
  Ref<DiligentBuffer> m_DiligentVertexBuffer;
  Ref<DiligentBuffer> m_DiligentIndexBuffer;
  Ref<DiligentPipeline> m_DiligentPipeline;

  // Loading methods (Hazel-style)
  bool LoadWithAssimp(const std::string &filepath);
  void ProcessAssimpScene(void *scene, const std::string &filepath);
  void ProcessAssimpMesh(void *mesh, void *scene, uint32_t meshIndex);
  void ProcessAssimpMaterials(void *scene, const std::string &filepath);
  void ProcessAssimpAnimations(void *scene);

  // Hazel-style node traversal and skeleton processing
  void TraverseNodes(void *scene, void *node, uint32_t parentIndex);
  void ProcessSkeleton(void *scene);
  void ProcessBoneInfluences(void *scene);
  void DetermineMeshType(); // Static vs Skeletal

  // Utility methods
  void CalculateBoundingBox();
  void CreateRenderingMeshes();
  glm::mat4 ConvertAssimpMatrix(const void *matrix);
  glm::vec3 ConvertAssimpVector(const void *vector);
  glm::vec2 ConvertAssimpVector2D(const void *vector);

  // Material processing (Hazel-style)
  void ExtractMaterialProperties(void *aiMaterial, Ref<MaterialAsset> material);
  Ref<Texture> LoadAssimpTexture(void *aiMaterial, const char *textureType,
                                 const std::string &modelPath);
  Ref<Texture> LoadEmbeddedTexture(void *embeddedTexture,
                                   const std::string &debugName);

  // Hazel-style animation name extraction
  void ExtractAnimationNames(void *scene);
};

// Mesh3D class for individual mesh rendering
class Mesh3D {
public:
  static Ref<Mesh3D> Create(const std::vector<Vertex> &vertices,
                            const std::vector<Index> &indices,
                            Ref<MaterialAsset> material = nullptr) {
    return CreateRef<Mesh3D>(vertices, indices, material);
  }

  Mesh3D(const std::vector<Vertex> &vertices, const std::vector<Index> &indices,
         Ref<MaterialAsset> material = nullptr);
  ~Mesh3D() = default;

  // Rendering
  void Draw() const;
  void DrawInstanced(uint32_t instanceCount) const;

  // Accessors
  Ref<MaterialAsset> GetMaterial() const { return m_Material; }
  void SetMaterial(Ref<MaterialAsset> material) { m_Material = material; }

  const std::vector<Vertex> &GetVertices() const { return m_Vertices; }
  const std::vector<Index> &GetIndices() const { return m_Indices; }

  // Bounding box
  const Model3D::AABB &GetBoundingBox() const { return m_BoundingBox; }

private:
  std::vector<Vertex> m_Vertices;
  std::vector<Index> m_Indices;
  Ref<MaterialAsset> m_Material;

  // OpenGL objects
  uint32_t m_VAO = 0;
  uint32_t m_VBO = 0;
  uint32_t m_EBO = 0;

  // Bounding box
  Model3D::AABB m_BoundingBox;

  // Initialization
  void SetupMesh();
  void CalculateBoundingBox();
};

} // namespace Gini
