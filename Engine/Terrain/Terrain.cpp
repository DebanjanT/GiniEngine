#include "Terrain.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <stb_image_write.h>
#include <yaml-cpp/yaml.h>

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
  // Convert world position to terrain coordinates (terrain is centered at
  // origin)
  f32 halfWidth = (m_Width - 1) * m_Scale * 0.5f;
  f32 halfHeight = (m_Height - 1) * m_Scale * 0.5f;

  f32 terrainX = (worldX - m_Position.x + halfWidth) / m_Scale;
  f32 terrainZ = (worldZ - m_Position.z + halfHeight) / m_Scale;

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
  // Convert world position to terrain coordinates (terrain is centered at
  // origin)
  f32 halfWidth = (m_Width - 1) * m_Scale * 0.5f;
  f32 halfHeight = (m_Height - 1) * m_Scale * 0.5f;

  f32 terrainX = (worldX - m_Position.x + halfWidth) / m_Scale;
  f32 terrainZ = (worldZ - m_Position.z + halfHeight) / m_Scale;
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
  // Convert world position to terrain coordinates (terrain is centered at
  // origin)
  f32 halfWidth = (m_Width - 1) * m_Scale * 0.5f;
  f32 halfHeight = (m_Height - 1) * m_Scale * 0.5f;

  f32 terrainX = (worldX - m_Position.x + halfWidth) / m_Scale;
  f32 terrainZ = (worldZ - m_Position.z + halfHeight) / m_Scale;
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
  // Convert world position to terrain coordinates (terrain is centered at
  // origin)
  f32 halfWidth = (m_Width - 1) * m_Scale * 0.5f;
  f32 halfHeight = (m_Height - 1) * m_Scale * 0.5f;

  f32 terrainX = (worldX - m_Position.x + halfWidth) / m_Scale;
  f32 terrainZ = (worldZ - m_Position.z + halfHeight) / m_Scale;
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

  // Convert world position to terrain coordinates (terrain is centered at
  // origin)
  f32 halfWidth = (m_Width - 1) * m_Scale * 0.5f;
  f32 halfHeight = (m_Height - 1) * m_Scale * 0.5f;

  f32 terrainX = (worldX - m_Position.x + halfWidth) / m_Scale;
  f32 terrainZ = (worldZ - m_Position.z + halfHeight) / m_Scale;
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

  // Center the terrain around origin
  f32 halfWidth = (m_Width - 1) * m_Scale * 0.5f;
  f32 halfHeight = (m_Height - 1) * m_Scale * 0.5f;

  // Debug: find actual height range in mesh
  f32 minMeshH = 9999.0f, maxMeshH = -9999.0f;

  for (u32 z = 0; z < m_Height; z++) {
    for (u32 x = 0; x < m_Width; x++) {
      f32 height = m_Heightmap[z * m_Width + x] * m_HeightScale;
      minMeshH = std::min(minMeshH, height);
      maxMeshH = std::max(maxMeshH, height);

      // Position - centered around origin
      vertices.push_back(static_cast<f32>(x) * m_Scale - halfWidth);
      vertices.push_back(height);
      vertices.push_back(static_cast<f32>(z) * m_Scale - halfHeight);

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

  GINI_INFO("CreateMesh: HeightScale={}, mesh height range: {} to {}",
            m_HeightScale, minMeshH, maxMeshH);

  // Create splatmap texture
  UpdateSplatmapTexture();
}

void Terrain::Render(const Camera3D &camera) {
  if (m_NeedsUpdate) {
    RegenerateMesh();
  }

  if (!m_VAO || !m_Shader) {
    GINI_WARN("Terrain::Render skipped - VAO={}, Shader={}", m_VAO,
              (m_Shader ? "valid" : "null"));
    return;
  }

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

  // Save as PNG using stb_image_write
  if (stbi_write_png(filepath.c_str(), m_Width, m_Height, 1, data.data(),
                     m_Width)) {
    GINI_INFO("Heightmap saved: {}", filepath);
  } else {
    GINI_ERROR("Failed to save heightmap: {}", filepath);
  }
}

void Terrain::SaveSplatmap(const std::string &filepath) {
  std::vector<u8> data(m_Width * m_Height * 4);
  for (u32 i = 0; i < m_Width * m_Height; i++) {
    data[i * 4 + 0] = static_cast<u8>(
        std::clamp(m_Splatmap[i * 4 + 0] * 255.0f, 0.0f, 255.0f));
    data[i * 4 + 1] = static_cast<u8>(
        std::clamp(m_Splatmap[i * 4 + 1] * 255.0f, 0.0f, 255.0f));
    data[i * 4 + 2] = static_cast<u8>(
        std::clamp(m_Splatmap[i * 4 + 2] * 255.0f, 0.0f, 255.0f));
    data[i * 4 + 3] = static_cast<u8>(
        std::clamp(m_Splatmap[i * 4 + 3] * 255.0f, 0.0f, 255.0f));
  }

  if (stbi_write_png(filepath.c_str(), m_Width, m_Height, 4, data.data(),
                     m_Width * 4)) {
    GINI_INFO("Splatmap saved: {}", filepath);
  } else {
    GINI_ERROR("Failed to save splatmap: {}", filepath);
  }
}

void Terrain::SaveTerrain(const std::string &filepath) {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "Terrain" << YAML::Value << YAML::BeginMap;

  // Basic properties
  out << YAML::Key << "Width" << YAML::Value << m_Width;
  out << YAML::Key << "Height" << YAML::Value << m_Height;
  out << YAML::Key << "Scale" << YAML::Value << m_Scale;
  out << YAML::Key << "HeightScale" << YAML::Value << m_HeightScale;
  out << YAML::Key << "Position" << YAML::Value << YAML::Flow << YAML::BeginSeq
      << m_Position.x << m_Position.y << m_Position.z << YAML::EndSeq;

  // Get directory for saving heightmap/splatmap
  std::filesystem::path basePath(filepath);
  std::string baseDir = basePath.parent_path().string();
  std::string baseName = basePath.stem().string();

  // Save heightmap
  std::string heightmapPath = baseDir + "/" + baseName + "_heightmap.png";
  SaveHeightmap(heightmapPath);
  out << YAML::Key << "HeightmapFile" << YAML::Value
      << baseName + "_heightmap.png";

  // Save splatmap
  std::string splatmapPath = baseDir + "/" + baseName + "_splatmap.png";
  SaveSplatmap(splatmapPath);
  out << YAML::Key << "SplatmapFile" << YAML::Value
      << baseName + "_splatmap.png";

  // Save layer information
  out << YAML::Key << "Layers" << YAML::Value << YAML::BeginSeq;
  for (const auto &layer : m_Layers) {
    out << YAML::BeginMap;
    out << YAML::Key << "Name" << YAML::Value << layer.name;
    out << YAML::Key << "Color" << YAML::Value << YAML::Flow << YAML::BeginSeq
        << layer.color.x << layer.color.y << layer.color.z << YAML::EndSeq;
    out << YAML::Key << "Tiling" << YAML::Value << YAML::Flow << YAML::BeginSeq
        << layer.tiling.x << layer.tiling.y << YAML::EndSeq;
    out << YAML::Key << "Metallic" << YAML::Value << layer.metallic;
    out << YAML::Key << "Roughness" << YAML::Value << layer.roughness;
    // Note: Texture paths would need to be saved if textures are used
    out << YAML::EndMap;
  }
  out << YAML::EndSeq;

  out << YAML::EndMap; // Terrain
  out << YAML::EndMap; // Root

  std::ofstream fout(filepath);
  fout << out.c_str();
  fout.close();

  GINI_INFO("Terrain saved: {}", filepath);
}

void Terrain::LoadTerrain(const std::string &filepath) {
  try {
    YAML::Node data = YAML::LoadFile(filepath);
    if (!data["Terrain"]) {
      GINI_ERROR("Invalid terrain file: {}", filepath);
      return;
    }

    auto terrain = data["Terrain"];
    std::filesystem::path basePath(filepath);
    std::string baseDir = basePath.parent_path().string();

    // Load basic properties
    m_Width = terrain["Width"].as<u32>();
    m_Height = terrain["Height"].as<u32>();
    m_Scale = terrain["Scale"].as<f32>();
    m_HeightScale = terrain["HeightScale"].as<f32>();

    if (terrain["Position"]) {
      auto pos = terrain["Position"];
      m_Position = Vec3(pos[0].as<f32>(), pos[1].as<f32>(), pos[2].as<f32>());
    }

    // Check for new format with Textures section (exported terrain)
    if (terrain["Textures"]) {
      auto textures = terrain["Textures"];

      // Load heightmap from texture
      if (textures["Heightmap"]) {
        std::string heightmapPath =
            baseDir + "/" + textures["Heightmap"].as<std::string>();
        GINI_INFO("Loading heightmap from: {}", heightmapPath);
        if (std::filesystem::exists(heightmapPath)) {
          int width, height, channels;
          stbi_set_flip_vertically_on_load(0);
          u8 *heightData =
              stbi_load(heightmapPath.c_str(), &width, &height, &channels, 1);
          if (heightData) {
            m_Width = width;
            m_Height = height;
            m_Heightmap.resize(m_Width * m_Height);
            m_Splatmap.resize(m_Width * m_Height * 4, 0.0f);

            f32 minH = 1.0f, maxH = 0.0f;
            for (u32 i = 0; i < m_Width * m_Height; i++) {
              m_Heightmap[i] = static_cast<f32>(heightData[i]) / 255.0f;
              m_Splatmap[i * 4] = 1.0f; // Default first layer
              minH = std::min(minH, m_Heightmap[i]);
              maxH = std::max(maxH, m_Heightmap[i]);
            }
            stbi_image_free(heightData);
            m_NeedsUpdate = true; // Force mesh regeneration with new heightmap
            GINI_INFO("Heightmap loaded: {}x{}, height range: {} to {}", width,
                      height, minH, maxH);
          } else {
            GINI_ERROR("Failed to load heightmap image: {}", heightmapPath);
          }
        } else {
          GINI_ERROR("Heightmap file not found: {}", heightmapPath);
        }
      }
    } else {
      // Old format - load heightmap file directly
      if (terrain["HeightmapFile"]) {
        std::string heightmapPath =
            baseDir + "/" + terrain["HeightmapFile"].as<std::string>();
        LoadHeightmap(heightmapPath);
      }

      // Load splatmap
      if (terrain["SplatmapFile"]) {
        std::string splatmapPath =
            baseDir + "/" + terrain["SplatmapFile"].as<std::string>();
        int width, height, channels;
        stbi_set_flip_vertically_on_load(0);
        u8 *splatData =
            stbi_load(splatmapPath.c_str(), &width, &height, &channels, 4);
        if (splatData && width == (int)m_Width && height == (int)m_Height) {
          m_Splatmap.resize(m_Width * m_Height * 4);
          for (u32 i = 0; i < m_Width * m_Height * 4; i++) {
            m_Splatmap[i] = static_cast<f32>(splatData[i]) / 255.0f;
          }
          stbi_image_free(splatData);
          UpdateSplatmapTexture();
        }
      }
    }

    // Load layers
    m_Layers.clear();
    if (terrain["Layers"]) {
      for (const auto &layerNode : terrain["Layers"]) {
        TerrainLayer layer;
        layer.name = layerNode["Name"].as<std::string>();
        if (layerNode["Color"]) {
          auto col = layerNode["Color"];
          layer.color =
              Vec3(col[0].as<f32>(), col[1].as<f32>(), col[2].as<f32>());
        }
        if (layerNode["Tiling"]) {
          auto til = layerNode["Tiling"];
          layer.tiling = Vec2(til[0].as<f32>(), til[1].as<f32>());
        }
        if (layerNode["Metallic"])
          layer.metallic = layerNode["Metallic"].as<f32>();
        if (layerNode["Roughness"])
          layer.roughness = layerNode["Roughness"].as<f32>();
        // Load textures if paths are specified
        if (layerNode["AlbedoTexture"]) {
          std::string texPath = layerNode["AlbedoTexture"].as<std::string>();
          if (!texPath.empty() && std::filesystem::exists(texPath)) {
            layer.albedoMap = Texture2D::Create(texPath);
            GINI_INFO("Loaded layer albedo texture: {}", texPath);
          }
        }
        if (layerNode["NormalTexture"]) {
          std::string texPath = layerNode["NormalTexture"].as<std::string>();
          if (!texPath.empty() && std::filesystem::exists(texPath)) {
            layer.normalMap = Texture2D::Create(texPath);
          }
        }
        m_Layers.push_back(layer);
      }
    }

    // Ensure shader is initialized
    if (!m_Shader) {
      InitShader();
    }

    GINI_INFO("Before RegenerateMesh: HeightScale={}, Heightmap size={}",
              m_HeightScale, m_Heightmap.size());
    RegenerateMesh();
    UpdateSplatmapTexture();

    GINI_INFO("Terrain loaded: {} (Size={}x{}, Scale={}, HeightScale={}, "
              "VAO={}, IndexCount={})",
              filepath, m_Width, m_Height, m_Scale, m_HeightScale, m_VAO,
              m_IndexCount);

  } catch (const std::exception &e) {
    GINI_ERROR("Failed to load terrain: {}", e.what());
  }
}

// ============================================================================
// Unified Terrain Export System
// ============================================================================

void Terrain::ExportTerrain(const std::string &exportFolder,
                            const std::string &terrainName) {
  // Create export folder if it doesn't exist
  std::filesystem::create_directories(exportFolder);

  std::string basePath = exportFolder + "/" + terrainName;

  GINI_INFO("Exporting terrain '{}' to folder: {}", terrainName, exportFolder);

  // Export all textures
  ExportBaseColorTexture(basePath + "-basecolor.jpg");
  ExportHeightmapTexture(basePath + "-heightmap.jpg");
  ExportNormalTexture(basePath + "-normal.jpg");
  ExportRoughnessTexture(basePath + "-roughness.jpg");
  ExportAOTexture(basePath + "-ambientocclusion.jpg");
  ExportMetallicTexture(basePath + "-metallic.jpg");

  // Export mesh files
  ExportOBJMesh(basePath + ".obj");
  ExportMTLFile(basePath + ".mtl", terrainName);

  // Export material and terrain data files
  ExportGMATFile(basePath + ".gmat", terrainName);
  ExportGTerrainFile(basePath + ".gterrain", terrainName);

  GINI_INFO("Terrain export complete: {}", terrainName);
}

void Terrain::ExportBaseColorTexture(const std::string &filepath) {
  std::vector<u8> data(m_Width * m_Height * 3);

  // Default layer colors
  Vec3 defaultColors[4] = {
      Vec3(0.4f, 0.6f, 0.3f), // Green grass
      Vec3(0.6f, 0.5f, 0.4f), // Brown dirt
      Vec3(0.5f, 0.5f, 0.5f), // Gray rock
      Vec3(0.9f, 0.9f, 0.85f) // Light sand
  };

  // Load texture data for each layer that has an albedo texture
  struct LayerTextureData {
    std::vector<u8> pixels;
    int width = 0;
    int height = 0;
    int channels = 0;
    bool valid = false;
  };
  LayerTextureData layerTextures[4];

  for (u32 i = 0; i < m_Layers.size() && i < 4; i++) {
    if (m_Layers[i].albedoMap) {
      // Get texture path and load pixel data
      std::string texPath = m_Layers[i].albedoMap->GetPath();
      if (!texPath.empty()) {
        stbi_set_flip_vertically_on_load(0);
        u8 *texData =
            stbi_load(texPath.c_str(), &layerTextures[i].width,
                      &layerTextures[i].height, &layerTextures[i].channels, 3);
        if (texData) {
          layerTextures[i].pixels.assign(
              texData,
              texData + layerTextures[i].width * layerTextures[i].height * 3);
          layerTextures[i].valid = true;
          stbi_image_free(texData);
          GINI_INFO("Loaded layer {} texture: {}x{}", i, layerTextures[i].width,
                    layerTextures[i].height);
        }
      }
    }
  }

  for (u32 z = 0; z < m_Height; z++) {
    for (u32 x = 0; x < m_Width; x++) {
      u32 idx = z * m_Width + x;

      f32 weights[4] = {m_Splatmap[idx * 4 + 0], m_Splatmap[idx * 4 + 1],
                        m_Splatmap[idx * 4 + 2], m_Splatmap[idx * 4 + 3]};

      // Ensure weights sum to at least something
      f32 weightSum = weights[0] + weights[1] + weights[2] + weights[3];
      if (weightSum < 0.001f) {
        weights[0] = 1.0f;
      }

      // Calculate UV coordinates for this terrain position
      f32 u = static_cast<f32>(x) / static_cast<f32>(m_Width - 1);
      f32 v = static_cast<f32>(z) / static_cast<f32>(m_Height - 1);

      Vec3 color(0.0f);
      for (u32 i = 0; i < 4; i++) {
        if (weights[i] < 0.001f)
          continue;

        Vec3 layerColor;

        if (i < m_Layers.size() && layerTextures[i].valid) {
          // Sample texture with tiling
          Vec2 tiling = m_Layers[i].tiling;
          f32 tiledU = std::fmod(u * tiling.x, 1.0f);
          f32 tiledV = std::fmod(v * tiling.y, 1.0f);
          if (tiledU < 0)
            tiledU += 1.0f;
          if (tiledV < 0)
            tiledV += 1.0f;

          int texX = static_cast<int>(tiledU * (layerTextures[i].width - 1));
          int texY = static_cast<int>(tiledV * (layerTextures[i].height - 1));
          texX = std::clamp(texX, 0, layerTextures[i].width - 1);
          texY = std::clamp(texY, 0, layerTextures[i].height - 1);

          int texIdx = (texY * layerTextures[i].width + texX) * 3;
          layerColor.r = layerTextures[i].pixels[texIdx + 0] / 255.0f;
          layerColor.g = layerTextures[i].pixels[texIdx + 1] / 255.0f;
          layerColor.b = layerTextures[i].pixels[texIdx + 2] / 255.0f;
        } else if (i < m_Layers.size()) {
          // Use layer fallback color
          layerColor = m_Layers[i].color;
        } else {
          // Use default color
          layerColor = defaultColors[i];
        }

        color += layerColor * weights[i];
      }

      data[idx * 3 + 0] =
          static_cast<u8>(std::clamp(color.r * 255.0f, 0.0f, 255.0f));
      data[idx * 3 + 1] =
          static_cast<u8>(std::clamp(color.g * 255.0f, 0.0f, 255.0f));
      data[idx * 3 + 2] =
          static_cast<u8>(std::clamp(color.b * 255.0f, 0.0f, 255.0f));
    }
  }

  stbi_write_jpg(filepath.c_str(), m_Width, m_Height, 3, data.data(), 95);
  GINI_INFO("Exported: {}", filepath);
}

void Terrain::ExportHeightmapTexture(const std::string &filepath) {
  std::vector<u8> data(m_Width * m_Height * 3);

  for (u32 i = 0; i < m_Width * m_Height; i++) {
    u8 h = static_cast<u8>(std::clamp(m_Heightmap[i] * 255.0f, 0.0f, 255.0f));
    data[i * 3 + 0] = h;
    data[i * 3 + 1] = h;
    data[i * 3 + 2] = h;
  }

  stbi_write_jpg(filepath.c_str(), m_Width, m_Height, 3, data.data(), 95);
  GINI_INFO("Exported: {}", filepath);
}

void Terrain::ExportNormalTexture(const std::string &filepath) {
  std::vector<u8> data(m_Width * m_Height * 3);

  for (u32 z = 0; z < m_Height; z++) {
    for (u32 x = 0; x < m_Width; x++) {
      u32 idx = z * m_Width + x;

      // Calculate normal from heightmap
      f32 hL = (x > 0) ? m_Heightmap[z * m_Width + (x - 1)] : m_Heightmap[idx];
      f32 hR = (x < m_Width - 1) ? m_Heightmap[z * m_Width + (x + 1)]
                                 : m_Heightmap[idx];
      f32 hD = (z > 0) ? m_Heightmap[(z - 1) * m_Width + x] : m_Heightmap[idx];
      f32 hU = (z < m_Height - 1) ? m_Heightmap[(z + 1) * m_Width + x]
                                  : m_Heightmap[idx];

      Vec3 normal =
          glm::normalize(Vec3(hL - hR, 2.0f / m_HeightScale, hD - hU));

      // Convert from [-1,1] to [0,255]
      data[idx * 3 + 0] = static_cast<u8>((normal.x * 0.5f + 0.5f) * 255.0f);
      data[idx * 3 + 1] = static_cast<u8>((normal.y * 0.5f + 0.5f) * 255.0f);
      data[idx * 3 + 2] = static_cast<u8>((normal.z * 0.5f + 0.5f) * 255.0f);
    }
  }

  stbi_write_jpg(filepath.c_str(), m_Width, m_Height, 3, data.data(), 95);
  GINI_INFO("Exported: {}", filepath);
}

void Terrain::ExportRoughnessTexture(const std::string &filepath) {
  std::vector<u8> data(m_Width * m_Height * 3);

  for (u32 z = 0; z < m_Height; z++) {
    for (u32 x = 0; x < m_Width; x++) {
      u32 idx = z * m_Width + x;

      f32 weights[4] = {m_Splatmap[idx * 4 + 0], m_Splatmap[idx * 4 + 1],
                        m_Splatmap[idx * 4 + 2], m_Splatmap[idx * 4 + 3]};

      // Ensure weights sum to at least something
      f32 weightSum = weights[0] + weights[1] + weights[2] + weights[3];
      if (weightSum < 0.001f) {
        weights[0] = 1.0f;
      }

      // Blend roughness from layers (default roughness values per layer)
      f32 defaultRoughness[4] = {0.8f, 0.9f, 0.7f, 0.6f};
      f32 roughness = 0.0f;

      if (m_Layers.empty()) {
        for (u32 i = 0; i < 4; i++) {
          roughness += defaultRoughness[i] * weights[i];
        }
      } else {
        for (u32 i = 0; i < 4 && i < m_Layers.size(); i++) {
          roughness += m_Layers[i].roughness * weights[i];
        }
      }

      u8 r = static_cast<u8>(std::clamp(roughness * 255.0f, 0.0f, 255.0f));
      data[idx * 3 + 0] = r;
      data[idx * 3 + 1] = r;
      data[idx * 3 + 2] = r;
    }
  }

  stbi_write_jpg(filepath.c_str(), m_Width, m_Height, 3, data.data(), 95);
  GINI_INFO("Exported: {}", filepath);
}

void Terrain::ExportAOTexture(const std::string &filepath) {
  std::vector<u8> data(m_Width * m_Height * 3);

  // Simple AO based on cavity detection - darker in valleys/crevices
  for (u32 z = 0; z < m_Height; z++) {
    for (u32 x = 0; x < m_Width; x++) {
      u32 idx = z * m_Width + x;

      f32 centerH = m_Heightmap[idx];

      // Calculate average height of neighbors
      int sampleRadius = 3;
      f32 avgNeighborH = 0.0f;
      int samples = 0;

      for (int dz = -sampleRadius; dz <= sampleRadius; dz++) {
        for (int dx = -sampleRadius; dx <= sampleRadius; dx++) {
          if (dx == 0 && dz == 0)
            continue;

          int nx = static_cast<int>(x) + dx;
          int nz = static_cast<int>(z) + dz;

          if (nx >= 0 && nx < (int)m_Width && nz >= 0 && nz < (int)m_Height) {
            avgNeighborH += m_Heightmap[nz * m_Width + nx];
            samples++;
          }
        }
      }

      f32 ao = 1.0f;
      if (samples > 0) {
        avgNeighborH /= samples;
        // If center is lower than average neighbors, it's in a cavity (darker
        // AO) If center is higher, it's exposed (brighter AO)
        f32 heightDiff = centerH - avgNeighborH;
        ao = 0.5f + heightDiff * 5.0f; // Scale the difference
        ao = std::clamp(ao, 0.3f, 1.0f);
      }

      u8 aoVal = static_cast<u8>(ao * 255.0f);
      data[idx * 3 + 0] = aoVal;
      data[idx * 3 + 1] = aoVal;
      data[idx * 3 + 2] = aoVal;
    }
  }

  stbi_write_jpg(filepath.c_str(), m_Width, m_Height, 3, data.data(), 95);
  GINI_INFO("Exported: {}", filepath);
}

void Terrain::ExportMetallicTexture(const std::string &filepath) {
  std::vector<u8> data(m_Width * m_Height * 3);

  for (u32 z = 0; z < m_Height; z++) {
    for (u32 x = 0; x < m_Width; x++) {
      u32 idx = z * m_Width + x;

      f32 weights[4] = {m_Splatmap[idx * 4 + 0], m_Splatmap[idx * 4 + 1],
                        m_Splatmap[idx * 4 + 2], m_Splatmap[idx * 4 + 3]};

      // Ensure weights sum to at least something
      f32 weightSum = weights[0] + weights[1] + weights[2] + weights[3];
      if (weightSum < 0.001f) {
        weights[0] = 1.0f;
      }

      // Blend metallic from layers (terrain is mostly non-metallic)
      f32 defaultMetallic[4] = {0.0f, 0.0f, 0.1f, 0.0f};
      f32 metallic = 0.0f;

      if (m_Layers.empty()) {
        for (u32 i = 0; i < 4; i++) {
          metallic += defaultMetallic[i] * weights[i];
        }
      } else {
        for (u32 i = 0; i < 4 && i < m_Layers.size(); i++) {
          metallic += m_Layers[i].metallic * weights[i];
        }
      }

      u8 m = static_cast<u8>(std::clamp(metallic * 255.0f, 0.0f, 255.0f));
      data[idx * 3 + 0] = m;
      data[idx * 3 + 1] = m;
      data[idx * 3 + 2] = m;
    }
  }

  stbi_write_jpg(filepath.c_str(), m_Width, m_Height, 3, data.data(), 95);
  GINI_INFO("Exported: {}", filepath);
}

void Terrain::ExportOBJMesh(const std::string &filepath) {
  f32 halfWidth = (m_Width - 1) * m_Scale * 0.5f;
  f32 halfHeight = (m_Height - 1) * m_Scale * 0.5f;

  std::filesystem::path basePath(filepath);
  std::string baseName = basePath.stem().string();

  std::ofstream objFile(filepath);
  if (!objFile.is_open()) {
    GINI_ERROR("Failed to create OBJ file: {}", filepath);
    return;
  }

  objFile << "# Gini Engine Terrain Export\n";
  objFile << "# Vertices: " << (m_Width * m_Height) << "\n";
  objFile << "# Faces: " << ((m_Width - 1) * (m_Height - 1) * 2) << "\n\n";
  objFile << "mtllib " << baseName << ".mtl\n";
  objFile << "usemtl terrain_material\n\n";

  // Write vertices
  for (u32 z = 0; z < m_Height; z++) {
    for (u32 x = 0; x < m_Width; x++) {
      f32 height = m_Heightmap[z * m_Width + x] * m_HeightScale;
      f32 vx = static_cast<f32>(x) * m_Scale - halfWidth;
      f32 vz = static_cast<f32>(z) * m_Scale - halfHeight;
      objFile << "v " << vx << " " << height << " " << vz << "\n";
    }
  }
  objFile << "\n";

  // Write texture coordinates
  for (u32 z = 0; z < m_Height; z++) {
    for (u32 x = 0; x < m_Width; x++) {
      f32 u = static_cast<f32>(x) / static_cast<f32>(m_Width - 1);
      f32 v = static_cast<f32>(z) / static_cast<f32>(m_Height - 1);
      objFile << "vt " << u << " " << v << "\n";
    }
  }
  objFile << "\n";

  // Write normals
  for (u32 z = 0; z < m_Height; z++) {
    for (u32 x = 0; x < m_Width; x++) {
      f32 hL = (x > 0) ? m_Heightmap[z * m_Width + (x - 1)] * m_HeightScale
                       : m_Heightmap[z * m_Width + x] * m_HeightScale;
      f32 hR = (x < m_Width - 1)
                   ? m_Heightmap[z * m_Width + (x + 1)] * m_HeightScale
                   : m_Heightmap[z * m_Width + x] * m_HeightScale;
      f32 hD = (z > 0) ? m_Heightmap[(z - 1) * m_Width + x] * m_HeightScale
                       : m_Heightmap[z * m_Width + x] * m_HeightScale;
      f32 hU = (z < m_Height - 1)
                   ? m_Heightmap[(z + 1) * m_Width + x] * m_HeightScale
                   : m_Heightmap[z * m_Width + x] * m_HeightScale;

      Vec3 normal = glm::normalize(Vec3(hL - hR, 2.0f * m_Scale, hD - hU));
      objFile << "vn " << normal.x << " " << normal.y << " " << normal.z
              << "\n";
    }
  }
  objFile << "\n";

  // Write faces
  for (u32 z = 0; z < m_Height - 1; z++) {
    for (u32 x = 0; x < m_Width - 1; x++) {
      u32 topLeft = z * m_Width + x + 1;
      u32 topRight = topLeft + 1;
      u32 bottomLeft = (z + 1) * m_Width + x + 1;
      u32 bottomRight = bottomLeft + 1;

      objFile << "f " << topLeft << "/" << topLeft << "/" << topLeft << " "
              << bottomLeft << "/" << bottomLeft << "/" << bottomLeft << " "
              << topRight << "/" << topRight << "/" << topRight << "\n";
      objFile << "f " << topRight << "/" << topRight << "/" << topRight << " "
              << bottomLeft << "/" << bottomLeft << "/" << bottomLeft << " "
              << bottomRight << "/" << bottomRight << "/" << bottomRight
              << "\n";
    }
  }

  objFile.close();
  GINI_INFO("Exported: {}", filepath);
}

void Terrain::ExportMTLFile(const std::string &filepath,
                            const std::string &terrainName) {
  std::ofstream mtlFile(filepath);
  if (!mtlFile.is_open()) {
    GINI_ERROR("Failed to create MTL file: {}", filepath);
    return;
  }

  mtlFile << "# Gini Engine Terrain Material\n";
  mtlFile << "# PBR Material for terrain: " << terrainName << "\n\n";
  mtlFile << "newmtl terrain_material\n";
  mtlFile << "Ka 0.1 0.1 0.1\n";
  mtlFile << "Kd 1.0 1.0 1.0\n";
  mtlFile << "Ks 0.1 0.1 0.1\n";
  mtlFile << "Ns 10.0\n";
  mtlFile << "d 1.0\n";
  mtlFile << "illum 2\n";
  mtlFile << "map_Kd " << terrainName << "-basecolor.jpg\n";
  mtlFile << "map_Bump " << terrainName << "-normal.jpg\n";
  mtlFile << "map_Ns " << terrainName << "-roughness.jpg\n";
  mtlFile << "map_Ka " << terrainName << "-ambientocclusion.jpg\n";

  mtlFile.close();
  GINI_INFO("Exported: {}", filepath);
}

void Terrain::ExportGMATFile(const std::string &filepath,
                             const std::string &terrainName) {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;

  out << YAML::Key << "Name" << YAML::Value << (terrainName + "_material");
  out << YAML::Key << "Albedo" << YAML::Value << YAML::Flow << YAML::BeginSeq
      << 1.0f << 1.0f << 1.0f << 1.0f << YAML::EndSeq;
  out << YAML::Key << "Roughness" << YAML::Value << 0.8f;
  out << YAML::Key << "Metallic" << YAML::Value << 0.0f;

  // Texture references
  out << YAML::Key << "AlbedoTexture" << YAML::Value
      << (terrainName + "-basecolor.jpg");
  out << YAML::Key << "NormalTexture" << YAML::Value
      << (terrainName + "-normal.jpg");
  out << YAML::Key << "RoughnessTexture" << YAML::Value
      << (terrainName + "-roughness.jpg");
  out << YAML::Key << "MetallicTexture" << YAML::Value
      << (terrainName + "-metallic.jpg");
  out << YAML::Key << "AOTexture" << YAML::Value
      << (terrainName + "-ambientocclusion.jpg");

  out << YAML::EndMap; // Material
  out << YAML::EndMap; // Root

  std::ofstream fout(filepath);
  fout << out.c_str();
  fout.close();

  GINI_INFO("Exported: {}", filepath);
}

void Terrain::ExportGTerrainFile(const std::string &filepath,
                                 const std::string &terrainName) {
  GINI_INFO("Exporting .gterrain with HeightScale={}", m_HeightScale);

  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "Terrain" << YAML::Value << YAML::BeginMap;

  // Basic properties
  out << YAML::Key << "Name" << YAML::Value << terrainName;
  out << YAML::Key << "Width" << YAML::Value << m_Width;
  out << YAML::Key << "Height" << YAML::Value << m_Height;
  out << YAML::Key << "Scale" << YAML::Value << m_Scale;
  out << YAML::Key << "HeightScale" << YAML::Value << m_HeightScale;
  out << YAML::Key << "Position" << YAML::Value << YAML::Flow << YAML::BeginSeq
      << m_Position.x << m_Position.y << m_Position.z << YAML::EndSeq;

  // File references (relative paths)
  out << YAML::Key << "MeshFile" << YAML::Value << (terrainName + ".obj");
  out << YAML::Key << "MaterialFile" << YAML::Value << (terrainName + ".mtl");
  out << YAML::Key << "GiniMaterialFile" << YAML::Value
      << (terrainName + ".gmat");

  // Texture references
  out << YAML::Key << "Textures" << YAML::Value << YAML::BeginMap;
  out << YAML::Key << "BaseColor" << YAML::Value
      << (terrainName + "-basecolor.jpg");
  out << YAML::Key << "Heightmap" << YAML::Value
      << (terrainName + "-heightmap.jpg");
  out << YAML::Key << "Normal" << YAML::Value << (terrainName + "-normal.jpg");
  out << YAML::Key << "Roughness" << YAML::Value
      << (terrainName + "-roughness.jpg");
  out << YAML::Key << "AmbientOcclusion" << YAML::Value
      << (terrainName + "-ambientocclusion.jpg");
  out << YAML::Key << "Metallic" << YAML::Value
      << (terrainName + "-metallic.jpg");
  out << YAML::EndMap; // Textures

  // Layer info
  out << YAML::Key << "Layers" << YAML::Value << YAML::BeginSeq;
  for (const auto &layer : m_Layers) {
    out << YAML::BeginMap;
    out << YAML::Key << "Name" << YAML::Value << layer.name;
    out << YAML::Key << "Color" << YAML::Value << YAML::Flow << YAML::BeginSeq
        << layer.color.x << layer.color.y << layer.color.z << YAML::EndSeq;
    out << YAML::Key << "Roughness" << YAML::Value << layer.roughness;
    out << YAML::Key << "Metallic" << YAML::Value << layer.metallic;
    out << YAML::Key << "Tiling" << YAML::Value << YAML::Flow << YAML::BeginSeq
        << layer.tiling.x << layer.tiling.y << YAML::EndSeq;
    // Save texture paths if available
    if (layer.albedoMap) {
      out << YAML::Key << "AlbedoTexture" << YAML::Value
          << layer.albedoMap->GetPath();
    }
    if (layer.normalMap) {
      out << YAML::Key << "NormalTexture" << YAML::Value
          << layer.normalMap->GetPath();
    }
    out << YAML::EndMap;
  }
  out << YAML::EndSeq;

  out << YAML::EndMap; // Terrain
  out << YAML::EndMap; // Root

  std::ofstream fout(filepath);
  fout << out.c_str();
  fout.close();

  GINI_INFO("Exported: {}", filepath);
}

bool Terrain::Raycast(const Vec3 &rayOrigin, const Vec3 &rayDir,
                      Vec3 &hitPoint) const {
  // Step-based raymarching for terrain intersection
  const f32 maxDistance = 500.0f;
  const f32 stepSize = 0.5f;

  Vec3 currentPos = rayOrigin;
  Vec3 dir = glm::normalize(rayDir);

  // Calculate terrain bounds (centered at origin)
  f32 halfWidth = (m_Width - 1) * m_Scale * 0.5f;
  f32 halfHeight = (m_Height - 1) * m_Scale * 0.5f;

  for (f32 t = 0.0f; t < maxDistance; t += stepSize) {
    currentPos = rayOrigin + dir * t;

    // Check if within terrain bounds (terrain is centered at origin)
    f32 terrainX = currentPos.x - m_Position.x;
    f32 terrainZ = currentPos.z - m_Position.z;

    if (terrainX < -halfWidth || terrainX >= halfWidth ||
        terrainZ < -halfHeight || terrainZ >= halfHeight) {
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
