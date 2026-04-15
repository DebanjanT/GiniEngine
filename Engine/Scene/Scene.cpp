#include "Scene.h"
#include "Core/Logger.h"
#include "Renderer/Renderer3D.h"
#include <fstream>
#include <sstream>

namespace Gini {

Scene::Scene(const std::string &name) : m_Name(name) {
  GINI_INFO("Scene created: ", name);
}

Scene::~Scene() { Clear(); }

Entity Scene::CreateEntity(const std::string &name) {
  return CreateEntityWithUUID(m_NextUUID++, name);
}

Entity Scene::CreateEntityWithUUID(u64 uuid, const std::string &name) {
  // Use raw registry create to avoid duplicate component addition
  Entity entity = m_World.GetRegistry().create();

  m_World.AddComponent<TagComponent>(entity, name);
  m_World.AddComponent<TransformComponent>(entity);
  m_World.AddComponent<UUIDComponent>(entity, uuid);

  m_UUIDToEntity[uuid] = entity;

  return entity;
}

void Scene::DestroyEntity(Entity entity) {
  if (m_World.HasComponent<UUIDComponent>(entity)) {
    u64 uuid = m_World.GetComponent<UUIDComponent>(entity).uuid;
    m_UUIDToEntity.erase(uuid);
  }
  m_World.DestroyEntity(entity);
}

Entity Scene::DuplicateEntity(Entity entity) {
  if (!m_World.IsValid(entity))
    return NullEntity;

  std::string name = "Entity";
  if (m_World.HasComponent<TagComponent>(entity)) {
    name = m_World.GetComponent<TagComponent>(entity).tag + " (Copy)";
  }

  Entity newEntity = CreateEntity(name);

  if (m_World.HasComponent<TransformComponent>(entity)) {
    auto &src = m_World.GetComponent<TransformComponent>(entity);
    auto &dst = m_World.GetComponent<TransformComponent>(newEntity);
    dst = src;
  }

  if (m_World.HasComponent<SpriteComponent>(entity)) {
    auto &src = m_World.GetComponent<SpriteComponent>(entity);
    m_World.AddComponent<SpriteComponent>(newEntity, src);
  }

  if (m_World.HasComponent<StaticMeshComponent>(entity)) {
    auto &src = m_World.GetComponent<StaticMeshComponent>(entity);
    m_World.AddComponent<StaticMeshComponent>(newEntity, src);
  }

  if (m_World.HasComponent<MaterialComponent>(entity)) {
    auto &src = m_World.GetComponent<MaterialComponent>(entity);
    m_World.AddComponent<MaterialComponent>(newEntity, src);
  }

  if (m_World.HasComponent<LightComponent>(entity)) {
    auto &src = m_World.GetComponent<LightComponent>(entity);
    m_World.AddComponent<LightComponent>(newEntity, src);
  }

  if (m_World.HasComponent<CameraComponent>(entity)) {
    auto &src = m_World.GetComponent<CameraComponent>(entity);
    m_World.AddComponent<CameraComponent>(newEntity, src);
  }

  if (m_World.HasComponent<SkyboxComponent>(entity)) {
    auto &src = m_World.GetComponent<SkyboxComponent>(entity);
    m_World.AddComponent<SkyboxComponent>(newEntity, src);
  }

  if (m_World.HasComponent<AnimatorComponent3D>(entity)) {
    auto &src = m_World.GetComponent<AnimatorComponent3D>(entity);
    m_World.AddComponent<AnimatorComponent3D>(newEntity, src);
  }

  return newEntity;
}

Entity Scene::FindEntityByName(const std::string &name) {
  auto view = m_World.GetRegistry().view<TagComponent>();
  for (auto entity : view) {
    auto &tag = view.get<TagComponent>(entity);
    if (tag.tag == name) {
      return entity;
    }
  }
  return NullEntity;
}

Entity Scene::FindEntityByUUID(u64 uuid) {
  auto it = m_UUIDToEntity.find(uuid);
  if (it != m_UUIDToEntity.end()) {
    return it->second;
  }
  return NullEntity;
}

void Scene::OnStart() {
  m_Running = true;
  m_Paused = false;
  GINI_INFO("Scene started: ", m_Name);
}

void Scene::OnStop() {
  m_Running = false;
  GINI_INFO("Scene stopped: ", m_Name);
}

void Scene::OnUpdate(f32 deltaTime) {
  if (!m_Running || m_Paused)
    return;

  // Update scripts, physics, etc.
}

void Scene::OnRender() {
  // 2D rendering
}

void Scene::OnRender3D() {
  // Render StaticMeshComponents (new Hazel-style)
  {
    auto view = m_World.GetRegistry().view<TransformComponent, StaticMeshComponent>();
    for (auto entity : view) {
      auto &transform = view.get<TransformComponent>(entity);
      auto &staticMesh = view.get<StaticMeshComponent>(entity);

      if (!staticMesh.visible || staticMesh.meshSourceHandle == 0) {
        continue;
      }

      Mat4 transformMatrix = transform.GetTransform();
      Renderer3D::RenderStaticMesh(staticMesh.meshSourceHandle, transformMatrix,
                                   staticMesh.submeshIndices,
                                   staticMesh.materialOverrides);
    }
  }

  // Render DynamicMeshComponents (new Hazel-style, for rigged meshes)
  {
    auto view = m_World.GetRegistry().view<TransformComponent, DynamicMeshComponent>();
    for (auto entity : view) {
      auto &transform = view.get<TransformComponent>(entity);
      auto &dynamicMesh = view.get<DynamicMeshComponent>(entity);

      if (!dynamicMesh.visible || dynamicMesh.meshSourceHandle == 0) {
        continue;
      }

      Mat4 transformMatrix = transform.GetTransform();
      Renderer3D::RenderStaticMesh(dynamicMesh.meshSourceHandle, transformMatrix,
                                   dynamicMesh.submeshIndices,
                                   dynamicMesh.materialOverrides);
    }
  }

}

void Scene::SetParent(Entity child, Entity parent) {
  if (!m_World.HasComponent<HierarchyComponent>(child)) {
    m_World.AddComponent<HierarchyComponent>(child);
  }
  auto &hierarchy = m_World.GetComponent<HierarchyComponent>(child);
  hierarchy.parent = parent;
}

void Scene::RemoveParent(Entity child) {
  if (m_World.HasComponent<HierarchyComponent>(child)) {
    auto &hierarchy = m_World.GetComponent<HierarchyComponent>(child);
    hierarchy.parent = NullEntity;
  }
}

Entity Scene::GetParent(Entity entity) {
  if (m_World.HasComponent<HierarchyComponent>(entity)) {
    return m_World.GetComponent<HierarchyComponent>(entity).parent;
  }
  return NullEntity;
}

std::vector<Entity> Scene::GetChildren(Entity parent) {
  std::vector<Entity> children;
  auto view = m_World.GetRegistry().view<HierarchyComponent>();
  for (auto entity : view) {
    auto &hierarchy = view.get<HierarchyComponent>(entity);
    if (hierarchy.parent == parent) {
      children.push_back(entity);
    }
  }
  return children;
}

std::vector<Entity> Scene::GetRootEntities() {
  std::vector<Entity> roots;
  auto view = m_World.GetRegistry().view<TagComponent>();
  for (auto entity : view) {
    if (!m_World.HasComponent<HierarchyComponent>(entity) ||
        m_World.GetComponent<HierarchyComponent>(entity).parent == NullEntity) {
      roots.push_back(entity);
    }
  }
  return roots;
}

void Scene::Save(const std::string &filepath) {
  m_Filepath = filepath;

  std::ofstream file(filepath);
  if (!file.is_open()) {
    GINI_ERROR("Failed to save scene: ", filepath);
    return;
  }

  file << "# Gini Scene File\n";
  file << "name: " << m_Name << "\n";
  file << "entities:\n";

  auto view = m_World.GetRegistry().view<TagComponent, UUIDComponent>();
  for (auto entity : view) {
    auto &tag = view.get<TagComponent>(entity);
    auto &uuid = view.get<UUIDComponent>(entity);

    file << "  - uuid: " << uuid.uuid << "\n";
    file << "    name: " << tag.tag << "\n";

    if (m_World.HasComponent<TransformComponent>(entity)) {
      auto &t = m_World.GetComponent<TransformComponent>(entity);
      file << "    transform:\n";
      file << "      position: [" << t.position.x << ", " << t.position.y
           << ", " << t.position.z << "]\n";
      file << "      rotation: [" << t.rotation.x << ", " << t.rotation.y
           << ", " << t.rotation.z << "]\n";
      file << "      scale: [" << t.scale.x << ", " << t.scale.y << ", "
           << t.scale.z << "]\n";
    }
  }

  file.close();
  GINI_INFO("Scene saved: ", filepath);
}

Ref<Scene> Scene::Load(const std::string &filepath) {
  // TODO: Implement proper YAML/JSON parsing
  auto scene = CreateRef<Scene>("Loaded Scene");
  scene->m_Filepath = filepath;
  GINI_INFO("Scene loaded: ", filepath);
  return scene;
}

void Scene::Clear() {
  m_World.Clear();
  m_UUIDToEntity.clear();
  m_NextUUID = 1;
}

Scene::SceneStats Scene::GetStats() const {
  SceneStats stats;
  stats.entityCount = m_World.GetEntityCount();
  // TODO: Count meshes, lights, cameras
  return stats;
}

// SceneManager implementation
Ref<Scene> SceneManager::CreateScene(const std::string &name) {
  auto scene = CreateRef<Scene>(name);
  m_LoadedScenes.push_back(scene);
  return scene;
}

Ref<Scene> SceneManager::LoadScene(const std::string &filepath) {
  auto scene = Scene::Load(filepath);
  m_LoadedScenes.push_back(scene);
  return scene;
}

void SceneManager::SaveScene(Ref<Scene> scene, const std::string &filepath) {
  if (scene) {
    scene->Save(filepath);
  }
}

void SceneManager::UnloadScene(Ref<Scene> scene) {
  auto it = std::find(m_LoadedScenes.begin(), m_LoadedScenes.end(), scene);
  if (it != m_LoadedScenes.end()) {
    m_LoadedScenes.erase(it);
  }
  if (m_ActiveScene == scene) {
    m_ActiveScene = nullptr;
  }
}

void SceneManager::SetActiveScene(Ref<Scene> scene) { m_ActiveScene = scene; }

void Scene::LoadTerrainFromFile(const std::string &filepath) {
  if (filepath.empty()) {
    return;
  }

  // Create terrain and load from file
  m_Terrain = Terrain::Create();
  m_Terrain->LoadTerrain(filepath);
  m_TerrainPath = filepath;

  GINI_INFO("Loaded terrain for scene: {}", filepath);
}

void Scene::EnableAtmosphericSky(bool enable) {
  m_UseAtmosphericSky = enable;
  if (enable && !m_AtmosphericSky) {
    m_AtmosphericSky = AtmosphericSky::Create();
    m_AtmosphericSky->Initialize();
  }
}

void Scene::EnableSkybox(bool enable) {
  m_UseSkybox = enable;
  if (enable && !m_Skybox) {
    m_Skybox = Skybox::Create();
  }
}

} // namespace Gini
