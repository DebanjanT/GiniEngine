#include "Model.h"
#include "Animation/Animation.h"
#include "Shader.h"
#include "Core/Logger.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <fstream>
#include <sstream>

namespace Gini {

Ref<Model> Model::Create(const std::string& filepath) {
    auto model = CreateRef<Model>();
    if (!model->LoadFromFile(filepath)) {
        GINI_ERROR("Failed to load model: ", filepath);
        return nullptr;
    }
    return model;
}

bool Model::LoadFromFile(const std::string& filepath) {
    m_Filepath = filepath;
    
    // Extract directory
    size_t lastSlash = filepath.find_last_of("/\\");
    m_Directory = (lastSlash != std::string::npos) ? filepath.substr(0, lastSlash + 1) : "";
    
    // Determine file type
    size_t lastDot = filepath.find_last_of('.');
    if (lastDot == std::string::npos) {
        GINI_ERROR("Model file has no extension: ", filepath);
        return false;
    }
    
    std::string extension = filepath.substr(lastDot + 1);
    for (auto& c : extension) c = static_cast<char>(std::tolower(c));
    
    if (extension == "obj") {
        return LoadOBJ(filepath);
    } else if (extension == "gltf" || extension == "glb") {
        return LoadGLTF(filepath);
    } else {
        // Try Assimp for other formats
        return LoadOBJ(filepath); // Assimp handles many formats
    }
}

bool Model::LoadOBJ(const std::string& filepath) {
    Assimp::Importer importer;
    
    const aiScene* scene = importer.ReadFile(filepath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_OptimizeMeshes
    );
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        GINI_ERROR("Assimp error: ", importer.GetErrorString());
        return false;
    }
    
    // Load materials first
    for (u32 i = 0; i < scene->mNumMaterials; i++) {
        Material3D material;
        aiMaterial* aiMat = scene->mMaterials[i];
        
        aiString name;
        if (aiMat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
            material.name = name.C_Str();
        }
        
        // Load colors
        aiColor3D color;
        if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            material.diffuse = Vec3(color.r, color.g, color.b);
            material.albedo = material.diffuse;
        }
        if (aiMat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
            material.specular = Vec3(color.r, color.g, color.b);
        }
        if (aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
            material.emissive = Vec3(color.r, color.g, color.b);
        }
        
        float shininess;
        if (aiMat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
            material.shininess = shininess;
            material.roughness = 1.0f - glm::clamp(shininess / 128.0f, 0.0f, 1.0f);
        }
        
        LoadMaterialTextures(material, aiMat);
        m_Materials.push_back(material);
    }
    
    // Process nodes
    ProcessNode(scene->mRootNode, scene);
    
    GINI_INFO("Loaded model: ", filepath, " (", m_Meshes.size(), " meshes, ", m_Materials.size(), " materials)");
    return true;
}

bool Model::LoadGLTF(const std::string& filepath) {
    // GLTF uses the same Assimp loader
    return LoadOBJ(filepath);
}

void Model::ProcessNode(const aiNode* node, const aiScene* scene) {
    // Process all meshes in this node
    for (u32 i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_Meshes.push_back(ProcessMesh(mesh, scene));
        m_MeshMaterialIndices.push_back(static_cast<i32>(mesh->mMaterialIndex));
    }
    
    // Process children
    for (u32 i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene);
    }
}

