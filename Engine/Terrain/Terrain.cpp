#include "Terrain.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cmath>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

namespace Gini {

// Terrain shader source
static const char *s_TerrainVertexShader = R"(
#version 410 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main() {
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
    v_TexCoord = a_TexCoord;
    gl_Position = u_Projection * u_View * worldPos;
}
)";

static const char *s_TerrainFragmentShader = R"(
#version 410 core
out vec4 FragColor;

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

uniform sampler2D u_Splatmap;
uniform sampler2D u_Layer0Albedo;
uniform sampler2D u_Layer1Albedo;
uniform sampler2D u_Layer2Albedo;
uniform sampler2D u_Layer3Albedo;

uniform vec2 u_Layer0Tiling;
uniform vec2 u_Layer1Tiling;
uniform vec2 u_Layer2Tiling;
uniform vec2 u_Layer3Tiling;

uniform vec3 u_Layer0Color;
uniform vec3 u_Layer1Color;
uniform vec3 u_Layer2Color;
uniform vec3 u_Layer3Color;

uniform bool u_HasLayer0Texture;
uniform bool u_HasLayer1Texture;
uniform bool u_HasLayer2Texture;
uniform bool u_HasLayer3Texture;

uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_CameraPos;

void main() {
    vec4 splat = texture(u_Splatmap, v_TexCoord);
    
    // Sample each layer with its tiling, use fallback color if no texture
    vec3 layer0 = u_HasLayer0Texture ? texture(u_Layer0Albedo, v_TexCoord * u_Layer0Tiling).rgb : u_Layer0Color;
    vec3 layer1 = u_HasLayer1Texture ? texture(u_Layer1Albedo, v_TexCoord * u_Layer1Tiling).rgb : u_Layer1Color;
    vec3 layer2 = u_HasLayer2Texture ? texture(u_Layer2Albedo, v_TexCoord * u_Layer2Tiling).rgb : u_Layer2Color;
    vec3 layer3 = u_HasLayer3Texture ? texture(u_Layer3Albedo, v_TexCoord * u_Layer3Tiling).rgb : u_Layer3Color;
    
    // Blend layers based on splatmap
    vec3 albedo = layer0 * splat.r + layer1 * splat.g + layer2 * splat.b + layer3 * splat.a;
    
    // Simple lighting
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(-u_LightDir);
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 viewDir = normalize(u_CameraPos - v_WorldPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    
    vec3 ambient = 0.3 * albedo;
    vec3 diffuse = diff * albedo * u_LightColor;
    vec3 specular = spec * 0.2 * u_LightColor;
    
    vec3 color = ambient + diffuse + specular;
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}
)";

// Simple noise function for terrain generation
static f32 Noise2D(f32 x, f32 y) {
  i32 n = static_cast<i32>(x + y * 57);
  n = (n << 13) ^ n;
  return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) /
                     1073741824.0f);
}

static f32 SmoothNoise2D(f32 x, f32 y) {
  f32 corners = (Noise2D(x - 1, y - 1) + Noise2D(x + 1, y - 1) +
                 Noise2D(x - 1, y + 1) + Noise2D(x + 1, y + 1)) /
                16.0f;
  f32 sides = (Noise2D(x - 1, y) + Noise2D(x + 1, y) + Noise2D(x, y - 1) +
               Noise2D(x, y + 1)) /
              8.0f;
  f32 center = Noise2D(x, y) / 4.0f;
  return corners + sides + center;
}

static f32 InterpolatedNoise2D(f32 x, f32 y) {
  i32 intX = static_cast<i32>(x);
  f32 fracX = x - intX;
  i32 intY = static_cast<i32>(y);
  f32 fracY = y - intY;

  f32 v1 = SmoothNoise2D(static_cast<f32>(intX), static_cast<f32>(intY));
  f32 v2 = SmoothNoise2D(static_cast<f32>(intX + 1), static_cast<f32>(intY));
  f32 v3 = SmoothNoise2D(static_cast<f32>(intX), static_cast<f32>(intY + 1));
  f32 v4 =
      SmoothNoise2D(static_cast<f32>(intX + 1), static_cast<f32>(intY + 1));

  f32 i1 = v1 * (1 - fracX) + v2 * fracX;
  f32 i2 = v3 * (1 - fracX) + v4 * fracX;

  return i1 * (1 - fracY) + i2 * fracY;
}

