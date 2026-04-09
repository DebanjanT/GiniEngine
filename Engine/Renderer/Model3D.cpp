#include "Model3D.h"
#include "Core/Logger.h"
#include "Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>
#include <glad/gl.h>
#include <glm/glm.hpp>

namespace Gini {

// Assimp import flags based on Hazel's configuration
static const uint32_t s_MeshImportFlags =
    aiProcess_CalcTangentSpace // Create binormals/tangents just in case
    | aiProcess_Triangulate    // Make sure we're triangles
    | aiProcess_SortByPType    // Split meshes by primitive type
    | aiProcess_GenNormals     // Make sure we have legit normals
    | aiProcess_GenUVCoords    // Convert UVs if required
    | aiProcess_OptimizeMeshes // Batch draws where possible
    | aiProcess_JoinIdenticalVertices |
    aiProcess_LimitBoneWeights // If more than N (=4) bone weights, discard
                               // least influencing bones and renormalise sum to
                               // 1
    | aiProcess_ValidateDataStructure // Validation
    | aiProcess_GlobalScale // e.g. convert cm to m for fbx import (and other
                            // formats where cm is native)
    ;

// Utility functions for Assimp conversion
glm::mat4 Model3D::ConvertAssimpMatrix(const void *matrix) {
  const aiMatrix4x4 *aiMatrix = static_cast<const aiMatrix4x4 *>(matrix);
  glm::mat4 result;

  // Convert row-major to column-major
  result[0][0] = aiMatrix->a1;
  result[1][0] = aiMatrix->a2;
  result[2][0] = aiMatrix->a3;
  result[3][0] = aiMatrix->a4;
  result[0][1] = aiMatrix->b1;
  result[1][1] = aiMatrix->b2;
  result[2][1] = aiMatrix->b3;
  result[3][1] = aiMatrix->b4;
  result[0][2] = aiMatrix->c1;
  result[1][2] = aiMatrix->c2;
  result[2][2] = aiMatrix->c3;
  result[3][2] = aiMatrix->c4;
  result[0][3] = aiMatrix->d1;
  result[1][3] = aiMatrix->d2;
  result[2][3] = aiMatrix->d3;
  result[3][3] = aiMatrix->d4;

  return result;
}

glm::vec3 Model3D::ConvertAssimpVector(const void *vector) {
  const aiVector3D *aiVector = static_cast<const aiVector3D *>(vector);
  return glm::vec3(aiVector->x, aiVector->y, aiVector->z);
}

glm::vec2 Model3D::ConvertAssimpVector2D(const void *vector) {
  const aiVector3D *aiVector = static_cast<const aiVector3D *>(vector);
  return glm::vec2(aiVector->x, aiVector->y);
}

// Model3D implementation
Model3D::Model3D(const std::string &filepath) : m_FilePath(filepath) {

  std::filesystem::path path(filepath);
  m_FileName = path.filename().string();

  Load();
}

bool Model3D::Load() { return LoadWithAssimp(m_FilePath); }

bool Model3D::LoadFromFile(const std::string &filepath) {
  m_FilePath = filepath;
  std::filesystem::path path(filepath);
  m_FileName = path.filename().string();

  return Load();
}

bool Model3D::LoadWithAssimp(const std::string &filepath) {
  GINI_INFO("Loading model: {}", filepath);

  Assimp::Importer importer;
  importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

  const aiScene *scene = importer.ReadFile(filepath, s_MeshImportFlags);
  if (!scene) {
    GINI_ERROR("Failed to load model file: {}", filepath);
    GINI_ERROR("Assimp error: {}", importer.GetErrorString());
    return false;
  }

  if (!scene->HasMeshes()) {
    GINI_WARN("Model file has no meshes: {}", filepath);
    m_IsValid = true; // Still valid, just no meshes
    return true;
  }

  ProcessAssimpScene((void *)scene, filepath);
  ProcessAssimpMaterials((void *)scene, filepath);
  ProcessAssimpAnimations((void *)scene);

  CalculateBoundingBox();
  CreateRenderingMeshes();

  m_IsValid = true;
  GINI_INFO("Successfully loaded model: {} ({} meshes, {} materials)",
            m_FileName, m_Submeshes.size(), m_Materials.size());

  return true;
}

void Model3D::ProcessAssimpScene(void *scenePtr, const std::string &filepath) {
  aiScene *scene = static_cast<aiScene *>(scenePtr);

  uint32_t vertexCount = 0;
  uint32_t indexCount = 0;

  // Initialize bounding box
  m_BoundingBox.Min = glm::vec3(FLT_MAX);
  m_BoundingBox.Max = glm::vec3(-FLT_MAX);

  // Reserve space for submeshes
  m_Submeshes.reserve(scene->mNumMeshes);

  // Process skeleton first (Hazel-style)
  ProcessSkeleton(scenePtr);

  // Process each mesh
  for (unsigned m = 0; m < scene->mNumMeshes; m++) {
    aiMesh *mesh = scene->mMeshes[m];

    if (!mesh->HasPositions()) {
      GINI_WARN("Mesh {} has no vertex positions - skipping!",
                mesh->mName.C_Str());
      continue;
    }

    if (!mesh->HasNormals()) {
      GINI_WARN("Mesh {} has no vertex normals - skipping!",
                mesh->mName.C_Str());
      continue;
    }

    // Create submesh
    Submesh &submesh = m_Submeshes.emplace_back();
    submesh.BaseVertex = vertexCount;
    submesh.BaseIndex = indexCount;
    submesh.MaterialIndex = mesh->mMaterialIndex;
    submesh.VertexCount = mesh->mNumVertices;
    submesh.IndexCount = mesh->mNumFaces * 3;
    submesh.MeshName = mesh->mName.C_Str();

    vertexCount += mesh->mNumVertices;
    indexCount += submesh.IndexCount;

    // Process vertices
    for (size_t i = 0; i < mesh->mNumVertices; i++) {
      Vertex vertex;

      // Position
      vertex.Position = ConvertAssimpVector(&mesh->mVertices[i]);

      // Normal
      vertex.Normal = ConvertAssimpVector(&mesh->mNormals[i]);

      // Tangent and Binormal
      if (mesh->HasTangentsAndBitangents()) {
        vertex.Tangent = ConvertAssimpVector(&mesh->mTangents[i]);
        vertex.Binormal = ConvertAssimpVector(&mesh->mBitangents[i]);
      }

      // Texture coordinates
      if (mesh->HasTextureCoords(0)) {
        vertex.Texcoord = ConvertAssimpVector2D(&mesh->mTextureCoords[0][i]);
      }

      m_Vertices.push_back(vertex);

      // Update bounding box
      m_BoundingBox.Min = glm::min(m_BoundingBox.Min, vertex.Position);
      m_BoundingBox.Max = glm::max(m_BoundingBox.Max, vertex.Position);
    }

    // Process indices
    for (size_t i = 0; i < mesh->mNumFaces; i++) {
      // Assimp triangulates, so we should have 3 indices per face
      GINI_ASSERT(mesh->mFaces[i].mNumIndices == 3, "Face must have 3 indices");

      Index index = {mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1],
                     mesh->mFaces[i].mIndices[2]};
      m_Indices.push_back(index);
    }
  }