Ref<Mesh> Model::ProcessMesh(const aiMesh* mesh, const aiScene* scene) {
    std::vector<u32> indices;

    // Process indices
    for (u32 i = 0; i < mesh->mNumFaces; i++) {
        aiFace& face = mesh->mFaces[i];
        for (u32 j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    bool hasBones = mesh->mNumBones > 0;

    if (hasBones) {
        std::vector<SkinnedVertex3D> vertices;
        vertices.reserve(mesh->mNumVertices);

        for (u32 i = 0; i < mesh->mNumVertices; i++) {
            SkinnedVertex3D vertex;
            vertex.position = Vec3(mesh->mVertices[i].x, mesh->mVertices[i].y,
                                   mesh->mVertices[i].z);
            if (mesh->HasNormals()) {
                vertex.normal = Vec3(mesh->mNormals[i].x, mesh->mNormals[i].y,
                                     mesh->mNormals[i].z);
            }
            if (mesh->mTextureCoords[0]) {
                vertex.texCoords =
                    Vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            }
            if (mesh->HasTangentsAndBitangents()) {
                vertex.tangent = Vec3(mesh->mTangents[i].x, mesh->mTangents[i].y,
                                      mesh->mTangents[i].z);
                vertex.bitangent = Vec3(mesh->mBitangents[i].x,
                                        mesh->mBitangents[i].y,
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
        Vertex3D vertex;
        vertex.position = Vec3(mesh->mVertices[i].x, mesh->mVertices[i].y,
                               mesh->mVertices[i].z);
        if (mesh->HasNormals()) {
            vertex.normal = Vec3(mesh->mNormals[i].x, mesh->mNormals[i].y,
                                 mesh->mNormals[i].z);
        }
        if (mesh->mTextureCoords[0]) {
            vertex.texCoords =
                Vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        if (mesh->HasTangentsAndBitangents()) {
            vertex.tangent = Vec3(mesh->mTangents[i].x, mesh->mTangents[i].y,
                                  mesh->mTangents[i].z);
            vertex.bitangent = Vec3(mesh->mBitangents[i].x,
                                    mesh->mBitangents[i].y,
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
            info.offsetMatrix = Mat4(
                m.a1, m.b1, m.c1, m.d1,
                m.a2, m.b2, m.c2, m.d2,
                m.a3, m.b3, m.c3, m.d3,
                m.a4, m.b4, m.c4, m.d4
            );
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

void Model::LoadMaterialTextures(Material3D& material, const aiMaterial* aiMat) {
    auto loadTexture = [this](const aiMaterial* mat, aiTextureType type) -> Ref<Texture2D> {
        if (mat->GetTextureCount(type) > 0) {
            aiString path;
            if (mat->GetTexture(type, 0, &path) == AI_SUCCESS) {
                std::string texPath = m_Directory + path.C_Str();
                
                // Check cache
                auto it = m_TextureCache.find(texPath);
                if (it != m_TextureCache.end()) {
                    return it->second;
                }
                
                auto texture = Texture2D::Create(texPath);
                if (texture) {
                    m_TextureCache[texPath] = texture;
                    return texture;
                }
            }
        }
        return nullptr;
    };
    
    material.diffuseMap = loadTexture(aiMat, aiTextureType_DIFFUSE);
    material.albedoMap = material.diffuseMap;
    material.specularMap = loadTexture(aiMat, aiTextureType_SPECULAR);
    material.normalMap = loadTexture(aiMat, aiTextureType_NORMALS);
    if (!material.normalMap) {
        material.normalMap = loadTexture(aiMat, aiTextureType_HEIGHT);
    }
    material.metallicMap = loadTexture(aiMat, aiTextureType_METALNESS);
    material.roughnessMap = loadTexture(aiMat, aiTextureType_DIFFUSE_ROUGHNESS);
    material.aoMap = loadTexture(aiMat, aiTextureType_AMBIENT_OCCLUSION);
    material.emissiveMap = loadTexture(aiMat, aiTextureType_EMISSIVE);
    material.heightMap = loadTexture(aiMat, aiTextureType_DISPLACEMENT);
}

void Model::Draw(Shader* shader) const {
    for (u32 i = 0; i < m_Meshes.size(); i++) {
        DrawMesh(i, shader);
    }
}

void Model::DrawMesh(u32 index, Shader* shader) const {
    if (index >= m_Meshes.size()) return;
    
    // Bind material if available
    if (index < m_MeshMaterialIndices.size()) {
        i32 matIndex = m_MeshMaterialIndices[index];
        if (matIndex >= 0 && matIndex < static_cast<i32>(m_Materials.size())) {
            const Material3D& mat = m_Materials[matIndex];
            
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
