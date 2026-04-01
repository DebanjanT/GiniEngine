#pragma once

#include "Core/Types.h"
#include "TileMap.h"
#include <vector>
#include <queue>
#include <unordered_map>

namespace Gini {

struct PathNode {
    IVec2 position;
    f32 gCost = 0.0f;  // Cost from start
    f32 hCost = 0.0f;  // Heuristic cost to end
    f32 fCost() const { return gCost + hCost; }
    PathNode* parent = nullptr;
    
    bool operator>(const PathNode& other) const {
        return fCost() > other.fCost();
    }
};

class Pathfinder {
public:
    Pathfinder(TileMap* tileMap);
    
    // A* pathfinding
    std::vector<Vec2> FindPath(const Vec2& start, const Vec2& end);
    std::vector<IVec2> FindPathTiles(const IVec2& start, const IVec2& end);
    
    // Settings
    void SetAllowDiagonals(bool allow) { m_AllowDiagonals = allow; }
    void SetMaxIterations(u32 max) { m_MaxIterations = max; }
    
    // Path smoothing
    std::vector<Vec2> SmoothPath(const std::vector<Vec2>& path);
    
    // Line of sight check
    bool HasLineOfSight(const IVec2& start, const IVec2& end) const;
    
private:
    f32 Heuristic(const IVec2& a, const IVec2& b) const;
    f32 GetMovementCost(const IVec2& from, const IVec2& to) const;
    std::vector<IVec2> ReconstructPath(PathNode* endNode);
    
    TileMap* m_TileMap;
    bool m_AllowDiagonals = true;
    u32 m_MaxIterations = 10000;
};

// Flow field pathfinding for large unit counts
class FlowField {
public:
    FlowField(TileMap* tileMap);
    
    void Generate(const IVec2& target);
    Vec2 GetDirection(const IVec2& position) const;
    bool IsValid() const { return m_Valid; }
    
private:
    TileMap* m_TileMap;
    std::vector<Vec2> m_Directions;
    std::vector<f32> m_Costs;
    bool m_Valid = false;
};

} // namespace Gini
