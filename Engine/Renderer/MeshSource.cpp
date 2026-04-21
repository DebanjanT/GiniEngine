#include "MeshSource.h"
#include "Core/Logger.h"
#include <glad/gl.h>

namespace Gini {

MeshSource::MeshSource() = default;

MeshSource::~MeshSource() {
  if (m_VAO != 0) {
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
  }
}

void MeshSource::SetMaterialHandle(u32 index, u64 handle) {
  if (index >= m_MaterialHandles.size()) {
    m_MaterialHandles.resize(index + 1, 0);
  }
  m_MaterialHandles[index] = handle;
}

void MeshSource::UploadToGPU() {
  if (m_Uploaded || m_Vertices.empty()) return;
  
  glGenVertexArrays(1, &m_VAO);
  glGenBuffers(1, &m_VBO);
  glGenBuffers(1, &m_EBO);
  
  glBindVertexArray(m_VAO);
  
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(MeshSourceVertex),
               m_Vertices.data(), GL_STATIC_DRAW);
  
  std::vector<u32> flatIndices;
  flatIndices.reserve(m_Indices.size() * 3);
  for (const auto& idx : m_Indices) {
    flatIndices.push_back(idx.V1);
    flatIndices.push_back(idx.V2);
    flatIndices.push_back(idx.V3);
  }
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, flatIndices.size() * sizeof(u32),
               flatIndices.data(), GL_STATIC_DRAW);
  
  // Position (location 0)
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshSourceVertex),
                        (void*)offsetof(MeshSourceVertex, Position));
  
  // Normal (location 1)
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshSourceVertex),
                        (void*)offsetof(MeshSourceVertex, Normal));
  
  // TexCoord (location 2) - MUST match shader layout!
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshSourceVertex),
                        (void*)offsetof(MeshSourceVertex, Texcoord));
  
  // Tangent (location 3)
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(MeshSourceVertex),
                        (void*)offsetof(MeshSourceVertex, Tangent));
  
  // Bitangent/Binormal (location 4)
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(MeshSourceVertex),
                        (void*)offsetof(MeshSourceVertex, Binormal));
  
  glBindVertexArray(0);
  m_Uploaded = true;
}

void MeshSource::Bind() const {
  if (!m_Uploaded) {
    const_cast<MeshSource*>(this)->UploadToGPU();
  }
  glBindVertexArray(m_VAO);
}

void MeshSource::Unbind() const {
  glBindVertexArray(0);
}

void MeshSource::DrawSubmesh(u32 submeshIndex) const {
  if (submeshIndex >= m_Submeshes.size()) return;
  
  const Submesh& submesh = m_Submeshes[submeshIndex];
  glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT,
                           (void*)(submesh.BaseIndex * sizeof(u32)),
                           submesh.BaseVertex);
}

Ref<Mesh> MeshSource::CreateMesh(u32 submeshIndex) const {
  if (submeshIndex >= m_Submeshes.size()) {
    GINI_ERROR("MeshSource::CreateMesh - Invalid submesh index: ", submeshIndex);
    return nullptr;
  }
  
  const Submesh& submesh = m_Submeshes[submeshIndex];
  
  if (submesh.IsRigged && !m_BoneInfluences.empty()) {
    std::vector<SkinnedVertex3D> skinnedVertices;
    skinnedVertices.reserve(submesh.VertexCount);
    
    for (u32 i = 0; i < submesh.VertexCount; i++) {
      u32 vertexIndex = submesh.BaseVertex + i;
      const MeshSourceVertex& srcVertex = m_Vertices[vertexIndex];
      
      SkinnedVertex3D vertex;
      vertex.position = srcVertex.Position;
      vertex.normal = srcVertex.Normal;
      vertex.texCoords = srcVertex.Texcoord;
      vertex.tangent = srcVertex.Tangent;
      vertex.bitangent = srcVertex.Binormal;
      
      if (vertexIndex < m_BoneInfluences.size()) {
        const BoneInfluence& influence = m_BoneInfluences[vertexIndex];
        for (u32 j = 0; j < MAX_BONE_INFLUENCE; j++) {
          vertex.boneIDs[j] = influence.BoneIndices[j];
          vertex.boneWeights[j] = influence.Weights[j];
        }
      }
      
      skinnedVertices.push_back(vertex);
    }
    
    std::vector<u32> indices;
    indices.reserve(submesh.IndexCount);
    for (u32 i = 0; i < submesh.IndexCount / 3; i++) {
      u32 indexOffset = submesh.BaseIndex / 3 + i;
      if (indexOffset < m_Indices.size()) {
        const Index& idx = m_Indices[indexOffset];
        indices.push_back(idx.V1);
        indices.push_back(idx.V2);
        indices.push_back(idx.V3);
      }
    }
    
    auto mesh = CreateRef<Mesh>();
    mesh->CreateSkinned(skinnedVertices, indices);
    return mesh;
  }
  
  std::vector<Vertex3D> vertices;
  vertices.reserve(submesh.VertexCount);
  
  for (u32 i = 0; i < submesh.VertexCount; i++) {
    u32 vertexIndex = submesh.BaseVertex + i;
    const MeshSourceVertex& srcVertex = m_Vertices[vertexIndex];
    
    Vertex3D vertex;
    vertex.position = srcVertex.Position;
    vertex.normal = srcVertex.Normal;
    vertex.texCoords = srcVertex.Texcoord;
    vertex.tangent = srcVertex.Tangent;
    vertex.bitangent = srcVertex.Binormal;
    
    vertices.push_back(vertex);
  }
  
  std::vector<u32> indices;
  indices.reserve(submesh.IndexCount);
  for (u32 i = 0; i < submesh.IndexCount / 3; i++) {
    u32 indexOffset = submesh.BaseIndex / 3 + i;
    if (indexOffset < m_Indices.size()) {
      const Index& idx = m_Indices[indexOffset];
      indices.push_back(idx.V1);
      indices.push_back(idx.V2);
      indices.push_back(idx.V3);
    }
  }
  
  auto mesh = CreateRef<Mesh>();
  mesh->Create(vertices, indices);
  return mesh;
}

std::vector<Ref<Mesh>> MeshSource::CreateAllMeshes() const {
  std::vector<Ref<Mesh>> meshes;
  meshes.reserve(m_Submeshes.size());
  
  for (u32 i = 0; i < static_cast<u32>(m_Submeshes.size()); i++) {
    meshes.push_back(CreateMesh(i));
  }
  
  return meshes;
}

void MeshSourceLibrary::Add(u64 handle, Ref<MeshSource> meshSource) {
  m_MeshSources[handle] = meshSource;
  if (meshSource) {
    meshSource->SetHandle(handle);
  }
}

Ref<MeshSource> MeshSourceLibrary::Get(u64 handle) {
  auto it = m_MeshSources.find(handle);
  if (it != m_MeshSources.end()) {
    return it->second;
  }
  return nullptr;
}

bool MeshSourceLibrary::Exists(u64 handle) const {
  return m_MeshSources.find(handle) != m_MeshSources.end();
}

void MeshSourceLibrary::Remove(u64 handle) {
  m_MeshSources.erase(handle);
}

void MeshSourceLibrary::Clear() {
  m_MeshSources.clear();
}

} // namespace Gini