static f32 PerlinNoise2D(f32 x, f32 y, f32 persistence, i32 octaves) {
  f32 total = 0.0f;
  f32 frequency = 1.0f;
  f32 amplitude = 1.0f;
  f32 maxValue = 0.0f;

  for (i32 i = 0; i < octaves; i++) {
    total += InterpolatedNoise2D(x * frequency, y * frequency) * amplitude;
    maxValue += amplitude;
    amplitude *= persistence;
    frequency *= 2.0f;
  }

  return total / maxValue;
}

Terrain::Terrain(u32 width, u32 height, f32 scale)
    : m_Width(width), m_Height(height), m_Scale(scale) {

  m_Heightmap.resize(width * height, 0.0f);
  m_Splatmap.resize(width * height * 4, 0.0f);

  // Initialize first layer to 1.0 (default ground)
  for (u32 i = 0; i < width * height; i++) {
    m_Splatmap[i * 4] = 1.0f;
  }

  InitShader();
  GenerateFlat();

  GINI_INFO("Terrain created: ", width, "x", height, " scale: ", scale);
}

Terrain::~Terrain() {
  if (m_VAO)
    glDeleteVertexArrays(1, &m_VAO);
  if (m_VBO)
    glDeleteBuffers(1, &m_VBO);
  if (m_EBO)
    glDeleteBuffers(1, &m_EBO);

  for (auto &chunk : m_Chunks) {
    if (chunk.vao)
      glDeleteVertexArrays(1, &chunk.vao);
    if (chunk.vbo)
      glDeleteBuffers(1, &chunk.vbo);
    if (chunk.ebo)
      glDeleteBuffers(1, &chunk.ebo);
  }
}

void Terrain::InitShader() {
  m_Shader = Shader::Create(s_TerrainVertexShader, s_TerrainFragmentShader);
}

void Terrain::LoadHeightmap(const std::string &filepath) {
  int width, height, channels;
  stbi_set_flip_vertically_on_load(1);
  stbi_uc *data = stbi_load(filepath.c_str(), &width, &height, &channels, 1);

  if (!data) {
    GINI_ERROR("Failed to load heightmap: ", filepath);
    return;
  }

  // Resize if needed
  if (static_cast<u32>(width) != m_Width ||
      static_cast<u32>(height) != m_Height) {
    m_Width = width;
    m_Height = height;
    m_Heightmap.resize(m_Width * m_Height);
    m_Splatmap.resize(m_Width * m_Height * 4, 0.0f);
    for (u32 i = 0; i < m_Width * m_Height; i++) {
      m_Splatmap[i * 4] = 1.0f;
    }
  }

  // Convert to float heightmap
  for (u32 i = 0; i < m_Width * m_Height; i++) {
    m_Heightmap[i] = static_cast<f32>(data[i]) / 255.0f;
  }

  stbi_image_free(data);
  RegenerateMesh();

  GINI_INFO("Heightmap loaded: ", filepath);
}

void Terrain::GenerateFlat() {
  std::fill(m_Heightmap.begin(), m_Heightmap.end(), 0.0f);
  RegenerateMesh();
}

void Terrain::GenerateFromNoise(f32 frequency, f32 amplitude, i32 octaves) {
  for (u32 z = 0; z < m_Height; z++) {
    for (u32 x = 0; x < m_Width; x++) {
      f32 nx = static_cast<f32>(x) * frequency;
      f32 nz = static_cast<f32>(z) * frequency;
      m_Heightmap[z * m_Width + x] =
          PerlinNoise2D(nx, nz, 0.5f, octaves) * amplitude;
    }
  }
  RegenerateMesh();
  GINI_INFO("Terrain generated from noise");
}