  // Hazel-style node traversal
  if (scene->mRootNode) {
    TraverseNodes(scenePtr, scene->mRootNode, 0xffffffff);
  }

  // Process bone influences if skeleton exists
  if (m_HasSkeleton) {
    ProcessBoneInfluences(scenePtr);
  }

  // Determine mesh type (static vs skeletal)
  DetermineMeshType();
}

void Model3D::ProcessAssimpMaterials(void *scenePtr,
                                     const std::string &filepath) {
  aiScene *scene = static_cast<aiScene *>(scenePtr);

  if (!scene->HasMaterials()) {
    GINI_WARN("Model has no materials: {}", filepath);
    return;
  }

  std::filesystem::path modelPath(filepath);
  std::filesystem::path parentPath = modelPath.parent_path();

  m_Materials.resize(scene->mNumMaterials);

  for (uint32_t i = 0; i < scene->mNumMaterials; i++) {
    aiMaterial *aiMaterial = scene->mMaterials[i];
    aiString aiMaterialName = aiMaterial->GetName();

    auto material = MaterialAsset::Create(aiMaterialName.C_Str());

    GINI_DEBUG("Processing material: {} (Index: {})", aiMaterialName.C_Str(),
               i);

    // Extract material properties
    ExtractMaterialProperties(aiMaterial, material);

    // Load textures
    // Albedo/Base Color
    bool hasAlbedoMap = false;
    aiString aiTexPath;

    // Try PBR base color first
    if (aiMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &aiTexPath) ==
        AI_SUCCESS) {
      hasAlbedoMap = true;
    } else if (aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) ==
               AI_SUCCESS) {
      // Fall back to diffuse
      hasAlbedoMap = true;
    }

    if (hasAlbedoMap) {
      auto texture =
          LoadAssimpTexture(aiMaterial, "baseColor", parentPath.string());
      if (texture) {
        material->SetAlbedoMap(texture);
        material->SetAlbedoColor(glm::vec3(1.0f)); // Use full texture color
      }
    }

    // Normal map
    if (aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &aiTexPath) ==
        AI_SUCCESS) {
      auto texture =
          LoadAssimpTexture(aiMaterial, "normal", parentPath.string());
      if (texture) {
        material->SetNormalMap(texture);
      }
    }

    // Roughness map
    bool hasRoughnessMap = false;
    bool invertRoughness = false;

    // Note: aiTextureType_ROUGHNESS may not be available in all Assimp versions
    // Try different texture types for roughness
    if (aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &aiTexPath) ==
        AI_SUCCESS) {
      // Convert shininess to roughness
      hasRoughnessMap = true;
      invertRoughness = true;
    }

    if (hasRoughnessMap) {
      auto texture =
          LoadAssimpTexture(aiMaterial, "roughness", parentPath.string());
      if (texture) {
        material->SetRoughnessMap(texture, invertRoughness);
      }
    }

    // Metallic map
    if (aiMaterial->GetTexture(aiTextureType_METALNESS, 0, &aiTexPath) ==
        AI_SUCCESS) {
      auto texture =
          LoadAssimpTexture(aiMaterial, "metallic", parentPath.string());
      if (texture) {
        material->SetMetallicMap(texture);
      }
    }

    // AO map
    if (aiMaterial->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0,
                               &aiTexPath) == AI_SUCCESS) {
      auto texture = LoadAssimpTexture(aiMaterial, "ao", parentPath.string());
      if (texture) {
        material->SetAOMap(texture);
      }
    }

    m_Materials[i] = material;
  }
}

