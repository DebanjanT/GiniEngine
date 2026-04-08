#include "Model.h"
#include "Animation/Animation.h"
#include "Core/Logger.h"
#include "Renderer/Texture.h"
#include "Shader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cstdlib>

namespace Gini {

Ref<Model> Model::Create(const std::string &filepath) {
  auto model = CreateRef<Model>();
  if (!model->LoadFromFile(filepath)) {
    GINI_ERROR("Failed to load model: ", filepath);
    return nullptr;
  }
  return model;
}

bool Model::LoadFromFile(const std::string &filepath) {
  m_Filepath = filepath;

  // Extract directory
  size_t lastSlash = filepath.find_last_of("/\\");
  m_Directory =
      (lastSlash != std::string::npos) ? filepath.substr(0, lastSlash + 1) : "";

  // Determine file type
  size_t lastDot = filepath.find_last_of('.');
  if (lastDot == std::string::npos) {
    GINI_ERROR("Model file has no extension: ", filepath);
    return false;
  }

  std::string extension = filepath.substr(lastDot + 1);
  for (auto &c : extension)
    c = static_cast<char>(std::tolower(c));

  if (extension == "obj") {
    return LoadOBJ(filepath);
  } else if (extension == "gltf" || extension == "glb") {
    return LoadGLTF(filepath);
  } else {
    // Try Assimp for other formats
    return LoadOBJ(filepath); // Assimp handles many formats
  }
}

bool Model::LoadOBJ(const std::string &filepath) {
  Assimp::Importer importer;

  const aiScene *scene = importer.ReadFile(
      filepath, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                    aiProcess_FlipUVs | aiProcess_CalcTangentSpace |
                    aiProcess_JoinIdenticalVertices | aiProcess_OptimizeMeshes);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    GINI_ERROR("Assimp error: ", importer.GetErrorString());
    return false;
  }

  // Load materials first
  for (u32 i = 0; i < scene->mNumMaterials; i++) {
    Material3D material;
    aiMaterial *aiMat = scene->mMaterials[i];

    // Set reasonable defaults for PBR properties
    material.albedo = Vec3(1.0f, 1.0f, 1.0f);
    material.diffuse = material.albedo;
    material.metallic = 0.0f;
    material.roughness = 0.5f;
    material.ao = 0.5f; // Lower default AO to prevent overly dark appearance
    material.emissive = Vec3(0.0f, 0.0f, 0.0f);

    aiString name;
    if (aiMat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
      material.name = name.C_Str();
    }

    // Load colors / PBR factors
    aiColor3D color;
    bool hasColor = false;

    if (aiMat->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS) {
      material.albedo = Vec3(color.r, color.g, color.b);
      material.diffuse = material.albedo;
      hasColor = true;
    }
    if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
      material.diffuse = Vec3(color.r, color.g, color.b);
      material.albedo = material.diffuse;
      hasColor = true;
    }
    if (aiMat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
      material.specular = Vec3(color.r, color.g, color.b);
    }
    if (aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
      material.emissive = Vec3(color.r, color.g, color.b);
    }

    // If no color found, assign default colors based on material name
    if (!hasColor) {
      std::string lowerName = material.name;
      std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                     ::tolower);

      if (lowerName.find("leaf") != std::string::npos ||
          lowerName.find("leaves") != std::string::npos) {
        material.albedo = Vec3(0.1f, 0.7f, 0.1f); // Green for leaves
        material.diffuse = material.albedo;
      } else if (lowerName.find("wood") != std::string::npos ||
                 lowerName.find("bark") != std::string::npos) {
        material.albedo = Vec3(0.5f, 0.3f, 0.1f); // Brown for wood/bark
        material.diffuse = material.albedo;
      } else if (lowerName.find("dirt") != std::string::npos ||
                 lowerName.find("soil") != std::string::npos) {
        material.albedo = Vec3(0.4f, 0.3f, 0.2f); // Brown for dirt
        material.diffuse = material.albedo;
      } else if (lowerName.find("stalk") != std::string::npos ||
                 lowerName.find("stem") != std::string::npos) {
        material.albedo = Vec3(0.3f, 0.5f, 0.1f); // Light green for stalks
        material.diffuse = material.albedo;
      } else if (lowerName.find("black") != std::string::npos) {
        material.albedo =
            Vec3(0.1f, 0.1f, 0.1f); // Dark gray for black materials
        material.diffuse = material.albedo;
      } else if (lowerName.find("metal") != std::string::npos ||
                 lowerName.find("iron") != std::string::npos) {
        material.albedo = Vec3(0.7f, 0.7f, 0.7f); // Gray for metal
        material.diffuse = material.albedo;
        material.metallic = 0.8f;
        material.roughness = 0.2f;
      } else {
        material.albedo = Vec3(0.7f, 0.7f, 0.7f); // Default gray
        material.diffuse = material.albedo;
      }