void Terrain::SetHeight(u32 x, u32 z, f32 height) {
  if (x < m_Width && z < m_Height) {
    m_Heightmap[z * m_Width + x] = height;
    m_NeedsUpdate = true;
  }
}

f32 Terrain::GetHeight(u32 x, u32 z) const {
  if (x < m_Width && z < m_Height) {
    return m_Heightmap[z * m_Width + x];
  }
  return 0.0f;
}

f32 Terrain::GetHeightAtPosition(f32 worldX, f32 worldZ) const {
  // Convert world position to terrain coordinates
  f32 terrainX = (worldX - m_Position.x) / m_Scale;
  f32 terrainZ = (worldZ - m_Position.z) / m_Scale;

  if (terrainX < 0 || terrainX >= m_Width - 1 || terrainZ < 0 ||
      terrainZ >= m_Height - 1) {
    return 0.0f;
  }

  // Bilinear interpolation
  u32 x0 = static_cast<u32>(terrainX);
  u32 z0 = static_cast<u32>(terrainZ);
  u32 x1 = x0 + 1;
  u32 z1 = z0 + 1;

  f32 xFrac = terrainX - x0;
  f32 zFrac = terrainZ - z0;

  f32 h00 = GetHeight(x0, z0);
  f32 h10 = GetHeight(x1, z0);
  f32 h01 = GetHeight(x0, z1);
  f32 h11 = GetHeight(x1, z1);

  f32 h0 = h00 * (1 - xFrac) + h10 * xFrac;
  f32 h1 = h01 * (1 - xFrac) + h11 * xFrac;

  return (h0 * (1 - zFrac) + h1 * zFrac) * m_HeightScale + m_Position.y;
}

Vec3 Terrain::GetNormalAtPosition(f32 worldX, f32 worldZ) const {
  f32 delta = m_Scale;
  f32 hL = GetHeightAtPosition(worldX - delta, worldZ);
  f32 hR = GetHeightAtPosition(worldX + delta, worldZ);
  f32 hD = GetHeightAtPosition(worldX, worldZ - delta);
  f32 hU = GetHeightAtPosition(worldX, worldZ + delta);

  Vec3 normal = glm::normalize(Vec3(hL - hR, 2.0f * delta, hD - hU));
  return normal;
}

void Terrain::AddLayer(const TerrainLayer &layer) {
  if (m_Layers.size() < 4) {
    m_Layers.push_back(layer);
  } else {
    GINI_WARN("Maximum 4 terrain layers supported");
  }
}

void Terrain::RemoveLayer(u32 index) {
  if (index < m_Layers.size()) {
    m_Layers.erase(m_Layers.begin() + index);
  }
}

void Terrain::SetSplatmapPixel(u32 x, u32 z, u32 layerIndex, f32 weight) {
  if (x < m_Width && z < m_Height && layerIndex < 4) {
    u32 idx = (z * m_Width + x) * 4 + layerIndex;
    m_Splatmap[idx] = std::clamp(weight, 0.0f, 1.0f);
  }
}

f32 Terrain::GetSplatmapPixel(u32 x, u32 z, u32 layerIndex) const {
  if (x < m_Width && z < m_Height && layerIndex < 4) {
    return m_Splatmap[(z * m_Width + x) * 4 + layerIndex];
  }
  return 0.0f;
}

