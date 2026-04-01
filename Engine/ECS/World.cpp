#include "World.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Gini {

Entity World::CreateEntity(const std::string &name) {
  Entity entity = m_Registry.create();
  AddComponent<TagComponent>(entity, name);
  AddComponent<TransformComponent>(entity);
  return entity;
}

void World::DestroyEntity(Entity entity) { m_Registry.destroy(entity); }

bool World::IsValid(Entity entity) const { return m_Registry.valid(entity); }

void World::Clear() { m_Registry.clear(); }

u32 World::GetEntityCount() const {
  return static_cast<u32>(m_Registry.storage<entt::entity>()->size());
}

Entity World::FindEntityByTag(const std::string &tag) {
  auto view = m_Registry.view<TagComponent>();
  for (auto entity : view) {
    if (view.get<TagComponent>(entity).tag == tag) {
      return entity;
    }
  }
  return NullEntity;
}

std::vector<Entity> World::FindEntitiesByTag(const std::string &tag) {
  std::vector<Entity> result;
  auto view = m_Registry.view<TagComponent>();
  for (auto entity : view) {
    if (view.get<TagComponent>(entity).tag == tag) {
      result.push_back(entity);
    }
  }
  return result;
}

Mat4 TransformComponent::GetTransform() const {
  Mat4 rot = glm::rotate(Mat4(1.0f), rotation.x, Vec3(1, 0, 0)) *
             glm::rotate(Mat4(1.0f), rotation.y, Vec3(0, 1, 0)) *
             glm::rotate(Mat4(1.0f), rotation.z, Vec3(0, 0, 1));

  return glm::translate(Mat4(1.0f), position) * rot *
         glm::scale(Mat4(1.0f), scale);
}

} // namespace Gini