void Model3D::ExtractMaterialProperties(void *aiMaterialPtr,
                                        Ref<MaterialAsset> material) {
  struct aiMaterial *aiMaterial =
      static_cast<struct aiMaterial *>(aiMaterialPtr);

  // Albedo color
  aiColor3D aiColor;
  if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) == AI_SUCCESS) {
    material->SetAlbedoColor(glm::vec3(aiColor.r, aiColor.g, aiColor.b));
  }

  // Emission
  aiColor3D aiEmission;
  if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmission) == AI_SUCCESS) {
    material->SetEmission(aiEmission.r);
  }

  // Roughness
  float roughness = 0.4f; // Default
  if (aiMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != AI_SUCCESS) {
    // Try shininess as fallback
    float shininess = 0.0f;
    if (aiMaterial->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
      roughness = 1.0f - (shininess / 100.0f); // Convert shininess to roughness
    }
  }
  material->SetRoughness(glm::clamp(roughness, 0.0f, 1.0f));

  // Metallic
  float metallic = 0.0f;
  if (aiMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallic) != AI_SUCCESS) {
    if (aiMaterial->Get(AI_MATKEY_REFLECTIVITY, metallic) != AI_SUCCESS) {
      metallic = 0.0f;
    }
  }

  // Physically realistic materials are either metal (1.0) or not (0.0)
  if (metallic < 0.9f) {
    metallic = 0.0f;
  } else {
    metallic = 1.0f;
  }
  material->SetMetallic(metallic);

  GINI_DEBUG("Material properties - Albedo: ({:.2f}, {:.2f}, {:.2f}), "
             "Roughness: {:.2f}, Metallic: {:.2f}",
             material->GetMaterial().Albedo.r, material->GetMaterial().Albedo.g,
             material->GetMaterial().Albedo.b,
             material->GetMaterial().Roughness,
             material->GetMaterial().Metallic);
}