void Terrain::UpdateSplatmapTexture() {
  if (!m_SplatmapTexture) {
    m_SplatmapTexture = Texture2D::Create(m_Width, m_Height);
  }

  // Convert float splatmap to RGBA8
  std::vector<u8> data(m_Width * m_Height * 4);
  for (u32 i = 0; i < m_Width * m_Height; i++) {
    data[i * 4 + 0] = static_cast<u8>(m_Splatmap[i * 4 + 0] * 255.0f);
    data[i * 4 + 1] = static_cast<u8>(m_Splatmap[i * 4 + 1] * 255.0f);
    data[i * 4 + 2] = static_cast<u8>(m_Splatmap[i * 4 + 2] * 255.0f);
    data[i * 4 + 3] = static_cast<u8>(m_Splatmap[i * 4 + 3] * 255.0f);
  }

  m_SplatmapTexture->SetData(data.data(), static_cast<u32>(data.size()));
}

void Terrain::PaintHeight(f32 worldX, f32 worldZ, f32 radius, f32 strength,
                          bool raise) {
  f32 terrainX = (worldX - m_Position.x) / m_Scale;
  f32 terrainZ = (worldZ - m_Position.z) / m_Scale;
  f32 terrainRadius = radius / m_Scale;

  i32 minX = std::max(0, static_cast<i32>(terrainX - terrainRadius));
  i32 maxX = std::min(static_cast<i32>(m_Width - 1),
                      static_cast<i32>(terrainX + terrainRadius));
  i32 minZ = std::max(0, static_cast<i32>(terrainZ - terrainRadius));
  i32 maxZ = std::min(static_cast<i32>(m_Height - 1),
                      static_cast<i32>(terrainZ + terrainRadius));

  for (i32 z = minZ; z <= maxZ; z++) {
    for (i32 x = minX; x <= maxX; x++) {
      f32 dx = static_cast<f32>(x) - terrainX;
      f32 dz = static_cast<f32>(z) - terrainZ;
      f32 dist = std::sqrt(dx * dx + dz * dz);

      if (dist <= terrainRadius) {
        f32 falloff = 1.0f - (dist / terrainRadius);
        falloff = falloff * falloff; // Smooth falloff

        f32 delta = strength * falloff * (raise ? 1.0f : -1.0f);
        m_Heightmap[z * m_Width + x] += delta;
      }
    }
  }

  m_NeedsUpdate = true;
}

void Terrain::SmoothHeight(f32 worldX, f32 worldZ, f32 radius, f32 strength) {
  f32 terrainX = (worldX - m_Position.x) / m_Scale;
  f32 terrainZ = (worldZ - m_Position.z) / m_Scale;
  f32 terrainRadius = radius / m_Scale;

  i32 minX = std::max(1, static_cast<i32>(terrainX - terrainRadius));
  i32 maxX = std::min(static_cast<i32>(m_Width - 2),
                      static_cast<i32>(terrainX + terrainRadius));
  i32 minZ = std::max(1, static_cast<i32>(terrainZ - terrainRadius));
  i32 maxZ = std::min(static_cast<i32>(m_Height - 2),
                      static_cast<i32>(terrainZ + terrainRadius));

  std::vector<f32> smoothed = m_Heightmap;

  for (i32 z = minZ; z <= maxZ; z++) {
    for (i32 x = minX; x <= maxX; x++) {
      f32 dx = static_cast<f32>(x) - terrainX;
      f32 dz = static_cast<f32>(z) - terrainZ;
      f32 dist = std::sqrt(dx * dx + dz * dz);

      if (dist <= terrainRadius) {
        f32 falloff = 1.0f - (dist / terrainRadius);

        // Average of neighbors
        f32 avg = (GetHeight(x - 1, z) + GetHeight(x + 1, z) +
                   GetHeight(x, z - 1) + GetHeight(x, z + 1)) /
                  4.0f;

        f32 current = GetHeight(x, z);
        smoothed[z * m_Width + x] =
            current + (avg - current) * strength * falloff;
      }
    }
  }

  m_Heightmap = smoothed;
  m_NeedsUpdate = true;
}

