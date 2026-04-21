#include "AssimpMeshImporter.h"
#include "Core/Logger.h"
#include "Renderer/Texture.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Gini {

namespace Utils {

Mat4 Mat4FromAIMatrix4x4(const aiMatrix4x4& matrix) {
  Mat4 result;
  result[0][0] = matrix.a1; result[1][0] = matrix.a2; result[2][0] = matrix.a3; result[3][0] = matrix.a4;
  result[0][1] = matrix.b1; result[1][1] = matrix.b2; result[2][1] = matrix.b3; result[3][1] = matrix.b4;
  result[0][2] = matrix.c1; result[1][2] = matrix.c2; result[2][2] = matrix.c3; result[3][2] = matrix.c4;
  result[0][3] = matrix.d1; result[1][3] = matrix.d2; result[2][3] = matrix.d3; result[3][3] = matrix.d4;
  return result;
}

} // namespace Utils

AssimpMeshImporter::AssimpMeshImporter(const std::filesystem::path& path)
    : m_Path(path) {
}

AssimpMeshImporter::AssimpMeshImporter(const std::string& path)
    : m_Path(path) {
}

MeshImportResult AssimpMeshImporter::Import(MeshImportFlags flags) {
  Ref<MeshSource> meshSource = MeshSource::Create();
  meshSource->SetFilePath(m_Path.string());
  
  GINI_INFO("Loading mesh: ", m_Path.string());
  
  u32 importFlags = 0;
  if (HasFlag(flags, MeshImportFlags::CalculateTangents))
    importFlags |= aiProcess_CalcTangentSpace;
  if (HasFlag(flags, MeshImportFlags::Triangulate))
    importFlags |= aiProcess_Triangulate;
  if (HasFlag(flags, MeshImportFlags::GenNormals))
    importFlags |= aiProcess_GenNormals;
  if (HasFlag(flags, MeshImportFlags::OptimizeMeshes))
    importFlags |= aiProcess_OptimizeMeshes;
  if (HasFlag(flags, MeshImportFlags::JoinIdenticalVertices))
    importFlags |= aiProcess_JoinIdenticalVertices;
  if (HasFlag(flags, MeshImportFlags::FlipUVs))
    importFlags |= aiProcess_FlipUVs;
  if (HasFlag(flags, MeshImportFlags::LimitBoneWeights))
    importFlags |= aiProcess_LimitBoneWeights;
  if (HasFlag(flags, MeshImportFlags::GlobalScale))
    importFlags |= aiProcess_GlobalScale;
  
  importFlags |= aiProcess_SortByPType;
  importFlags |= aiProcess_ValidateDataStructure;
  
  Assimp::Importer importer;
  importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
  
  const aiScene* scene = importer.ReadFile(m_Path.string(), importFlags);
  if (!scene) {
    return MeshImportResult::Error("Failed to load mesh file: " + m_Path.string() + " - " + importer.GetErrorString());
  }
  
  if (!scene->HasMeshes()) {
    return MeshImportResult::Error("No meshes found in file: " + m_Path.string());
  }
  
  u32 vertexCount = 0;
  u32 indexCount = 0;
  
  AABB& boundingBox = const_cast<AABB&>(meshSource->GetBoundingBox());
  boundingBox.Min = Vec3(FLT_MAX);
  boundingBox.Max = Vec3(-FLT_MAX);
  
  auto& submeshes = meshSource->GetSubmeshes();
  submeshes.reserve(scene->mNumMeshes);
  
  for (u32 m = 0; m < scene->mNumMeshes; m++) {
    aiMesh* mesh = scene->mMeshes[m];
    
    if (!mesh->HasPositions()) {
      GINI_WARN("Mesh '", mesh->mName.C_Str(), "' has no positions, skipping");
      continue;
    }
    
    if (!mesh->HasNormals()) {
      GINI_WARN("Mesh '", mesh->mName.C_Str(), "' has no normals, skipping");
      continue;
    }
    
    Submesh& submesh = submeshes.emplace_back();
    submesh.BaseVertex = vertexCount;
    submesh.BaseIndex = indexCount;
    submesh.MaterialIndex = mesh->mMaterialIndex;
    submesh.VertexCount = mesh->mNumVertices;
    submesh.IndexCount = mesh->mNumFaces * 3;
    submesh.MeshName = mesh->mName.C_Str();
    
    vertexCount += mesh->mNumVertices;
    indexCount += submesh.IndexCount;
    
    AABB& aabb = submesh.BoundingBox;
    aabb.Min = Vec3(FLT_MAX);
    aabb.Max = Vec3(-FLT_MAX);
    
    for (u32 i = 0; i < mesh->mNumVertices; i++) {
      MeshSourceVertex vertex;
      vertex.Position = Vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
      vertex.Normal = Vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
      
      aabb.Min.x = glm::min(vertex.Position.x, aabb.Min.x);
      aabb.Min.y = glm::min(vertex.Position.y, aabb.Min.y);
      aabb.Min.z = glm::min(vertex.Position.z, aabb.Min.z);
      aabb.Max.x = glm::max(vertex.Position.x, aabb.Max.x);
      aabb.Max.y = glm::max(vertex.Position.y, aabb.Max.y);
      aabb.Max.z = glm::max(vertex.Position.z, aabb.Max.z);
      
      if (mesh->HasTangentsAndBitangents()) {
        vertex.Tangent = Vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
        vertex.Binormal = Vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
      }
      
      if (mesh->HasTextureCoords(0)) {
        vertex.Texcoord = Vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
      }
      
      meshSource->GetVertices().push_back(vertex);
    }
    
    for (u32 i = 0; i < mesh->mNumFaces; i++) {
      aiFace& face = mesh->mFaces[i];
      if (face.mNumIndices != 3) continue;
      
      Index index;
      index.V1 = face.mIndices[0];
      index.V2 = face.mIndices[1];
      index.V3 = face.mIndices[2];
      meshSource->GetIndices().push_back(index);
    }
  }
  
  MeshNode& rootNode = meshSource->GetNodes().emplace_back();
  TraverseNodes(meshSource, scene->mRootNode, 0);
  
  for (const auto& submesh : meshSource->GetSubmeshes()) {
    AABB transformedAABB = submesh.BoundingBox;
    Vec3 minPt = Vec3(submesh.Transform * Vec4(transformedAABB.Min, 1.0f));
    Vec3 maxPt = Vec3(submesh.Transform * Vec4(transformedAABB.Max, 1.0f));
    
    boundingBox.Min.x = glm::min(boundingBox.Min.x, minPt.x);
    boundingBox.Min.y = glm::min(boundingBox.Min.y, minPt.y);
    boundingBox.Min.z = glm::min(boundingBox.Min.z, minPt.z);
    boundingBox.Max.x = glm::max(boundingBox.Max.x, maxPt.x);
    boundingBox.Max.y = glm::max(boundingBox.Max.y, maxPt.y);
    boundingBox.Max.z = glm::max(boundingBox.Max.z, maxPt.z);
  }
  
  auto& boneInfoMap = meshSource->GetBoneInfoMap();
  i32& boneCount = meshSource->GetBoneCount();
  
  for (u32 m = 0; m < scene->mNumMeshes; m++) {
    aiMesh* mesh = scene->mMeshes[m];
    if (m >= submeshes.size()) continue;
    
    Submesh& submesh = submeshes[m];
    
    if (mesh->mNumBones > 0) {
      submesh.IsRigged = true;
      meshSource->GetBoneInfluences().resize(meshSource->GetVertices().size());
      
      for (u32 i = 0; i < mesh->mNumBones; i++) {
        aiBone* bone = mesh->mBones[i];
        std::string boneName = bone->mName.C_Str();
        
        i32 boneIndex = -1;
        if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
          BoneInfo newBoneInfo;
          newBoneInfo.id = boneCount;
          newBoneInfo.offsetMatrix = Utils::Mat4FromAIMatrix4x4(bone->mOffsetMatrix);
          boneInfoMap[boneName] = newBoneInfo;
          boneIndex = boneCount;
          boneCount++;
        } else {
          boneIndex = boneInfoMap[boneName].id;
        }
        
        for (u32 j = 0; j < bone->mNumWeights; j++) {
          u32 vertexId = submesh.BaseVertex + bone->mWeights[j].mVertexId;
          f32 weight = bone->mWeights[j].mWeight;
          
          if (vertexId < meshSource->GetBoneInfluences().size()) {
            meshSource->GetBoneInfluences()[vertexId].AddBoneData(static_cast<u32>(boneIndex), weight);
          }
        }
      }
    }
  }
  
  for (auto& influence : meshSource->GetBoneInfluences()) {
    influence.NormalizeWeights();
  }
  
  MeshImportResult result;
  result.success = true;
  result.meshSource = meshSource;
  result.vertexCount = vertexCount;
  result.indexCount = indexCount;
  result.submeshCount = static_cast<u32>(submeshes.size());
  result.boneCount = boneCount;
  
  if (scene->HasMaterials()) {
    result.materials.reserve(scene->mNumMaterials);
    result.materialCount = scene->mNumMaterials;
    
    for (u32 i = 0; i < scene->mNumMaterials; i++) {
      aiMaterial* aiMat = scene->mMaterials[i];
      aiString matName;
      aiMat->Get(AI_MATKEY_NAME, matName);
      
      auto material = MaterialAsset::Create(matName.C_Str());
      
      aiColor3D diffuseColor(0.8f, 0.8f, 0.8f);
      if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS) {
        material->SetAlbedoColor(Vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b));
      }
      
      f32 roughness = 0.5f;
      if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
        material->SetRoughness(roughness);
      }
      
      f32 metalness = 0.0f;
      if (aiMat->Get(AI_MATKEY_REFLECTIVITY, metalness) == AI_SUCCESS) {
        material->SetMetalness(metalness < 0.9f ? 0.0f : 1.0f);
      }
      
      aiString texPath;
      
      bool hasAlbedo = aiMat->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &texPath) == AI_SUCCESS;
      if (!hasAlbedo) {
        hasAlbedo = aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS;
      }
      if (hasAlbedo) {
        GINI_INFO("  Material '", matName.C_Str(), "' albedo texture path from FBX: ", texPath.C_Str());
        
        // Check for embedded texture first
        if (auto aiTexEmbedded = scene->GetEmbeddedTexture(texPath.C_Str())) {
          GINI_INFO("  Found embedded texture, loading from memory");
          Ref<Texture2D> texture;
          if (aiTexEmbedded->mHeight == 0) {
            // Compressed format (PNG/JPG) - mWidth is the size in bytes
            texture = Texture2D::CreateFromMemory(
                reinterpret_cast<const unsigned char*>(aiTexEmbedded->pcData),
                aiTexEmbedded->mWidth);
          } else {
            // Uncompressed BGRA format
            texture = Texture2D::CreateFromBGRA(
                reinterpret_cast<const unsigned char*>(aiTexEmbedded->pcData),
                aiTexEmbedded->mWidth, aiTexEmbedded->mHeight);
          }
          if (texture) {
            material->SetAlbedoMap(texture);
            GINI_INFO("  Embedded albedo texture loaded successfully");
          } else {
            GINI_ERROR("  Failed to load embedded albedo texture");
          }
        } else {
          std::filesystem::path parentPath = m_Path.parent_path();
          std::filesystem::path texturePath = parentPath / texPath.C_Str();
          
          if (!std::filesystem::exists(texturePath)) {
            texturePath = parentPath / std::filesystem::path(texPath.C_Str()).filename();
          }
          
          // Also try Textures subfolder
          if (!std::filesystem::exists(texturePath)) {
            texturePath = parentPath / "Textures" / std::filesystem::path(texPath.C_Str()).filename();
          }
          
          GINI_INFO("  Resolved albedo path: ", texturePath.string(), " exists: ", std::filesystem::exists(texturePath) ? "YES" : "NO");
          
          if (std::filesystem::exists(texturePath)) {
            auto texture = Texture2D::Create(texturePath.string());
            if (texture) {
              material->SetAlbedoMap(texture);
              material->SetAlbedoMapPath(texturePath.string());
              GINI_INFO("  Albedo texture loaded successfully");
            } else {
              GINI_ERROR("  Failed to create albedo texture");
            }
          }
        }
      } else {
        GINI_INFO("  Material '", matName.C_Str(), "' has no albedo texture");
      }
      
      if (aiMat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS ||
          aiMat->GetTexture(aiTextureType_HEIGHT, 0, &texPath) == AI_SUCCESS) {
        GINI_INFO("  Material '", matName.C_Str(), "' normal texture path: ", texPath.C_Str());
        
        if (auto aiTexEmbedded = scene->GetEmbeddedTexture(texPath.C_Str())) {
          Ref<Texture2D> texture;
          if (aiTexEmbedded->mHeight == 0) {
            texture = Texture2D::CreateFromMemory(
                reinterpret_cast<const unsigned char*>(aiTexEmbedded->pcData),
                aiTexEmbedded->mWidth);
          } else {
            texture = Texture2D::CreateFromBGRA(
                reinterpret_cast<const unsigned char*>(aiTexEmbedded->pcData),
                aiTexEmbedded->mWidth, aiTexEmbedded->mHeight);
          }
          if (texture) {
            material->SetNormalMap(texture);
            GINI_INFO("  Embedded normal texture loaded");
          }
        } else {
          std::filesystem::path parentPath = m_Path.parent_path();
          std::filesystem::path texturePath = parentPath / texPath.C_Str();
          
          if (!std::filesystem::exists(texturePath)) {
            texturePath = parentPath / std::filesystem::path(texPath.C_Str()).filename();
          }
          if (!std::filesystem::exists(texturePath)) {
            texturePath = parentPath / "Textures" / std::filesystem::path(texPath.C_Str()).filename();
          }
          
          if (std::filesystem::exists(texturePath)) {
            auto texture = Texture2D::Create(texturePath.string());
            if (texture) {
              material->SetNormalMap(texture);
              material->SetNormalMapPath(texturePath.string());
              GINI_INFO("  Normal texture loaded: ", texturePath.string());
            }
          }
        }
      }
      
      if (aiMat->GetTexture(aiTextureType_METALNESS, 0, &texPath) == AI_SUCCESS) {
        std::filesystem::path parentPath = m_Path.parent_path();
        std::filesystem::path texturePath = parentPath / texPath.C_Str();
        
        if (!std::filesystem::exists(texturePath)) {
          texturePath = parentPath / std::filesystem::path(texPath.C_Str()).filename();
        }
        
        if (std::filesystem::exists(texturePath)) {
          auto texture = Texture2D::Create(texturePath.string());
          if (texture) {
            material->SetMetalnessMap(texture);
            material->SetMetalnessMapPath(texturePath.string());
          }
        }
      }
      
      if (aiMat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texPath) == AI_SUCCESS ||
          aiMat->GetTexture(aiTextureType_SHININESS, 0, &texPath) == AI_SUCCESS) {
        std::filesystem::path parentPath = m_Path.parent_path();
        std::filesystem::path texturePath = parentPath / texPath.C_Str();
        
        if (!std::filesystem::exists(texturePath)) {
          texturePath = parentPath / std::filesystem::path(texPath.C_Str()).filename();
        }
        
        if (std::filesystem::exists(texturePath)) {
          auto texture = Texture2D::Create(texturePath.string());
          if (texture) {
            material->SetRoughnessMap(texture);
            material->SetRoughnessMapPath(texturePath.string());
          }
        }
      }
      
      result.materials.push_back(material);
    }
  }
  
  GINI_INFO("Mesh loaded: ", m_Path.string(), " - ", result.vertexCount, " vertices, ", 
            result.submeshCount, " submeshes, ", result.materialCount, " materials");
  
  return result;
}