Ref<Texture> Model3D::LoadAssimpTexture(void *aiMaterialPtr,
                                        const char *textureType,
                                        const std::string &modelPath) {
  struct aiMaterial *aiMaterial =
      static_cast<struct aiMaterial *>(aiMaterialPtr);

  GINI_DEBUG("Loading {} texture for material", textureType);

  aiString aiTexPath;
  aiTextureType textureTypeEnum = aiTextureType_DIFFUSE; // Default

  // Determine the correct texture type
  if (strcmp(textureType, "baseColor") == 0 ||
      strcmp(textureType, "albedo") == 0) {
    textureTypeEnum = aiTextureType_BASE_COLOR;
    if (aiMaterial->GetTexture(textureTypeEnum, 0, &aiTexPath) != AI_SUCCESS) {
      textureTypeEnum = aiTextureType_DIFFUSE;
    }
  } else if (strcmp(textureType, "normal") == 0) {
    textureTypeEnum = aiTextureType_NORMALS;
  } else if (strcmp(textureType, "roughness") == 0) {
    // Try different texture types for roughness (not all Assimp versions
    // support aiTextureType_ROUGHNESS)
    if (aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &aiTexPath) ==
        AI_SUCCESS) {
      textureTypeEnum = aiTextureType_SHININESS;
    } else if (aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiTexPath) ==
               AI_SUCCESS) {
      textureTypeEnum = aiTextureType_DIFFUSE;
    }
  } else if (strcmp(textureType, "metallic") == 0) {
    // Try different texture types for metallic
    if (aiMaterial->GetTexture(aiTextureType_REFLECTION, 0, &aiTexPath) ==
        AI_SUCCESS) {
      textureTypeEnum = aiTextureType_REFLECTION;
    } else if (aiMaterial->GetTexture(aiTextureType_SPECULAR, 0, &aiTexPath) ==
               AI_SUCCESS) {
      textureTypeEnum = aiTextureType_SPECULAR;
    }
  } else if (strcmp(textureType, "ao") == 0) {
    textureTypeEnum = aiTextureType_AMBIENT_OCCLUSION;
  } else {
    textureTypeEnum = aiTextureType_DIFFUSE;
  }

  if (aiMaterial->GetTexture(textureTypeEnum, 0, &aiTexPath) != AI_SUCCESS) {
    GINI_DEBUG("No {} texture found", textureType);
    return nullptr;
  }

  std::filesystem::path modelDir =
      std::filesystem::path(modelPath).parent_path();
  std::filesystem::path texturePath = modelDir / aiTexPath.C_Str();

  // Check if texture file exists
  if (!std::filesystem::exists(texturePath)) {
    GINI_WARN("Texture file not found: {}", texturePath.string());
    return nullptr;
  }

  try {
    // Use the existing Texture2D system
    auto texture = Texture2D::Create(texturePath.string());
    if (texture) {
      GINI_DEBUG("Successfully loaded {} texture: {}", textureType,
                 texturePath.string());
      return texture;
    }
  } catch (const std::exception &e) {
    GINI_ERROR("Failed to load texture {}: {}", texturePath.string(), e.what());
  }

  return nullptr;
}

void Model3D::ProcessAssimpAnimations(void *scenePtr) {
  aiScene *scene = static_cast<aiScene *>(scenePtr);

  // Animation processing would go here
  // For now, just check if there are animations
  if (scene->HasAnimations()) {
    GINI_DEBUG("Model has {} animations", scene->mNumAnimations);
  }
}

void Model3D::CalculateBoundingBox() {
  // Already calculated during vertex processing
}

void Model3D::CreateRenderingMeshes() {
  m_Meshes.clear();

  for (const auto &submesh : m_Submeshes) {
    // Extract vertices and indices for this submesh
    std::vector<Vertex> submeshVertices;
    std::vector<Index> submeshIndices;

    submeshVertices.reserve(submesh.VertexCount);
    submeshIndices.reserve(submesh.IndexCount);

    // Copy vertices
    for (uint32_t i = 0; i < submesh.VertexCount; i++) {
      submeshVertices.push_back(m_Vertices[submesh.BaseVertex + i]);
    }

    // Copy indices (adjust for base vertex)
    for (uint32_t i = 0; i < submesh.IndexCount / 3; i++) {
      uint32_t idx = submesh.BaseIndex + i;
      Index index = m_Indices[idx];
      index.V1 -= submesh.BaseVertex;
      index.V2 -= submesh.BaseVertex;
      index.V3 -= submesh.BaseVertex;
      submeshIndices.push_back(index);
    }

    // Get material
    Ref<MaterialAsset> material = nullptr;
    if (submesh.MaterialIndex < m_Materials.size()) {
      material = m_Materials[submesh.MaterialIndex];
    }

    // Create mesh
    auto mesh = Mesh3D::Create(submeshVertices, submeshIndices, material);
    m_Meshes.push_back(mesh);
  }
}