void Terrain::FlattenHeight(f32 worldX, f32 worldZ, f32 radius,
                            f32 targetHeight) {
  f32 terrainX = (worldX - m_Position.x) / m_Scale;
  f32 terrainZ = (worldZ - m_Position.z) / m_Scale;
  f32 terrainRadius = radius / m_Scale;

  i32 minX = std::max(0, static_cast<i32>(terrainX - terrainRadius));
  i32 maxX = std::min(static_cast<i32>(m_Width - 1),
                      static_cast<i32>(terrainX + terrainRadius));
  i32 minZ = std::max(0, static_cast<i32>(terrainZ - terrainRadius));
  i32 maxZ = std::min(static_cast<i32>(m_Height - 1),
                      static_cast<i32>(terrainZ + terrainRadius));

  for (i32 z = minZ; z <= maxZ; z++) {
    for (i32 x = minX; x <= maxX; x++) {
      f32 dx = static_cast<f32>(x) - terrainX;
      f32 dz = static_cast<f32>(z) - terrainZ;
      f32 dist = std::sqrt(dx * dx + dz * dz);

      if (dist <= terrainRadius) {
        f32 falloff = 1.0f - (dist / terrainRadius);
        f32 current = GetHeight(x, z);
        m_Heightmap[z * m_Width + x] =
            current + (targetHeight - current) * falloff;
      }
    }
  }

  m_NeedsUpdate = true;
}

void Terrain::PaintMaterial(f32 worldX, f32 worldZ, f32 radius, f32 strength,
                            u32 layerIndex) {
  if (layerIndex >= 4)
    return;

  f32 terrainX = (worldX - m_Position.x) / m_Scale;
  f32 terrainZ = (worldZ - m_Position.z) / m_Scale;
  f32 terrainRadius = radius / m_Scale;

  i32 minX = std::max(0, static_cast<i32>(terrainX - terrainRadius));
  i32 maxX = std::min(static_cast<i32>(m_Width - 1),
                      static_cast<i32>(terrainX + terrainRadius));
  i32 minZ = std::max(0, static_cast<i32>(terrainZ - terrainRadius));
  i32 maxZ = std::min(static_cast<i32>(m_Height - 1),
                      static_cast<i32>(terrainZ + terrainRadius));

  for (i32 z = minZ; z <= maxZ; z++) {
    for (i32 x = minX; x <= maxX; x++) {
      f32 dx = static_cast<f32>(x) - terrainX;
      f32 dz = static_cast<f32>(z) - terrainZ;
      f32 dist = std::sqrt(dx * dx + dz * dz);

      if (dist <= terrainRadius) {
        f32 falloff = 1.0f - (dist / terrainRadius);
        falloff = falloff * falloff;

        u32 idx = z * m_Width + x;

        // Add to target layer
        f32 addAmount = strength * falloff;
        m_Splatmap[idx * 4 + layerIndex] =
            std::min(1.0f, m_Splatmap[idx * 4 + layerIndex] + addAmount);

        // Normalize all layers
        f32 total = 0.0f;
        for (u32 i = 0; i < 4; i++) {
          total += m_Splatmap[idx * 4 + i];
        }
        if (total > 0.0f) {
          for (u32 i = 0; i < 4; i++) {
            m_Splatmap[idx * 4 + i] /= total;
          }
        }
      }
    }
  }

  UpdateSplatmapTexture();
}

void Terrain::RegenerateMesh() {
  if (m_NeedsUpdate || m_VAO == 0) {
    CreateMesh();
    m_NeedsUpdate = false;
  }
}

