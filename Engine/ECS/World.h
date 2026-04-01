#pragma once

#include "Core/Types.h"
#include "Components.h"
#include <entt/entt.hpp>

namespace Gini {

using Entity = entt::entity;
constexpr Entity NullEntity = entt::null;

class World {
public:
    World() = default;
    ~World() = default;
    
    // Entity management
    Entity CreateEntity(const std::string& name = "Entity");
    void DestroyEntity(Entity entity);
    bool IsValid(Entity entity) const;
    
    // Component access
    template<typename T, typename... Args>
    T& AddComponent(Entity entity, Args&&... args) {
        return m_Registry.emplace<T>(entity, std::forward<Args>(args)...);
    }
    
    template<typename T>
    T& GetComponent(Entity entity) {
        return m_Registry.get<T>(entity);
    }
    
    template<typename T>
    const T& GetComponent(Entity entity) const {
        return m_Registry.get<T>(entity);
    }
    
    template<typename T>
    bool HasComponent(Entity entity) const {
        return m_Registry.all_of<T>(entity);
    }
    
    template<typename T>
    void RemoveComponent(Entity entity) {
        m_Registry.remove<T>(entity);
    }
    
    // Views for iterating entities
    template<typename... Components>
    auto View() {
        return m_Registry.view<Components...>();
    }
    
    template<typename... Components>
    auto View() const {
        return m_Registry.view<Components...>();
    }
    
    // Registry access
    entt::registry& GetRegistry() { return m_Registry; }
    const entt::registry& GetRegistry() const { return m_Registry; }
    
    // Utility
    void Clear();
    u32 GetEntityCount() const;
    
    // Find entities
    Entity FindEntityByTag(const std::string& tag);
    std::vector<Entity> FindEntitiesByTag(const std::string& tag);
    
private:
    entt::registry m_Registry;
};

} // namespace Gini
