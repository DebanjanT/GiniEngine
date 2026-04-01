#include "UnitSelection.h"
#include "ECS/Components.h"

namespace Gini {

UnitSelection::UnitSelection(World* world) : m_World(world) {}

void UnitSelection::SelectAt(const Vec2& worldPos, bool addToSelection) {
    if (!addToSelection) {
        DeselectAll();
    }
    
    auto view = m_World->View<TransformComponent, SelectableComponent>();
    
    f32 closestDist = std::numeric_limits<f32>::max();
    Entity closestEntity = NullEntity;
    
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& selectable = view.get<SelectableComponent>(entity);
        
        Vec2 entityPos(transform.position.x, transform.position.y);
        f32 dist = glm::distance(worldPos, entityPos);
        
        if (dist <= selectable.selectionRadius && dist < closestDist) {
            if (CanSelect(entity)) {
                closestDist = dist;
                closestEntity = entity;
            }
        }
    }
    
    if (closestEntity != NullEntity) {
        AddToSelection(closestEntity);
    }
}

void UnitSelection::SelectInRect(const Rect& worldRect, bool addToSelection) {
    if (!addToSelection) {
        DeselectAll();
    }
    
    auto view = m_World->View<TransformComponent, SelectableComponent>();
    
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        Vec2 entityPos(transform.position.x, transform.position.y);
        
        if (worldRect.Contains(entityPos) && CanSelect(entity)) {
            AddToSelection(entity);
        }
    }
}

void UnitSelection::SelectAll(i32 playerId) {
    DeselectAll();
    
    auto view = m_World->View<TransformComponent, SelectableComponent, UnitComponent>();
    
    for (auto entity : view) {
        auto& unit = view.get<UnitComponent>(entity);
        if (unit.playerId == playerId && CanSelect(entity)) {
            AddToSelection(entity);
        }
    }
}

void UnitSelection::Deselect(Entity entity) {
    RemoveFromSelection(entity);
}

void UnitSelection::DeselectAll() {
    for (auto entity : m_SelectedEntities) {
        if (m_World->HasComponent<SelectableComponent>(entity)) {
            m_World->GetComponent<SelectableComponent>(entity).selected = false;
        }
    }
    m_SelectedEntities.clear();
}

void UnitSelection::BeginSelectionBox(const Vec2& startPos) {
    m_IsSelecting = true;
    m_SelectionStart = startPos;
    m_SelectionBox = Rect(startPos.x, startPos.y, 0, 0);
}

void UnitSelection::UpdateSelectionBox(const Vec2& currentPos) {
    if (!m_IsSelecting) return;
    
    f32 x = std::min(m_SelectionStart.x, currentPos.x);
    f32 y = std::min(m_SelectionStart.y, currentPos.y);
    f32 w = std::abs(currentPos.x - m_SelectionStart.x);
    f32 h = std::abs(currentPos.y - m_SelectionStart.y);
    
    m_SelectionBox = Rect(x, y, w, h);
}

void UnitSelection::EndSelectionBox(bool addToSelection) {
    if (!m_IsSelecting) return;
    
    m_IsSelecting = false;
    
    if (m_SelectionBox.width > 5.0f || m_SelectionBox.height > 5.0f) {
        SelectInRect(m_SelectionBox, addToSelection);
    }
}

bool UnitSelection::IsSelected(Entity entity) const {
    return std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity) 
           != m_SelectedEntities.end();
}

bool UnitSelection::CanSelect(Entity entity) const {
    if (m_PlayerFilter >= 0) {
        if (m_World->HasComponent<UnitComponent>(entity)) {
            if (m_World->GetComponent<UnitComponent>(entity).playerId != m_PlayerFilter) {
                return false;
            }
        }
    }
    return true;
}

void UnitSelection::AddToSelection(Entity entity) {
    if (!IsSelected(entity)) {
        m_SelectedEntities.push_back(entity);
        if (m_World->HasComponent<SelectableComponent>(entity)) {
            m_World->GetComponent<SelectableComponent>(entity).selected = true;
        }
    }
}

void UnitSelection::RemoveFromSelection(Entity entity) {
    auto it = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity);
    if (it != m_SelectedEntities.end()) {
        if (m_World->HasComponent<SelectableComponent>(entity)) {
            m_World->GetComponent<SelectableComponent>(entity).selected = false;
        }
        m_SelectedEntities.erase(it);
    }
}

} // namespace Gini