void Terrain::CreateMesh() {
  // Delete old buffers
  if (m_VAO)
    glDeleteVertexArrays(1, &m_VAO);
  if (m_VBO)
    glDeleteBuffers(1, &m_VBO);
  if (m_EBO)
    glDeleteBuffers(1, &m_EBO);

  // Vertex data: position (3) + normal (3) + texcoord (2) = 8 floats per vertex
  std::vector<f32> vertices;
  vertices.reserve(m_Width * m_Height * 8);

  for (u32 z = 0; z < m_Height; z++) {
    for (u32 x = 0; x < m_Width; x++) {
      f32 height = m_Heightmap[z * m_Width + x] * m_HeightScale;

      // Position
      vertices.push_back(static_cast<f32>(x) * m_Scale);
      vertices.push_back(height);
      vertices.push_back(static_cast<f32>(z) * m_Scale);

      // Calculate normal
      f32 hL =
          (x > 0) ? m_Heightmap[z * m_Width + (x - 1)] * m_HeightScale : height;
      f32 hR = (x < m_Width - 1)
                   ? m_Heightmap[z * m_Width + (x + 1)] * m_HeightScale
                   : height;
      f32 hD =
          (z > 0) ? m_Heightmap[(z - 1) * m_Width + x] * m_HeightScale : height;
      f32 hU = (z < m_Height - 1)
                   ? m_Heightmap[(z + 1) * m_Width + x] * m_HeightScale
                   : height;

      Vec3 normal = glm::normalize(Vec3(hL - hR, 2.0f * m_Scale, hD - hU));
      vertices.push_back(normal.x);
      vertices.push_back(normal.y);
      vertices.push_back(normal.z);

      // Texture coordinates
      vertices.push_back(static_cast<f32>(x) / static_cast<f32>(m_Width - 1));
      vertices.push_back(static_cast<f32>(z) / static_cast<f32>(m_Height - 1));
    }
  }

  // Indices
  std::vector<u32> indices;
  indices.reserve((m_Width - 1) * (m_Height - 1) * 6);

  for (u32 z = 0; z < m_Height - 1; z++) {
    for (u32 x = 0; x < m_Width - 1; x++) {
      u32 topLeft = z * m_Width + x;
      u32 topRight = topLeft + 1;
      u32 bottomLeft = (z + 1) * m_Width + x;
      u32 bottomRight = bottomLeft + 1;

      // First triangle
      indices.push_back(topLeft);
      indices.push_back(bottomLeft);
      indices.push_back(topRight);

      // Second triangle
      indices.push_back(topRight);
      indices.push_back(bottomLeft);
      indices.push_back(bottomRight);
    }
  }

  m_IndexCount = static_cast<u32>(indices.size());

  // Create VAO, VBO, EBO
  glGenVertexArrays(1, &m_VAO);
  glGenBuffers(1, &m_VBO);
  glGenBuffers(1, &m_EBO);

  glBindVertexArray(m_VAO);

  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(f32), vertices.data(),
               GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(u32),
               indices.data(), GL_STATIC_DRAW);

  // Position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void *)0);

  // Normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32),
                        (void *)(3 * sizeof(f32)));

  // TexCoord
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32),
                        (void *)(6 * sizeof(f32)));

  glBindVertexArray(0);

  // Create splatmap texture
  UpdateSplatmapTexture();
}

