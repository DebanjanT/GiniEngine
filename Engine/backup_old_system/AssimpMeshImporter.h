#pragma once

#include "Core/Types.h"
#include "Renderer/MeshSource.h"
#include "Renderer/MaterialAsset.h"
#include <filesystem>
#include <string>
#include <vector>

namespace Gini {

enum class MeshImportFlags : u32 {
  None = 0,
  CalculateTangents = 1 << 0,
  Triangulate = 1 << 1,
  GenNormals = 1 << 2,
  OptimizeMeshes = 1 << 3,
  JoinIdenticalVertices = 1 << 4,
  FlipUVs = 1 << 5,
  LimitBoneWeights = 1 << 6,
  GlobalScale = 1 << 7,
  
  Default = CalculateTangents | Triangulate | GenNormals | 
            OptimizeMeshes | JoinIdenticalVertices | LimitBoneWeights | GlobalScale
};

inline MeshImportFlags operator|(MeshImportFlags a, MeshImportFlags b) {
  return static_cast<MeshImportFlags>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline MeshImportFlags operator&(MeshImportFlags a, MeshImportFlags b) {
  return static_cast<MeshImportFlags>(static_cast<u32>(a) & static_cast<u32>(b));
}

inline bool HasFlag(MeshImportFlags flags, MeshImportFlags flag) {
  return (static_cast<u32>(flags) & static_cast<u32>(flag)) != 0;
}

struct MeshImportResult {
  bool success = false;
  std::string errorMessage;
  
  Ref<MeshSource> meshSource;
  std::vector<Ref<MaterialAsset>> materials;
  
  u32 vertexCount = 0;
  u32 indexCount = 0;
  u32 submeshCount = 0;
  u32 materialCount = 0;
  u32 boneCount = 0;
  
  static MeshImportResult Error(const std::string& message) {
    MeshImportResult result;
    result.success = false;
    result.errorMessage = message;
    return result;
  }
  
  static MeshImportResult Success(Ref<MeshSource> mesh) {
    MeshImportResult result;
    result.success = true;
    result.meshSource = mesh;
    return result;
  }
};

class AssimpMeshImporter {
public:
  AssimpMeshImporter(const std::filesystem::path& path);
  AssimpMeshImporter(const std::string& path);
  ~AssimpMeshImporter() = default;
  
  MeshImportResult Import(MeshImportFlags flags = MeshImportFlags::Default);
  
  u32 GetMeshCount() const;
  u32 GetMaterialCount() const;
  u32 GetAnimationCount() const;
  
private:
  void TraverseNodes(Ref<MeshSource> meshSource, void* assimpNode, u32 nodeIndex, 
                     const Mat4& parentTransform = Mat4(1.0f), u32 level = 0);
  
  std::filesystem::path m_Path;
};

} // namespace Gini
