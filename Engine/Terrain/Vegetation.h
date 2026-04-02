#pragma once

#include "Core/Types.h"
#include "Renderer/Mesh.h"
#include "Renderer/Texture.h"
#include "Renderer/Shader.h"
#include "Renderer/Camera3D.h"
#include "Terrain.h"
#include <vector>
#include <string>

namespace Gini {

// Vegetation instance data for GPU instancing
struct VegetationInstance {
    Vec3 position;
    f32 rotation;
    f32 scale;
    Vec3 color; // Tint variation
};

// LOD level for vegetation
struct VegetationLOD {
    Ref<Mesh> mesh;
    f32 maxDistance;
};

// Vegetation type definition (grass, tree, bush, etc.)
struct VegetationType {
    std::string name = "Vegetation";
    std::vector<VegetationLOD> lods;
    Ref<Texture2D> albedoTexture;
    Ref<Texture2D> normalTexture;
    
    // Placement settings
    f32 minScale = 0.8f;
    f32 maxScale = 1.2f;
    f32 minSlope = 0.0f;  // Radians
    f32 maxSlope = 0.5f;  // Radians
    f32 minHeight = -1000.0f;
    f32 maxHeight = 1000.0f;
    
    // Rendering settings
    bool castShadows = true;
    bool receiveShadows = true;
    bool billboard = false; // For grass/small plants
    f32 windStrength = 0.5f;
    Vec3 colorVariation = Vec3(0.1f);
    
    // Culling
    f32 cullDistance = 500.0f;
};

// Vegetation cell for spatial partitioning
struct VegetationCell {
    std::vector<VegetationInstance> instances;
    Vec3 center;
    f32 radius;
    bool visible = true;
    
    // Instance buffer for GPU
    u32 instanceVBO = 0;
    u32 instanceCount = 0;
    bool dirty = true;
};

class VegetationSystem {
public:
    VegetationSystem();
    ~VegetationSystem();
    
    // Vegetation type management
    u32 AddVegetationType(const VegetationType& type);
    void RemoveVegetationType(u32 typeIndex);
    VegetationType& GetVegetationType(u32 index) { return m_Types[index]; }
    u32 GetVegetationTypeCount() const { return static_cast<u32>(m_Types.size()); }
    
    // Instance management
    void AddInstance(u32 typeIndex, const Vec3& position, f32 rotation = 0.0f, f32 scale = 1.0f);
    void RemoveInstancesInRadius(u32 typeIndex, const Vec3& center, f32 radius);
    void ClearInstances(u32 typeIndex);
    void ClearAllInstances();
    
    // Painting
    void Paint(u32 typeIndex, const Vec3& center, f32 radius, f32 density, Terrain* terrain = nullptr);
    void Erase(u32 typeIndex, const Vec3& center, f32 radius);
    
    // Rendering
    void Render(const Camera3D& camera);
    void RenderShadows(const Camera3D& lightCamera);
    
    // Culling
    void UpdateCulling(const Camera3D& camera);
    void SetFrustumCullingEnabled(bool enabled) { m_FrustumCulling = enabled; }
    void SetDistanceCullingEnabled(bool enabled) { m_DistanceCulling = enabled; }
    
    // Settings
    void SetCellSize(f32 size) { m_CellSize = size; RebuildCells(); }
    f32 GetCellSize() const { return m_CellSize; }
    void SetGlobalDensityMultiplier(f32 mult) { m_DensityMultiplier = mult; }
    
    // Statistics
    u32 GetTotalInstanceCount() const;
    u32 GetVisibleInstanceCount() const;
    u32 GetDrawCallCount() const { return m_DrawCallCount; }
    
    static Ref<VegetationSystem> Create();
    
private:
    void InitShader();
    void RebuildCells();
    void UpdateInstanceBuffer(u32 typeIndex, VegetationCell& cell);
    void RenderCell(u32 typeIndex, VegetationCell& cell, const Camera3D& camera, u32 lodLevel);
    u32 GetLODLevel(f32 distance, u32 typeIndex) const;
    bool IsInFrustum(const Vec3& center, f32 radius, const Camera3D& camera) const;
    Vec2 GetCellCoord(const Vec3& position) const;
    
    std::vector<VegetationType> m_Types;
    
    // Spatial partitioning: typeIndex -> cells
    std::vector<std::unordered_map<u64, VegetationCell>> m_Cells;
    
    Ref<Shader> m_Shader;
    Ref<Shader> m_BillboardShader;
    
    f32 m_CellSize = 32.0f;
    f32 m_DensityMultiplier = 1.0f;
    bool m_FrustumCulling = true;
    bool m_DistanceCulling = true;
    
    // Stats
    u32 m_DrawCallCount = 0;
    u32 m_VisibleInstances = 0;
};

} // namespace Gini