void Terrain::Render(const Camera3D &camera) {
  if (m_NeedsUpdate) {
    RegenerateMesh();
  }

  if (!m_VAO || !m_Shader)
    return;

  m_Shader->Bind();

  // Transform
  Mat4 model = glm::translate(Mat4(1.0f), m_Position);
  m_Shader->SetMat4("u_Model", model);
  m_Shader->SetMat4("u_View", camera.GetViewMatrix());
  m_Shader->SetMat4("u_Projection", camera.GetProjectionMatrix());

  // Lighting
  m_Shader->SetVec3("u_LightDir", Vec3(-0.2f, -1.0f, -0.3f));
  m_Shader->SetVec3("u_LightColor", Vec3(1.0f));
  m_Shader->SetVec3("u_CameraPos", camera.GetPosition());

  // Splatmap
  if (m_SplatmapTexture) {
    m_SplatmapTexture->Bind(0);
    m_Shader->SetInt("u_Splatmap", 0);
  }

  // Default layer colors (grass green, sand, rock gray, snow white)
  Vec3 defaultColors[4] = {
      Vec3(0.3f, 0.5f, 0.2f),  // Layer 0: Grass green
      Vec3(0.76f, 0.7f, 0.5f), // Layer 1: Sand
      Vec3(0.4f, 0.4f, 0.4f),  // Layer 2: Rock gray
      Vec3(0.9f, 0.9f, 0.95f)  // Layer 3: Snow white
  };

  // Layer textures
  for (u32 i = 0; i < 4; i++) {
    std::string uniformName = "u_Layer" + std::to_string(i) + "Albedo";
    std::string tilingName = "u_Layer" + std::to_string(i) + "Tiling";
    std::string colorName = "u_Layer" + std::to_string(i) + "Color";
    std::string hasTexName = "u_HasLayer" + std::to_string(i) + "Texture";

    bool hasTexture = (i < m_Layers.size() && m_Layers[i].albedoMap);
    m_Shader->SetInt(hasTexName.c_str(), hasTexture ? 1 : 0);

    if (hasTexture) {
      m_Layers[i].albedoMap->Bind(1 + i);
      m_Shader->SetInt(uniformName.c_str(), 1 + i);
      m_Shader->SetVec2(tilingName.c_str(), m_Layers[i].tiling);
    } else {
      m_Shader->SetVec2(tilingName.c_str(), Vec2(1.0f));
    }

    // Set fallback color from layer or default
    Vec3 color = (i < m_Layers.size()) ? m_Layers[i].color : defaultColors[i];
    m_Shader->SetVec3(colorName.c_str(), color);
  }

  glBindVertexArray(m_VAO);
  glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void Terrain::RenderWireframe(const Camera3D &camera) {
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  Render(camera);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Terrain::SaveHeightmap(const std::string &filepath) {
  std::vector<u8> data(m_Width * m_Height);
  for (u32 i = 0; i < m_Width * m_Height; i++) {
    data[i] =
        static_cast<u8>(std::clamp(m_Heightmap[i] * 255.0f, 0.0f, 255.0f));
  }

  // TODO: Use stb_image_write to save
  GINI_INFO("Heightmap saved: ", filepath);
}

void Terrain::SaveSplatmap(const std::string &filepath) {
  // TODO: Save splatmap as RGBA image
  GINI_INFO("Splatmap saved: ", filepath);
}

bool Terrain::Raycast(const Vec3 &rayOrigin, const Vec3 &rayDir,
                      Vec3 &hitPoint) const {
  // Step-based raymarching for terrain intersection
  const f32 maxDistance = 500.0f;
  const f32 stepSize = 0.5f;

  Vec3 currentPos = rayOrigin;
  Vec3 dir = glm::normalize(rayDir);

  for (f32 t = 0.0f; t < maxDistance; t += stepSize) {
    currentPos = rayOrigin + dir * t;

    // Check if within terrain bounds
    f32 terrainX = currentPos.x - m_Position.x;
    f32 terrainZ = currentPos.z - m_Position.z;

    if (terrainX < 0 || terrainX >= (m_Width - 1) * m_Scale || terrainZ < 0 ||
        terrainZ >= (m_Height - 1) * m_Scale) {
      continue;
    }

    // Get terrain height at this position
    f32 terrainHeight = GetHeightAtPosition(currentPos.x, currentPos.z);

    // Check if ray is below terrain surface
    if (currentPos.y <= terrainHeight) {
      // Binary search for more precise intersection
      f32 low = t - stepSize;
      f32 high = t;

      for (int i = 0; i < 8; i++) {
        f32 mid = (low + high) * 0.5f;
        Vec3 midPos = rayOrigin + dir * mid;
        f32 midHeight = GetHeightAtPosition(midPos.x, midPos.z);

        if (midPos.y <= midHeight) {
          high = mid;
        } else {
          low = mid;
        }
      }

      hitPoint = rayOrigin + dir * high;
      return true;
    }
  }

  return false;
}

Ref<Terrain> Terrain::Create(u32 width, u32 height, f32 scale) {
  return CreateRef<Terrain>(width, height, scale);
}

} // namespace Gini