      GINI_DEBUG("Applied fallback color for material '", material.name,
                 "': ", material.albedo.x, ", ", material.albedo.y, ", ",
                 material.albedo.z);
    }

    float shininess;
    if (aiMat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS &&
        shininess > 0.0f) {
      material.shininess = shininess;
      material.roughness = 1.0f - glm::clamp(shininess / 128.0f, 0.0f, 1.0f);
    }
    ai_real metallicFactor = 0.0f;
    if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor) == AI_SUCCESS) {
      material.metallic = static_cast<f32>(metallicFactor);
    }
    ai_real roughnessFactor = 0.5f;
    if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor) == AI_SUCCESS) {
      // Clamp roughness to reasonable range to prevent overly matte appearance
      f32 rawRoughness = static_cast<f32>(roughnessFactor);
      material.roughness =
          glm::clamp(rawRoughness, 0.0f, 0.8f); // Max 0.8 for some reflection
      GINI_DEBUG("  GLB roughness factor found: ", rawRoughness,
                 " clamped to: ", material.roughness);
    } else {
      GINI_DEBUG("  No GLB roughness factor found, using default: ",
                 material.roughness);
    }

    LoadMaterialTextures(material, aiMat, scene);

    // Debug: Log material property values
    GINI_DEBUG("Material '", material.name, "' properties:");
    GINI_DEBUG("  albedo: ", material.albedo.x, ", ", material.albedo.y, ", ",
               material.albedo.z);
    GINI_DEBUG("  metallic: ", material.metallic);
    GINI_DEBUG("  roughness: ", material.roughness);
    GINI_DEBUG("  ao: ", material.ao);
    GINI_DEBUG("  emissive: ", material.emissive.x, ", ", material.emissive.y,
               ", ", material.emissive.z);

    m_Materials.push_back(material);
  }

  // Process nodes
  ProcessNode(scene->mRootNode, scene);

  GINI_INFO("Loaded model: ", filepath, " (", m_Meshes.size(), " meshes, ",
            m_Materials.size(), " materials)");
  return true;
}

bool Model::LoadGLTF(const std::string &filepath) {
  // GLTF uses the same Assimp loader
  return LoadOBJ(filepath);
}

void Model::ProcessNode(const aiNode *node, const aiScene *scene) {
  // Process all meshes in this node
  for (u32 i = 0; i < node->mNumMeshes; i++) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    m_Meshes.push_back(ProcessMesh(mesh, scene));
    m_MeshMaterialIndices.push_back(static_cast<i32>(mesh->mMaterialIndex));
    GINI_INFO("Model mesh '",
              (mesh->mName.length ? mesh->mName.C_Str() : "<unnamed>"),
              "' uses material index ", static_cast<i32>(mesh->mMaterialIndex));
  }

  // Process children
  for (u32 i = 0; i < node->mNumChildren; i++) {
    ProcessNode(node->mChildren[i], scene);
  }
}

