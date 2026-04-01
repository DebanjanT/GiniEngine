#include "Pathfinding.h"
#include <cmath>
#include <algorithm>
#include <unordered_set>

namespace Gini {

// Hash function for IVec2
struct IVec2Hash {
    size_t operator()(const IVec2& v) const {
        return std::hash<i32>()(v.x) ^ (std::hash<i32>()(v.y) << 16);
    }
};

Pathfinder::Pathfinder(TileMap* tileMap) : m_TileMap(tileMap) {}

std::vector<Vec2> Pathfinder::FindPath(const Vec2& start, const Vec2& end) {
    IVec2 startTile = m_TileMap->WorldToTile(start);
    IVec2 endTile = m_TileMap->WorldToTile(end);
    
    auto tilePath = FindPathTiles(startTile, endTile);
    
    std::vector<Vec2> worldPath;
    worldPath.reserve(tilePath.size());
    for (const auto& tile : tilePath) {
        worldPath.push_back(m_TileMap->TileToWorldCenter(tile));
    }
    
    return SmoothPath(worldPath);
}

std::vector<IVec2> Pathfinder::FindPathTiles(const IVec2& start, const IVec2& end) {
    if (!m_TileMap->IsInBounds(start.x, start.y) || !m_TileMap->IsInBounds(end.x, end.y)) {
        return {};
    }
    
    if (!m_TileMap->IsWalkable(end.x, end.y)) {
        return {};
    }
    
    std::vector<PathNode> allNodes;
    allNodes.reserve(m_MaxIterations);
    
    auto compare = [](PathNode* a, PathNode* b) { return *a > *b; };
    std::priority_queue<PathNode*, std::vector<PathNode*>, decltype(compare)> openSet(compare);
    std::unordered_set<i64> closedSet;
    std::unordered_map<i64, PathNode*> openSetMap;
    
    auto posToKey = [](const IVec2& pos) -> i64 {
        return (static_cast<i64>(pos.x) << 32) | static_cast<i64>(pos.y);
    };
    
    allNodes.push_back({start, 0.0f, Heuristic(start, end), nullptr});
    PathNode* startNode = &allNodes.back();
    openSet.push(startNode);
    openSetMap[posToKey(start)] = startNode;
    
    u32 iterations = 0;
    
    while (!openSet.empty() && iterations < m_MaxIterations) {
        iterations++;
        
        PathNode* current = openSet.top();
        openSet.pop();
        
        i64 currentKey = posToKey(current->position);
        openSetMap.erase(currentKey);
        
        if (current->position == end) {
            return ReconstructPath(current);
        }
        
        closedSet.insert(currentKey);
        
        auto neighbors = m_TileMap->GetNeighbors(current->position, m_AllowDiagonals);
        
        for (const auto& neighborPos : neighbors) {
            i64 neighborKey = posToKey(neighborPos);
            
            if (closedSet.count(neighborKey) > 0) continue;
            if (!m_TileMap->IsWalkable(neighborPos.x, neighborPos.y)) continue;
            
            f32 tentativeG = current->gCost + GetMovementCost(current->position, neighborPos);
            
            auto it = openSetMap.find(neighborKey);
            if (it != openSetMap.end()) {
                if (tentativeG < it->second->gCost) {
                    it->second->gCost = tentativeG;
                    it->second->parent = current;
                }
            } else {
                allNodes.push_back({neighborPos, tentativeG, Heuristic(neighborPos, end), current});
                PathNode* newNode = &allNodes.back();
                openSet.push(newNode);
                openSetMap[neighborKey] = newNode;
            }
        }
    }
    
    return {}; // No path found
}

f32 Pathfinder::Heuristic(const IVec2& a, const IVec2& b) const {
    // Octile distance for 8-directional movement
    f32 dx = std::abs(static_cast<f32>(a.x - b.x));
    f32 dy = std::abs(static_cast<f32>(a.y - b.y));
    return dx + dy + (1.414f - 2.0f) * std::min(dx, dy);
}

f32 Pathfinder::GetMovementCost(const IVec2& from, const IVec2& to) const {
    f32 baseCost = (from.x != to.x && from.y != to.y) ? 1.414f : 1.0f;
    return baseCost * m_TileMap->GetTile(to.x, to.y).movementCost;
}

std::vector<IVec2> Pathfinder::ReconstructPath(PathNode* endNode) {
    std::vector<IVec2> path;
    PathNode* current = endNode;
    
    while (current != nullptr) {
        path.push_back(current->position);
        current = current->parent;
    }
    
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<Vec2> Pathfinder::SmoothPath(const std::vector<Vec2>& path) {
    if (path.size() <= 2) return path;
    
    std::vector<Vec2> smoothed;
    smoothed.push_back(path[0]);
    
    size_t current = 0;
    while (current < path.size() - 1) {
        size_t farthest = current + 1;
        
        for (size_t i = current + 2; i < path.size(); i++) {
            IVec2 start = m_TileMap->WorldToTile(path[current]);
            IVec2 end = m_TileMap->WorldToTile(path[i]);
            
            if (HasLineOfSight(start, end)) {
                farthest = i;
            }
        }
        
        smoothed.push_back(path[farthest]);
        current = farthest;
    }
    
    return smoothed;
}

bool Pathfinder::HasLineOfSight(const IVec2& start, const IVec2& end) const {
    // Bresenham's line algorithm
    i32 x0 = start.x, y0 = start.y;
    i32 x1 = end.x, y1 = end.y;
    
    i32 dx = std::abs(x1 - x0);
    i32 dy = std::abs(y1 - y0);
    i32 sx = x0 < x1 ? 1 : -1;
    i32 sy = y0 < y1 ? 1 : -1;
    i32 err = dx - dy;
    
    while (true) {
        if (!m_TileMap->IsWalkable(x0, y0)) return false;
        
        if (x0 == x1 && y0 == y1) break;
        
        i32 e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
    
    return true;
}

// FlowField implementation
FlowField::FlowField(TileMap* tileMap) : m_TileMap(tileMap) {
    u32 size = tileMap->GetWidth() * tileMap->GetHeight();
    m_Directions.resize(size, Vec2(0, 0));
    m_Costs.resize(size, std::numeric_limits<f32>::max());
}

void FlowField::Generate(const IVec2& target) {
    m_Valid = false;
    
    if (!m_TileMap->IsInBounds(target.x, target.y)) return;
    
    u32 width = m_TileMap->GetWidth();
    u32 height = m_TileMap->GetHeight();
    
    std::fill(m_Costs.begin(), m_Costs.end(), std::numeric_limits<f32>::max());
    std::fill(m_Directions.begin(), m_Directions.end(), Vec2(0, 0));
    
    std::queue<IVec2> frontier;
    frontier.push(target);
    m_Costs[target.y * width + target.x] = 0;
    
    while (!frontier.empty()) {
        IVec2 current = frontier.front();
        frontier.pop();
        
        auto neighbors = m_TileMap->GetNeighbors(current, true);
        for (const auto& neighbor : neighbors) {
            if (!m_TileMap->IsWalkable(neighbor.x, neighbor.y)) continue;
            
            f32 newCost = m_Costs[current.y * width + current.x] + 
                          m_TileMap->GetTile(neighbor.x, neighbor.y).movementCost;
            
            u32 idx = neighbor.y * width + neighbor.x;
            if (newCost < m_Costs[idx]) {
                m_Costs[idx] = newCost;
                frontier.push(neighbor);
            }
        }
    }
    
    // Calculate directions
    for (u32 y = 0; y < height; y++) {
        for (u32 x = 0; x < width; x++) {
            u32 idx = y * width + x;
            if (m_Costs[idx] == std::numeric_limits<f32>::max()) continue;
            
            Vec2 bestDir(0, 0);
            f32 bestCost = m_Costs[idx];
            
            auto neighbors = m_TileMap->GetNeighbors(IVec2(x, y), true);
            for (const auto& neighbor : neighbors) {
                u32 nIdx = neighbor.y * width + neighbor.x;
                if (m_Costs[nIdx] < bestCost) {
                    bestCost = m_Costs[nIdx];
                    bestDir = Vec2(neighbor.x - x, neighbor.y - y);
                }
            }
            
            if (glm::length(bestDir) > 0) {
                m_Directions[idx] = glm::normalize(bestDir);
            }
        }
    }
    
    m_Valid = true;
}

Vec2 FlowField::GetDirection(const IVec2& position) const {
    if (!m_Valid || !m_TileMap->IsInBounds(position.x, position.y)) {
        return Vec2(0, 0);
    }
    return m_Directions[position.y * m_TileMap->GetWidth() + position.x];
}

} // namespace Gini