void AssimpMeshImporter::TraverseNodes(Ref<MeshSource> meshSource, void* assimpNode, 
                                        u32 nodeIndex, const Mat4& parentTransform, u32 level) {
  aiNode* node = static_cast<aiNode*>(assimpNode);
  
  MeshNode& meshNode = meshSource->GetNodes()[nodeIndex];
  meshNode.Name = node->mName.C_Str();
  meshNode.LocalTransform = Utils::Mat4FromAIMatrix4x4(node->mTransformation);
  
  Mat4 worldTransform = parentTransform * meshNode.LocalTransform;
  
  for (u32 i = 0; i < node->mNumMeshes; i++) {
    u32 meshIndex = node->mMeshes[i];
    if (meshIndex < meshSource->GetSubmeshes().size()) {
      meshNode.Submeshes.push_back(meshIndex);
      
      Submesh& submesh = meshSource->GetSubmeshes()[meshIndex];
      submesh.Transform = worldTransform;
      submesh.LocalTransform = meshNode.LocalTransform;
    }
  }
  
  for (u32 i = 0; i < node->mNumChildren; i++) {
    u32 childIndex = static_cast<u32>(meshSource->GetNodes().size());
    meshNode.Children.push_back(childIndex);
    
    MeshNode& childNode = meshSource->GetNodes().emplace_back();
    childNode.Parent = nodeIndex;
    
    TraverseNodes(meshSource, node->mChildren[i], childIndex, worldTransform, level + 1);
  }
}

u32 AssimpMeshImporter::GetMeshCount() const {
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(m_Path.string(), aiProcess_Triangulate);
  if (!scene) return 0;
  return scene->mNumMeshes;
}

u32 AssimpMeshImporter::GetMaterialCount() const {
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(m_Path.string(), aiProcess_Triangulate);
  if (!scene) return 0;
  return scene->mNumMaterials;
}

u32 AssimpMeshImporter::GetAnimationCount() const {
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(m_Path.string(), aiProcess_Triangulate);
  if (!scene) return 0;
  return scene->mNumAnimations;
}

} // namespace Gini
