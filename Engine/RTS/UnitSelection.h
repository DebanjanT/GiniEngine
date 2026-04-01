#pragma once

#include "Core/Types.h"
#include "ECS/World.h"
#include <vector>

namespace Gini {

class UnitSelection {
public:
    UnitSelection(World* world);
    
    // Selection
    void SelectAt(const Vec2& worldPos, bool addToSelection = false);
    void SelectInRect(const Rect& worldRect, bool addToSelection = false);
    void SelectAll(i32 playerId);
    void Deselect(Entity entity);
    void DeselectAll();
    
    // Selection box (drag select)
    void BeginSelectionBox(const Vec2& startPos);
    void UpdateSelectionBox(const Vec2& currentPos);
    void EndSelectionBox(bool addToSelection = false);
    bool IsSelecting() const { return m_IsSelecting; }
    Rect GetSelectionBox() const { return m_SelectionBox; }
    
    // Getters
    const std::vector<Entity>& GetSelectedEntities() const { return m_SelectedEntities; }
    bool IsSelected(Entity entity) const;
    bool HasSelection() const { return !m_SelectedEntities.empty(); }
    u32 GetSelectionCount() const { return static_cast<u32>(m_SelectedEntities.size()); }
    
    // Filtering
    void SetPlayerFilter(i32 playerId) { m_PlayerFilter = playerId; }
    void SetSelectableTypes(const std::vector<std::string>& types) { m_SelectableTypes = types; }
    
private:
    bool CanSelect(Entity entity) const;
    void AddToSelection(Entity entity);
    void RemoveFromSelection(Entity entity);
    
    World* m_World;
    std::vector<Entity> m_SelectedEntities;
    
    bool m_IsSelecting = false;
    Vec2 m_SelectionStart;
    Rect m_SelectionBox;
    
    i32 m_PlayerFilter = -1; // -1 = no filter
    std::vector<std::string> m_SelectableTypes;
};

} // namespace Gini