Ref<Mesh> Model::ProcessMesh(const aiMesh *mesh, const aiScene *scene) {
  std::vector<u32> indices;

  // Process indices
  for (u32 i = 0; i < mesh->mNumFaces; i++) {
    aiFace &face = mesh->mFaces[i];
    for (u32 j = 0; j < face.mNumIndices; j++) {
      indices.push_back(face.mIndices[j]);
    }
  }

  bool hasBones = mesh->mNumBones > 0;

  if (hasBones) {
    std::vector<SkinnedVertex3D> vertices;
    vertices.reserve(mesh->mNumVertices);

    for (u32 i = 0; i < mesh->mNumVertices; i++) {
      SkinnedVertex3D vertex{};
      vertex.position = Vec3(mesh->mVertices[i].x, mesh->mVertices[i].y,
                             mesh->mVertices[i].z);
      vertex.normal = Vec3(0.0f, 1.0f, 0.0f);
      vertex.tangent = Vec3(1.0f, 0.0f, 0.0f);
      vertex.bitangent = Vec3(0.0f, 0.0f, 1.0f);
      if (mesh->HasNormals()) {
        vertex.normal =
            Vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
      }
      if (mesh->mTextureCoords[0]) {
        vertex.texCoords =
            Vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
      }
      if (mesh->HasVertexColors(0)) {
        const auto &c = mesh->mColors[0][i];
        vertex.color = Vec4(c.r, c.g, c.b, c.a);
      }
      if (mesh->HasTangentsAndBitangents()) {
        vertex.tangent = Vec3(mesh->mTangents[i].x, mesh->mTangents[i].y,
                              mesh->mTangents[i].z);
        vertex.bitangent = Vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y,
                                mesh->mBitangents[i].z);
      }
      for (u32 b = 0; b < MAX_BONE_INFLUENCE; b++) {
        vertex.boneIDs[b] = -1;
        vertex.boneWeights[b] = 0.0f;
      }
      vertices.push_back(vertex);
    }

    ExtractBoneWeights(vertices, mesh);
    return CreateRef<Mesh>(vertices, indices);
  }

  std::vector<Vertex3D> vertices;
  vertices.reserve(mesh->mNumVertices);

  for (u32 i = 0; i < mesh->mNumVertices; i++) {
    Vertex3D vertex{};
    vertex.position =
        Vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
    vertex.normal = Vec3(0.0f, 1.0f, 0.0f);
    vertex.tangent = Vec3(1.0f, 0.0f, 0.0f);
    vertex.bitangent = Vec3(0.0f, 0.0f, 1.0f);
    if (mesh->HasNormals()) {
      vertex.normal =
          Vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
    }
    if (mesh->mTextureCoords[0]) {
      vertex.texCoords =
          Vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
    }
    if (mesh->HasVertexColors(0)) {
      const auto &c = mesh->mColors[0][i];
      vertex.color = Vec4(c.r, c.g, c.b, c.a);
    }
    if (mesh->HasTangentsAndBitangents()) {
      vertex.tangent = Vec3(mesh->mTangents[i].x, mesh->mTangents[i].y,
                            mesh->mTangents[i].z);
      vertex.bitangent = Vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y,
                              mesh->mBitangents[i].z);
    }
    vertices.push_back(vertex);
  }

  return CreateRef<Mesh>(vertices, indices);
}

void Model::ExtractBoneWeights(std::vector<SkinnedVertex3D> &vertices,
                               const aiMesh *mesh) {
  for (u32 boneIdx = 0; boneIdx < mesh->mNumBones; boneIdx++) {
    auto *bone = mesh->mBones[boneIdx];
    std::string boneName = bone->mName.C_Str();
    i32 boneID = -1;

    auto it = m_BoneInfoMap.find(boneName);
    if (it == m_BoneInfoMap.end()) {
      BoneInfo info;
      info.id = m_BoneCounter;
      auto &m = bone->mOffsetMatrix;
      info.offsetMatrix = Mat4(m.a1, m.b1, m.c1, m.d1, m.a2, m.b2, m.c2, m.d2,
                               m.a3, m.b3, m.c3, m.d3, m.a4, m.b4, m.c4, m.d4);
      m_BoneInfoMap[boneName] = info;
      boneID = m_BoneCounter;
      m_BoneCounter++;
    } else {
      boneID = it->second.id;
    }

    for (u32 w = 0; w < bone->mNumWeights; w++) {
      u32 vertexId = bone->mWeights[w].mVertexId;
      f32 weight = bone->mWeights[w].mWeight;
      if (vertexId >= vertices.size())
        continue;
      auto &vertex = vertices[vertexId];
      for (u32 s = 0; s < MAX_BONE_INFLUENCE; s++) {
        if (vertex.boneIDs[s] < 0) {
          vertex.boneIDs[s] = boneID;
          vertex.boneWeights[s] = weight;
          break;
        }
      }
    }
  }
}

