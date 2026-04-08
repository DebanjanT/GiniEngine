#pragma once

#include "Core/Types.h"
#include "Renderer/Mesh.h"
#include "Renderer/Texture.h"
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations for Assimp types (outside Gini namespace)
struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;

namespace Gini {

struct BoneInfo;

struct Material3D {
  std::string name;

  // PBR properties
  Vec3 albedo = Vec3(1.0f);
  f32 metallic = 0.0f;
  f32 roughness = 0.5f;
  f32 ao = 1.0f;
  Vec3 emissive = Vec3(0.0f);
  f32 emissiveStrength = 1.0f;

  f32 heightScale = 0.05f;

  // Textures
  Ref<Texture2D> albedoMap;
  Ref<Texture2D> normalMap;
  Ref<Texture2D> metallicMap;
  Ref<Texture2D> roughnessMap;
  Ref<Texture2D> aoMap;
  Ref<Texture2D> emissiveMap;
  Ref<Texture2D> heightMap;

  // Legacy properties (for non-PBR)
  Vec3 diffuse = Vec3(1.0f);
  Vec3 specular = Vec3(1.0f);
  f32 shininess = 32.0f;
  Ref<Texture2D> diffuseMap;
  Ref<Texture2D> specularMap;
};

struct ModelNode {
  std::string name;
  Mat4 transform = Mat4(1.0f);
  std::vector<u32> meshIndices;
  std::vector<Scope<ModelNode>> children;
};

class Model {
public:
  Model() = default;
  ~Model() = default;

  bool LoadFromFile(const std::string &filepath);
  bool LoadOBJ(const std::string &filepath);
  bool LoadGLTF(const std::string &filepath);

  void Draw(class Shader *shader) const;
  void DrawMesh(u32 index, class Shader *shader) const;

  const std::vector<Ref<Mesh>> &GetMeshes() const { return m_Meshes; }
  const std::vector<Material3D> &GetMaterials() const { return m_Materials; }
  const std::vector<i32> &GetMeshMaterialIndices() const {
    return m_MeshMaterialIndices;
  }

  const std::string &GetFilepath() const { return m_Filepath; }
  const std::string &GetDirectory() const { return m_Directory; }
  bool HasBones() const { return !m_BoneInfoMap.empty(); }
  const std::unordered_map<std::string, BoneInfo> &GetBoneInfoMap() const {
    return m_BoneInfoMap;
  }
  i32 GetBoneCount() const { return m_BoneCounter; }

  static Ref<Model> Create(const std::string &filepath);

private:
  void ProcessNode(const ::aiNode *node, const ::aiScene *scene);
  Ref<Mesh> ProcessMesh(const ::aiMesh *mesh, const ::aiScene *scene);
  void ExtractBoneWeights(std::vector<SkinnedVertex3D> &vertices,
                          const ::aiMesh *mesh);
  void LoadMaterialTextures(Material3D &material, const ::aiMaterial *aiMat,
                            const ::aiScene *scene);

  std::string m_Filepath;
  std::string m_Directory;

  std::vector<Ref<Mesh>> m_Meshes;
  std::vector<Material3D> m_Materials;
  std::vector<i32> m_MeshMaterialIndices;

  std::unordered_map<std::string, Ref<Texture2D>> m_TextureCache;

  std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
  i32 m_BoneCounter = 0;
};

} // namespace Gini
