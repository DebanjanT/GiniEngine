#pragma once

#include "Core/Types.h"
#include "Renderer/Mesh.h"
#include "Renderer/Submesh.h"
#include "Renderer/MeshNode.h"
#include "Renderer/Texture.h"
#include "Animation/Animation.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace Gini {

struct Index {
  u32 V1, V2, V3;
};

struct BoneInfluence {
  i32 BoneIndices[MAX_BONE_INFLUENCE] = {-1, -1, -1, -1};
  f32 Weights[MAX_BONE_INFLUENCE] = {0.0f};
  
  void AddBoneData(u32 boneIndex, f32 weight) {
    for (u32 i = 0; i < MAX_BONE_INFLUENCE; i++) {
      if (BoneIndices[i] < 0) {
        BoneIndices[i] = static_cast<i32>(boneIndex);
        Weights[i] = weight;
        return;
      }
    }
    u32 minIndex = 0;
    f32 minWeight = Weights[0];
    for (u32 i = 1; i < MAX_BONE_INFLUENCE; i++) {
      if (Weights[i] < minWeight) {
        minWeight = Weights[i];
        minIndex = i;
      }
    }
    if (weight > minWeight) {
      BoneIndices[minIndex] = static_cast<i32>(boneIndex);
      Weights[minIndex] = weight;
    }
  }
  
  void NormalizeWeights() {
    f32 totalWeight = 0.0f;
    for (u32 i = 0; i < MAX_BONE_INFLUENCE; i++) {
      if (BoneIndices[i] >= 0) {
        totalWeight += Weights[i];
      }
    }
    if (totalWeight > 0.0f) {
      for (u32 i = 0; i < MAX_BONE_INFLUENCE; i++) {
        if (BoneIndices[i] >= 0) {
          Weights[i] /= totalWeight;
        }
      }
    }
  }
};

struct MeshSourceVertex {
  Vec3 Position{0.0f};
  Vec3 Normal{0.0f, 1.0f, 0.0f};
  Vec3 Tangent{1.0f, 0.0f, 0.0f};
  Vec3 Binormal{0.0f, 0.0f, 1.0f};
  Vec2 Texcoord{0.0f};
};

class MaterialTable {
public:
  MaterialTable() = default;
  ~MaterialTable() = default;
  
  static Ref<MaterialTable> Create() { return CreateRef<MaterialTable>(); }
  
  void SetMaterial(u32 index, u64 materialHandle) { m_Materials[index] = materialHandle; }
  u64 GetMaterial(u32 index) const {
    auto it = m_Materials.find(index);
    return it != m_Materials.end() ? it->second : 0;
  }
  bool HasMaterial(u32 index) const { return m_Materials.find(index) != m_Materials.end(); }
  void Clear() { m_Materials.clear(); }
  
  const std::unordered_map<u32, u64>& GetAll() const { return m_Materials; }

private:
  std::unordered_map<u32, u64> m_Materials;
};

class MeshSource {
public:
  MeshSource();
  ~MeshSource();
  
  static Ref<MeshSource> Create() { return CreateRef<MeshSource>(); }
  
  bool IsValid() const { return m_VAO != 0 || !m_Vertices.empty(); }
  
  const std::vector<MeshSourceVertex>& GetVertices() const { return m_Vertices; }
  const std::vector<Index>& GetIndices() const { return m_Indices; }
  const std::vector<Submesh>& GetSubmeshes() const { return m_Submeshes; }
  const std::vector<MeshNode>& GetNodes() const { return m_Nodes; }
  const AABB& GetBoundingBox() const { return m_BoundingBox; }
  
  std::vector<MeshSourceVertex>& GetVertices() { return m_Vertices; }
  std::vector<Index>& GetIndices() { return m_Indices; }
  std::vector<Submesh>& GetSubmeshes() { return m_Submeshes; }
  std::vector<MeshNode>& GetNodes() { return m_Nodes; }
  
  bool HasSkeleton() const { return !m_BoneInfoMap.empty(); }
  const std::unordered_map<std::string, BoneInfo>& GetBoneInfoMap() const { return m_BoneInfoMap; }
  std::unordered_map<std::string, BoneInfo>& GetBoneInfoMap() { return m_BoneInfoMap; }
  i32& GetBoneCount() { return m_BoneCount; }
  
  const std::vector<BoneInfluence>& GetBoneInfluences() const { return m_BoneInfluences; }
  std::vector<BoneInfluence>& GetBoneInfluences() { return m_BoneInfluences; }
  
  void SetBoundingBox(const AABB& box) { m_BoundingBox = box; }
  
  u64 GetHandle() const { return m_Handle; }
  void SetHandle(u64 handle) { m_Handle = handle; }
  
  const std::string& GetFilePath() const { return m_FilePath; }
  void SetFilePath(const std::string& path) { m_FilePath = path; }
  
  const std::vector<u64>& GetMaterialHandles() const { return m_MaterialHandles; }
  std::vector<u64>& GetMaterialHandles() { return m_MaterialHandles; }
  void SetMaterialHandle(u32 index, u64 handle);
  
  void UploadToGPU();
  void Bind() const;
  void Unbind() const;
  void DrawSubmesh(u32 submeshIndex) const;
  
  Ref<Mesh> CreateMesh(u32 submeshIndex = 0) const;
  std::vector<Ref<Mesh>> CreateAllMeshes() const;

private:
  std::vector<MeshSourceVertex> m_Vertices;
  std::vector<Index> m_Indices;
  std::vector<Submesh> m_Submeshes;
  std::vector<MeshNode> m_Nodes;
  std::vector<BoneInfluence> m_BoneInfluences;
  std::vector<u64> m_MaterialHandles;
  
  std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
  i32 m_BoneCount = 0;
  
  AABB m_BoundingBox;
  
  u64 m_Handle = 0;
  std::string m_FilePath;
  
  u32 m_VAO = 0;
  u32 m_VBO = 0;
  u32 m_EBO = 0;
  bool m_Uploaded = false;
};

class MeshSourceLibrary {
public:
  static MeshSourceLibrary& Get() {
    static MeshSourceLibrary instance;
    return instance;
  }
  
  void Add(u64 handle, Ref<MeshSource> meshSource);
  Ref<MeshSource> Get(u64 handle);
  bool Exists(u64 handle) const;
  void Remove(u64 handle);
  void Clear();
  
private:
  MeshSourceLibrary() = default;
  std::unordered_map<u64, Ref<MeshSource>> m_MeshSources;
};

} // namespace Gini