void Model::LoadMaterialTextures(Material3D &material, const aiMaterial *aiMat,
                                 const aiScene *scene) {
  // Debug: Show all texture types and counts for this material
  GINI_DEBUG("Material '", material.name, "' texture analysis:");
  for (int texType = aiTextureType_UNKNOWN;
       texType <= aiTextureType_TRANSMISSION; ++texType) {
    u32 count = aiMat->GetTextureCount(static_cast<aiTextureType>(texType));
    if (count > 0) {
      aiString path;
      aiMat->GetTexture(static_cast<aiTextureType>(texType), 0, &path);
      GINI_DEBUG("  Type ", texType, " (", count, " textures): ", path.C_Str());
    }
  }

  auto loadTexture = [this, scene,
                      &material](const aiMaterial *mat,
                                 aiTextureType type) -> Ref<Texture2D> {
    if (mat->GetTextureCount(type) == 0)
      return nullptr;
    aiString path;
    if (mat->GetTexture(type, 0, &path) != AI_SUCCESS)
      return nullptr;

    std::string pathStr = path.C_Str();
    GINI_DEBUG("Trying to load texture type ", static_cast<int>(type),
               " path: ", pathStr);

    // GLB/GLTF: embedded textures are referenced as "*N" (index into
    // scene->mTextures)
    if (!pathStr.empty() && pathStr[0] == '*' && scene &&
        scene->mNumTextures > 0) {
      int embedIndex = std::atoi(pathStr.c_str() + 1);
      if (embedIndex >= 0 &&
          static_cast<u32>(embedIndex) < scene->mNumTextures) {
        std::string cacheKey =
            m_Filepath + "|embed|" + std::to_string(embedIndex);
        auto cached = m_TextureCache.find(cacheKey);
        if (cached != m_TextureCache.end())
          return cached->second;

        const aiTexture *aiTex = scene->mTextures[embedIndex];
        Ref<Texture2D> texture;
        if (aiTex->mHeight == 0) {
          // Compressed image bytes (PNG/JPEG); mWidth is length
          texture = Texture2D::CreateFromMemory(
              reinterpret_cast<const unsigned char *>(aiTex->pcData),
              static_cast<size_t>(aiTex->mWidth));
        } else {
          // Uncompressed BGRA8, mWidth x mHeight
          texture = Texture2D::CreateFromBGRA(
              reinterpret_cast<const unsigned char *>(aiTex->pcData),
              aiTex->mWidth, aiTex->mHeight);
        }
        if (texture && texture->GetID() != 0) {
          m_TextureCache[cacheKey] = texture;
          GINI_INFO("Embedded texture loaded idx=", embedIndex,
                    " glId=", texture->GetID());
          return texture;
        }
        GINI_ERROR("Embedded texture decode/upload failed idx=", embedIndex);
      }
      return nullptr;
    }

    // Try multiple texture search paths for FBX models
    std::vector<std::string> searchPaths;

    // 1. Same directory as the model
    searchPaths.push_back(m_Directory + pathStr);

    // 2. Textures subdirectory
    searchPaths.push_back(m_Directory + "Textures/" + pathStr);

    // 3. Try with different extensions (common cases)
    std::string basePath = pathStr;
    size_t dotPos = basePath.find_last_of('.');
    if (dotPos != std::string::npos) {
      basePath = basePath.substr(0, dotPos);
    }

    std::vector<std::string> extensions = {".png", ".jpg", ".jpeg",
                                           ".tga", ".bmp", ".tiff"};
    for (const auto &ext : extensions) {
      searchPaths.push_back(m_Directory + basePath + ext);
      searchPaths.push_back(m_Directory + "Textures/" + basePath + ext);
    }

    for (const auto &texPath : searchPaths) {
      auto it = m_TextureCache.find(texPath);
      if (it != m_TextureCache.end())
        return it->second;

      auto texture = Texture2D::Create(texPath);
      if (texture && texture->GetID() != 0) {
        m_TextureCache[texPath] = texture;
        GINI_INFO("Texture loaded '", texPath, "' glId=", texture->GetID());
        return texture;
      }
    }

    GINI_WARN("Failed to load texture '", pathStr, "' for material '",
              material.name, "'. Tried paths: ", searchPaths.size());
    return nullptr;
  };

  auto loadTextureFallback =
      [&](std::initializer_list<aiTextureType> types) -> Ref<Texture2D> {
    for (auto type : types) {
      auto tex = loadTexture(aiMat, type);
      if (tex)
        return tex;
    }
    return nullptr;
  };

  // glTF PBR-first mapping with robust fallbacks for OBJ/FBX/etc.
  material.albedoMap = loadTextureFallback(
      {aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE, aiTextureType_UNKNOWN});
  material.diffuseMap = material.albedoMap;
  material.specularMap =
      loadTextureFallback({aiTextureType_SPECULAR, aiTextureType_SHININESS});

  // Do not fallback to HEIGHT here for GLTF/GLB; that often maps to non-normal
  // packed channels and causes severe PBR artifacts.
  material.normalMap =
      loadTextureFallback({aiTextureType_NORMAL_CAMERA, aiTextureType_NORMALS,
                           aiTextureType_HEIGHT});

  material.metallicMap =
      loadTextureFallback({aiTextureType_METALNESS, aiTextureType_REFLECTION});
  material.roughnessMap = loadTextureFallback(
      {aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_SHININESS});

  material.aoMap =
      loadTextureFallback({aiTextureType_AMBIENT_OCCLUSION,
                           aiTextureType_LIGHTMAP, aiTextureType_AMBIENT});
  material.emissiveMap = loadTextureFallback({aiTextureType_EMISSIVE});

  // NOTE:
  // Some GLTF/GLB files expose non-height data through displacement/height
  // channels. Our PBR shader enables parallax mapping when heightMap is set,
  // which can cause severe speckling/fragment discard artifacts.
  // Keep parallax disabled for imported models unless we add explicit authoring
  // control and validated height source.
  material.heightMap = nullptr;

  GINI_INFO("Material '", material.name,
            "' maps: albedo=", (material.albedoMap ? "Y" : "N"),
            " normal=", (material.normalMap ? "Y" : "N"),
            " metallic=", (material.metallicMap ? "Y" : "N"),
            " roughness=", (material.roughnessMap ? "Y" : "N"),
            " ao=", (material.aoMap ? "Y" : "N"));
  if (material.albedoMap)
    GINI_INFO("  albedo tex id=", material.albedoMap->GetID());
  if (material.normalMap)
    GINI_INFO("  normal tex id=", material.normalMap->GetID());
  if (material.metallicMap)
    GINI_INFO("  metallic tex id=", material.metallicMap->GetID());
  if (material.roughnessMap)
    GINI_INFO("  roughness tex id=", material.roughnessMap->GetID());
  if (material.aoMap)
    GINI_INFO("  ao tex id=", material.aoMap->GetID());
}