Ref<MaterialAsset> Model3D::GetMaterial(uint32_t index) const {
  if (index < m_Materials.size()) {
    return m_Materials[index];
  }
  return nullptr;
}

// Mesh3D implementation
Mesh3D::Mesh3D(const std::vector<Vertex> &vertices,
               const std::vector<Index> &indices, Ref<MaterialAsset> material)
    : m_Vertices(vertices), m_Indices(indices), m_Material(material) {

  SetupMesh();
  CalculateBoundingBox();
}

void Mesh3D::SetupMesh() {
  // Create VAO, VBO, EBO
  glGenVertexArrays(1, &m_VAO);
  glGenBuffers(1, &m_VBO);
  glGenBuffers(1, &m_EBO);

  glBindVertexArray(m_VAO);

  // VBO
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(Vertex),
               m_Vertices.data(), GL_STATIC_DRAW);

  // EBO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(Index),
               m_Indices.data(), GL_STATIC_DRAW);

  // Vertex attributes
  // Position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, Position));

  // Normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, Normal));

  // Texcoord
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, Texcoord));

  // Tangent
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, Tangent));

  // Binormal
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, Binormal));

  // Bone weights
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, BoneWeights));

  // Bone indices
  glEnableVertexAttribArray(6);
  glVertexAttribIPointer(6, 4, GL_INT, sizeof(Vertex),
                         (void *)offsetof(Vertex, BoneIndices));

  glBindVertexArray(0);
}

void Mesh3D::CalculateBoundingBox() {
  m_BoundingBox.Min = glm::vec3(FLT_MAX);
  m_BoundingBox.Max = glm::vec3(-FLT_MAX);

  for (const auto &vertex : m_Vertices) {
    m_BoundingBox.Min = glm::min(m_BoundingBox.Min, vertex.Position);
    m_BoundingBox.Max = glm::max(m_BoundingBox.Max, vertex.Position);
  }
}

// Hazel-style node traversal implementation
void Model3D::TraverseNodes(void *scenePtr, void *nodePtr,
                            uint32_t parentIndex) {
  aiScene *scene = static_cast<aiScene *>(scenePtr);
  aiNode *node = static_cast<aiNode *>(nodePtr);

  uint32_t nodeIndex = static_cast<uint32_t>(m_Nodes.size());

  // Create node
  MeshNode &meshNode = m_Nodes.emplace_back();
  meshNode.Parent = parentIndex;
  meshNode.Name = node->mName.C_Str();
  meshNode.LocalTransform = ConvertAssimpMatrix(&node->mTransformation);

  // Add submeshes to this node
  for (uint32_t i = 0; i < node->mNumMeshes; i++) {
    meshNode.Submeshes.push_back(node->mMeshes[i]);
  }

  // Process children
  for (uint32_t i = 0; i < node->mNumChildren; i++) {
    TraverseNodes(scenePtr, node->mChildren[i], nodeIndex);
  }

  // Add this node to parent's children list
  if (parentIndex != 0xffffffff && parentIndex < m_Nodes.size()) {
    m_Nodes[parentIndex].Children.push_back(nodeIndex);
  }
}

// Hazel-style skeleton processing
void Model3D::ProcessSkeleton(void *scenePtr) {
  aiScene *scene = static_cast<aiScene *>(scenePtr);

  // For now, we'll just check if there are any bones to determine if we have a
  // skeleton In a full implementation, this would create a proper Skeleton
  // object like Hazel does
  m_HasSkeleton = false;

  for (uint32_t m = 0; m < scene->mNumMeshes; m++) {
    aiMesh *mesh = scene->mMeshes[m];
    if (mesh->mNumBones > 0) {
      m_HasSkeleton = true;

      // Extract bone info (simplified version)
      for (uint32_t i = 0; i < mesh->mNumBones; i++) {
        aiBone *bone = mesh->mBones[i];

        BoneInfo boneInfo;
        boneInfo.id = static_cast<i32>(m_BoneInfo.size());
        boneInfo.offsetMatrix = ConvertAssimpMatrix(&bone->mOffsetMatrix);

        m_BoneInfo.push_back(boneInfo);
      }
    }
  }

  GINI_DEBUG("Model {} skeleton: {}", m_FileName,
             m_HasSkeleton ? "found" : "not found");
}

