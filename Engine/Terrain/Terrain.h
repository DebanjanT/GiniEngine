#pragma once

#include "Core/Types.h"
#include "Renderer/Camera3D.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture.h"
#include <string>
#include <vector>

namespace Gini {

// Terrain material layer for splatmap painting
struct TerrainLayer {
  std::string name = "Layer";
  Ref<Texture2D> albedoMap;
  Ref<Texture2D> normalMap;
  Vec3 color = Vec3(0.5f); // Fallback color when no texture
  Vec2 tiling = Vec2(10.0f);
  f32 metallic = 0.0f;
  f32 roughness = 0.8f;
};

// Terrain chunk for LOD and culling
struct TerrainChunk {
  u32 vao = 0;
  u32 vbo = 0;
  u32 ebo = 0;
  u32 indexCount = 0;
  Vec3 center;
  f32 radius;
  i32 lodLevel = 0;
};

class Terrain {
public:
  Terrain(u32 width = 256, u32 height = 256, f32 scale = 1.0f);
  ~Terrain();

  // Heightmap operations
  void LoadHeightmap(const std::string &filepath);
  void GenerateFlat();
  void GenerateFromNoise(f32 frequency = 0.02f, f32 amplitude = 50.0f,
                         i32 octaves = 4);
  void SetHeight(u32 x, u32 z, f32 height);
  f32 GetHeight(u32 x, u32 z) const;
  f32 GetHeightAtPosition(f32 worldX, f32 worldZ) const;
  Vec3 GetNormalAtPosition(f32 worldX, f32 worldZ) const;

  // Material layers (up to 4 for splatmap)
  void AddLayer(const TerrainLayer &layer);
  void RemoveLayer(u32 index);
  TerrainLayer &GetLayer(u32 index) { return m_Layers[index]; }
  u32 GetLayerCount() const { return static_cast<u32>(m_Layers.size()); }

  // Splatmap for material blending
  void SetSplatmapPixel(u32 x, u32 z, u32 layerIndex, f32 weight);
  f32 GetSplatmapPixel(u32 x, u32 z, u32 layerIndex) const;
  void UpdateSplatmapTexture();

  // Painting
  void PaintHeight(f32 worldX, f32 worldZ, f32 radius, f32 strength,
                   bool raise = true);
  void SmoothHeight(f32 worldX, f32 worldZ, f32 radius, f32 strength);
  void FlattenHeight(f32 worldX, f32 worldZ, f32 radius, f32 targetHeight);
  void PaintMaterial(f32 worldX, f32 worldZ, f32 radius, f32 strength,
                     u32 layerIndex);

  // Rendering
  void Render(const Camera3D &camera);
  void Render(const Camera3D &camera, const Vec3 &sunDirection,
              const Vec3 &sunColor, f32 ambientIntensity);
  void RenderWireframe(const Camera3D &camera);

  // Light settings (used when no external light is provided)
  void SetSunDirection(const Vec3 &dir) {
    m_SunDirection = glm::normalize(dir);
  }
  void SetSunColor(const Vec3 &color) { m_SunColor = color; }
  void SetAmbientIntensity(f32 intensity) { m_AmbientIntensity = intensity; }
  Vec3 GetSunDirection() const { return m_SunDirection; }
  Vec3 GetSunColor() const { return m_SunColor; }
  f32 GetAmbientIntensity() const { return m_AmbientIntensity; }

  // Mesh generation
  void RegenerateMesh();
  void GenerateLODMeshes();

  // Accessors
  u32 GetWidth() const { return m_Width; }
  u32 GetHeight() const { return m_Height; }
  f32 GetScale() const { return m_Scale; }
  f32 GetHeightScale() const { return m_HeightScale; }
  void SetHeightScale(f32 scale) {
    m_HeightScale = scale;
    RegenerateMesh();
  }

  Vec3 GetWorldPosition() const { return m_Position; }
  void SetWorldPosition(const Vec3 &pos) { m_Position = pos; }

  // Raycasting for painting
  bool Raycast(const Vec3 &rayOrigin, const Vec3 &rayDir, Vec3 &hitPoint) const;

  // Get terrain bounds
  Vec3 GetMinBounds() const { return m_Position; }
  Vec3 GetMaxBounds() const {
    return m_Position +
           Vec3(m_Width * m_Scale, m_HeightScale * 100.0f, m_Height * m_Scale);
  }

  // Serialization
  void SaveHeightmap(const std::string &filepath);
  void SaveSplatmap(const std::string &filepath);
  void SaveTerrain(const std::string &filepath);
  void LoadTerrain(const std::string &filepath);

  // Unified terrain export - exports all files to a folder
  // Creates: basecolor.jpg, heightmap.jpg, normal.jpg, roughness.jpg,
  //          ambientocclusion.jpg, metallic.jpg, terrain.obj, terrain.mtl,
  //          terrain.gmat, terrain.gterrain
  void ExportTerrain(const std::string &exportFolder,
                     const std::string &terrainName);

  static Ref<Terrain> Create(u32 width = 256, u32 height = 256,
                             f32 scale = 1.0f);

private:
  // Export helper functions
  void ExportBaseColorTexture(const std::string &filepath);
  void ExportHeightmapTexture(const std::string &filepath);
  void ExportNormalTexture(const std::string &filepath);
  void ExportRoughnessTexture(const std::string &filepath);
  void ExportAOTexture(const std::string &filepath);
  void ExportMetallicTexture(const std::string &filepath);
  void ExportSplatmapTexture(const std::string &filepath);
  void ExportOBJMesh(const std::string &filepath);
  void ExportMTLFile(const std::string &filepath,
                     const std::string &terrainName);
  void ExportGMATFile(const std::string &filepath,
                      const std::string &terrainName);
  void ExportGTerrainFile(const std::string &filepath,
                          const std::string &terrainName);

  void InitShader();
  void CreateMesh();
  void CreateChunks();
  u32 GetLODForDistance(f32 distance) const;

  u32 m_Width;
  u32 m_Height;
  f32 m_Scale;
  f32 m_HeightScale = 1.0f;
  Vec3 m_Position = Vec3(0.0f);

  // Heightmap data
  std::vector<f32> m_Heightmap;

  // Splatmap data (RGBA = 4 layers)
  std::vector<f32> m_Splatmap;
  Ref<Texture2D> m_SplatmapTexture;

  // Material layers
  std::vector<TerrainLayer> m_Layers;

  // Mesh data
  u32 m_VAO = 0;
  u32 m_VBO = 0;
  u32 m_EBO = 0;
  u32 m_IndexCount = 0;

  // LOD chunks
  std::vector<TerrainChunk> m_Chunks;
  u32 m_ChunkSize = 32;

  // Shader
  Ref<Shader> m_Shader;

  // LOD settings
  std::vector<f32> m_LODDistances = {50.0f, 100.0f, 200.0f, 400.0f};

  // Light settings
  Vec3 m_SunDirection = glm::normalize(Vec3(-0.5f, -0.8f, -0.3f));
  Vec3 m_SunColor = Vec3(3.0f, 2.9f, 2.7f);
  f32 m_AmbientIntensity = 0.15f;

  bool m_NeedsUpdate = true;
};

} // namespace Gini
