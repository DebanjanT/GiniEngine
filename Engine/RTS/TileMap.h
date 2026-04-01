#pragma once

#include "Core/Types.h"
#include "Renderer/Texture.h"
#include <vector>

namespace Gini {

enum class TileType : u8 {
    Empty = 0,
    Ground,
    Water,
    Forest,
    Mountain,
    Road,
    Bridge,
    Building,
    Resource
};

struct Tile {
    TileType type = TileType::Ground;
    u16 textureIndex = 0;
    bool walkable = true;
    bool buildable = true;
    f32 movementCost = 1.0f;
    i32 occupiedBy = -1; // Entity ID or -1
};

class TileMap {
public:
    TileMap(u32 width, u32 height, u32 tileSize = 32);
    ~TileMap() = default;
    
    // Tile access
    Tile& GetTile(u32 x, u32 y);
    const Tile& GetTile(u32 x, u32 y) const;
    Tile& GetTileAt(const Vec2& worldPos);
    const Tile& GetTileAt(const Vec2& worldPos) const;
    
    // Coordinate conversion
    IVec2 WorldToTile(const Vec2& worldPos) const;
    Vec2 TileToWorld(const IVec2& tilePos) const;
    Vec2 TileToWorldCenter(const IVec2& tilePos) const;
    
    // Properties
    u32 GetWidth() const { return m_Width; }
    u32 GetHeight() const { return m_Height; }
    u32 GetTileSize() const { return m_TileSize; }
    Vec2 GetWorldSize() const { return Vec2(m_Width * m_TileSize, m_Height * m_TileSize); }
    
    // Tile operations
    void SetTile(u32 x, u32 y, const Tile& tile);
    void SetTileType(u32 x, u32 y, TileType type);
    bool IsWalkable(u32 x, u32 y) const;
    bool IsBuildable(u32 x, u32 y) const;
    bool IsInBounds(i32 x, i32 y) const;
    
    // Rendering
    void SetTileAtlas(Ref<TextureAtlas> atlas) { m_TileAtlas = atlas; }
    Ref<TextureAtlas> GetTileAtlas() const { return m_TileAtlas; }
    
    // Serialization
    void LoadFromFile(const std::string& filepath);
    void SaveToFile(const std::string& filepath) const;
    
    // Utility
    std::vector<IVec2> GetNeighbors(const IVec2& pos, bool includeDiagonals = true) const;
    
private:
    u32 m_Width;
    u32 m_Height;
    u32 m_TileSize;
    std::vector<Tile> m_Tiles;
    Ref<TextureAtlas> m_TileAtlas;
    
    u32 GetIndex(u32 x, u32 y) const { return y * m_Width + x; }
};

} // namespace Gini
