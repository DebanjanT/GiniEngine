#include "TileMap.h"
#include "Core/Logger.h"
#include <fstream>

namespace Gini {

TileMap::TileMap(u32 width, u32 height, u32 tileSize)
    : m_Width(width), m_Height(height), m_TileSize(tileSize) {
    m_Tiles.resize(width * height);
}

Tile& TileMap::GetTile(u32 x, u32 y) {
    GINI_DEBUG_ASSERT(IsInBounds(x, y), "Tile coordinates out of bounds");
    return m_Tiles[GetIndex(x, y)];
}

const Tile& TileMap::GetTile(u32 x, u32 y) const {
    GINI_DEBUG_ASSERT(IsInBounds(x, y), "Tile coordinates out of bounds");
    return m_Tiles[GetIndex(x, y)];
}

Tile& TileMap::GetTileAt(const Vec2& worldPos) {
    IVec2 tilePos = WorldToTile(worldPos);
    return GetTile(tilePos.x, tilePos.y);
}

const Tile& TileMap::GetTileAt(const Vec2& worldPos) const {
    IVec2 tilePos = WorldToTile(worldPos);
    return GetTile(tilePos.x, tilePos.y);
}

IVec2 TileMap::WorldToTile(const Vec2& worldPos) const {
    return IVec2(
        static_cast<i32>(worldPos.x / m_TileSize),
        static_cast<i32>(worldPos.y / m_TileSize)
    );
}

Vec2 TileMap::TileToWorld(const IVec2& tilePos) const {
    return Vec2(
        static_cast<f32>(tilePos.x * m_TileSize),
        static_cast<f32>(tilePos.y * m_TileSize)
    );
}

Vec2 TileMap::TileToWorldCenter(const IVec2& tilePos) const {
    return Vec2(
        static_cast<f32>(tilePos.x * m_TileSize) + m_TileSize * 0.5f,
        static_cast<f32>(tilePos.y * m_TileSize) + m_TileSize * 0.5f
    );
}

void TileMap::SetTile(u32 x, u32 y, const Tile& tile) {
    if (IsInBounds(x, y)) {
        m_Tiles[GetIndex(x, y)] = tile;
    }
}

void TileMap::SetTileType(u32 x, u32 y, TileType type) {
    if (IsInBounds(x, y)) {
        m_Tiles[GetIndex(x, y)].type = type;
        
        // Set default properties based on type
        auto& tile = m_Tiles[GetIndex(x, y)];
        switch (type) {
            case TileType::Water:
                tile.walkable = false;
                tile.buildable = false;
                tile.movementCost = 999.0f;
                break;
            case TileType::Mountain:
                tile.walkable = false;
                tile.buildable = false;
                tile.movementCost = 999.0f;
                break;
            case TileType::Forest:
                tile.walkable = true;
                tile.buildable = false;
                tile.movementCost = 2.0f;
                break;
            case TileType::Road:
                tile.walkable = true;
                tile.buildable = false;
                tile.movementCost = 0.5f;
                break;
            default:
                tile.walkable = true;
                tile.buildable = true;
                tile.movementCost = 1.0f;
                break;
        }
    }
}

bool TileMap::IsWalkable(u32 x, u32 y) const {
    if (!IsInBounds(x, y)) return false;
    return m_Tiles[GetIndex(x, y)].walkable;
}

bool TileMap::IsBuildable(u32 x, u32 y) const {
    if (!IsInBounds(x, y)) return false;
    const auto& tile = m_Tiles[GetIndex(x, y)];
    return tile.buildable && tile.occupiedBy < 0;
}

bool TileMap::IsInBounds(i32 x, i32 y) const {
    return x >= 0 && x < static_cast<i32>(m_Width) && 
           y >= 0 && y < static_cast<i32>(m_Height);
}

std::vector<IVec2> TileMap::GetNeighbors(const IVec2& pos, bool includeDiagonals) const {
    std::vector<IVec2> neighbors;
    
    // Cardinal directions
    const IVec2 cardinals[] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    for (const auto& dir : cardinals) {
        IVec2 neighbor = pos + dir;
        if (IsInBounds(neighbor.x, neighbor.y)) {
            neighbors.push_back(neighbor);
        }
    }
    
    // Diagonal directions
    if (includeDiagonals) {
        const IVec2 diagonals[] = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}};
        for (const auto& dir : diagonals) {
            IVec2 neighbor = pos + dir;
            if (IsInBounds(neighbor.x, neighbor.y)) {
                neighbors.push_back(neighbor);
            }
        }
    }
    
    return neighbors;
}

void TileMap::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        GINI_ERROR("Failed to load tilemap: ", filepath);
        return;
    }
    
    file.read(reinterpret_cast<char*>(&m_Width), sizeof(m_Width));
    file.read(reinterpret_cast<char*>(&m_Height), sizeof(m_Height));
    file.read(reinterpret_cast<char*>(&m_TileSize), sizeof(m_TileSize));
    
    m_Tiles.resize(m_Width * m_Height);
    file.read(reinterpret_cast<char*>(m_Tiles.data()), m_Tiles.size() * sizeof(Tile));
    
    GINI_INFO("Loaded tilemap: ", filepath, " (", m_Width, "x", m_Height, ")");
}

void TileMap::SaveToFile(const std::string& filepath) const {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        GINI_ERROR("Failed to save tilemap: ", filepath);
        return;
    }
    
    file.write(reinterpret_cast<const char*>(&m_Width), sizeof(m_Width));
    file.write(reinterpret_cast<const char*>(&m_Height), sizeof(m_Height));
    file.write(reinterpret_cast<const char*>(&m_TileSize), sizeof(m_TileSize));
    file.write(reinterpret_cast<const char*>(m_Tiles.data()), m_Tiles.size() * sizeof(Tile));
    
    GINI_INFO("Saved tilemap: ", filepath);
}

} // namespace Gini