// Hazel-style bone influence processing
void Model3D::ProcessBoneInfluences(void *scenePtr) {
  aiScene *scene = static_cast<aiScene *>(scenePtr);

  if (!m_HasSkeleton) {
    return;
  }

  // Initialize bone influences for all vertices
  m_BoneInfluences.resize(m_Vertices.size());

  for (uint32_t m = 0; m < scene->mNumMeshes; m++) {
    aiMesh *mesh = scene->mMeshes[m];
    Submesh &submesh = m_Submeshes[m];

    if (mesh->mNumBones > 0) {
      submesh.IsRigged = true;

      for (uint32_t i = 0; i < mesh->mNumBones; i++) {
        aiBone *bone = mesh->mBones[i];

        // Find bone index in our bone info array
        uint32_t boneInfoIndex = ~0;
        for (uint32_t j = 0; j < m_BoneInfo.size(); j++) {
          // Compare bone names (simplified - in full implementation would use
          // proper bone mapping)
          if (std::string(bone->mName.C_Str()) ==
              std::string("Bone_" + std::to_string(j))) {
            boneInfoIndex = j;
            break;
          }
        }

        if (boneInfoIndex == ~0) {
          boneInfoIndex = static_cast<uint32_t>(m_BoneInfo.size());
        }

        // Process bone weights
        for (uint32_t j = 0; j < bone->mNumWeights; j++) {
          uint32_t vertexID = submesh.BaseVertex + bone->mWeights[j].mVertexId;
          float weight = bone->mWeights[j].mWeight;

          if (vertexID < m_BoneInfluences.size()) {
            m_BoneInfluences[vertexID].AddBoneData(boneInfoIndex, weight);
          }
        }
      }
    }
  }

  // Normalize bone weights
  for (auto &boneInfluence : m_BoneInfluences) {
    float totalWeight = boneInfluence.Weights[0] + boneInfluence.Weights[1] +
                        boneInfluence.Weights[2] + boneInfluence.Weights[3];
    if (totalWeight > 0.0f) {
      for (int i = 0; i < 4; i++) {
        boneInfluence.Weights[i] /= totalWeight;
      }
    }
  }
}

// Determine if this is a static or skeletal mesh
void Model3D::DetermineMeshType() {
  if (m_HasSkeleton) {
    m_MeshType = MeshType::Skeletal;
  } else {
    m_MeshType = MeshType::Static;
  }

  GINI_DEBUG("Model {} determined as: {}", m_FileName,
             m_MeshType == MeshType::Static ? "Static" : "Skeletal");
}

void Mesh3D::Draw() const {
  glBindVertexArray(m_VAO);
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_Indices.size() * 3),
                 GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void Mesh3D::DrawInstanced(uint32_t instanceCount) const {
  glBindVertexArray(m_VAO);
  glDrawElementsInstanced(GL_TRIANGLES,
                          static_cast<GLsizei>(m_Indices.size() * 3),
                          GL_UNSIGNED_INT, 0, instanceCount);
  glBindVertexArray(0);
}

// Diligent Engine integration implementations
void Model3D::CreateDiligentBuffers() {
  // Diligent Engine not available, this is a no-op
  // OpenGL buffers are already created in the existing system
  m_HasDiligentResources = false;
}

void Model3D::CreateDiligentPipeline(Ref<DiligentPipeline> pipeline) {
  // Diligent Engine not available, this is a no-op
  // OpenGL pipeline is already set up in the existing system
  m_DiligentPipeline = pipeline;
}

void Model3D::UpdateDiligentBuffers() {
  // Diligent Engine not available, this is a no-op
  // OpenGL buffers are updated in the existing system
}

void Model3D::RenderWithDiligentPipeline(const glm::mat4 &transform) {
  // Diligent Engine not available, this is a no-op
  // OpenGL rendering is handled by the existing Renderer3D system
}

void Model3D::RegisterWithHybridManager() {
  // Diligent Engine not available, this is a no-op
  // OpenGL rendering is already functional
  GINI_INFO("Model3D using OpenGL fallback renderer: " + m_FileName);
}

} // namespace Gini