void Model::Draw(Shader *shader) const {
  for (u32 i = 0; i < m_Meshes.size(); i++) {
    DrawMesh(i, shader);
  }
}

void Model::DrawMesh(u32 index, Shader *shader) const {
  if (index >= m_Meshes.size())
    return;

  // Bind material if available
  if (index < m_MeshMaterialIndices.size()) {
    i32 matIndex = m_MeshMaterialIndices[index];
    if (matIndex >= 0 && matIndex < static_cast<i32>(m_Materials.size())) {
      const Material3D &mat = m_Materials[matIndex];

      shader->SetVec3("u_Material_albedo", mat.albedo);
      shader->SetFloat("u_Material_metallic", mat.metallic);
      shader->SetFloat("u_Material_roughness", mat.roughness);
      shader->SetFloat("u_Material_ao", mat.ao);
      shader->SetVec3("u_Material_emissive", mat.emissive);

      // Bind textures
      u32 textureUnit = 0;

      if (mat.albedoMap) {
        mat.albedoMap->Bind(textureUnit);
        shader->SetInt("u_AlbedoMap", textureUnit++);
        shader->SetInt("u_HasAlbedoMap", 1);
      } else {
        shader->SetInt("u_HasAlbedoMap", 0);
      }

      if (mat.normalMap) {
        mat.normalMap->Bind(textureUnit);
        shader->SetInt("u_NormalMap", textureUnit++);
        shader->SetInt("u_HasNormalMap", 1);
      } else {
        shader->SetInt("u_HasNormalMap", 0);
      }

      if (mat.metallicMap) {
        mat.metallicMap->Bind(textureUnit);
        shader->SetInt("u_MetallicMap", textureUnit++);
        shader->SetInt("u_HasMetallicMap", 1);
      } else {
        shader->SetInt("u_HasMetallicMap", 0);
      }

      if (mat.roughnessMap) {
        mat.roughnessMap->Bind(textureUnit);
        shader->SetInt("u_RoughnessMap", textureUnit++);
        shader->SetInt("u_HasRoughnessMap", 1);
      } else {
        shader->SetInt("u_HasRoughnessMap", 0);
      }

      if (mat.aoMap) {
        mat.aoMap->Bind(textureUnit);
        shader->SetInt("u_AOMap", textureUnit++);
        shader->SetInt("u_HasAOMap", 1);
      } else {
        shader->SetInt("u_HasAOMap", 0);
      }

      if (mat.heightMap) {
        mat.heightMap->Bind(textureUnit);
        shader->SetInt("u_HeightMap", textureUnit++);
        shader->SetInt("u_HasHeightMap", 1);
        shader->SetFloat("u_HeightScale", mat.heightScale);
      } else {
        shader->SetInt("u_HasHeightMap", 0);
      }
    }
  }

  m_Meshes[index]->Draw();
}

} // namespace Gini
