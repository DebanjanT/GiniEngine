#pragma once

#include "Core/Types.h"
#include "ECS/Components.h"
#include "ECS/World.h"
#include "Renderer/AtmosphericSky.h"
#include "Terrain/Terrain.h"
#include <string>
#include <vector>

namespace Gini {

class Scene {
public:
  Scene(const std::string &name = "Untitled Scene");
  ~Scene();

  // Entity management
  Entity CreateEntity(const std::string &name = "Entity");
  Entity CreateEntityWithUUID(u64 uuid, const std::string &name = "Entity");
  void DestroyEntity(Entity entity);
  Entity DuplicateEntity(Entity entity);
  Entity FindEntityByName(const std::string &name);
  Entity FindEntityByUUID(u64 uuid);

  // Scene lifecycle
  void OnStart();
  void OnStop();
  void OnUpdate(f32 deltaTime);
  void OnRender();
  void OnRender3D();

  // Hierarchy
  void SetParent(Entity child, Entity parent);
  void RemoveParent(Entity child);
  Entity GetParent(Entity entity);
  std::vector<Entity> GetChildren(Entity entity);
  std::vector<Entity> GetRootEntities();

  // Serialization
  void Save(const std::string &filepath);
  static Ref<Scene> Load(const std::string &filepath);
  void Clear();

  // Accessors
  const std::string &GetName() const { return m_Name; }
  void SetName(const std::string &name) { m_Name = name; }
  const std::string &GetFilepath() const { return m_Filepath; }

  World &GetWorld() { return m_World; }
  const World &GetWorld() const { return m_World; }

  u32 GetEntityCount() const { return m_World.GetEntityCount(); }
  bool IsRunning() const { return m_Running; }
  bool IsPaused() const { return m_Paused; }
  void SetPaused(bool paused) { m_Paused = paused; }

  // Scene statistics
  struct SceneStats {
    u32 entityCount = 0;
    u32 meshCount = 0;
    u32 lightCount = 0;
    u32 cameraCount = 0;
  };
  SceneStats GetStats() const;

  // Terrain (one per scene)
  void SetTerrain(Ref<Terrain> terrain) { m_Terrain = terrain; }
  Ref<Terrain> GetTerrain() const { return m_Terrain; }
  bool HasTerrain() const { return m_Terrain != nullptr; }
  void LoadTerrainFromFile(const std::string &filepath);
  const std::string &GetTerrainPath() const { return m_TerrainPath; }
  void SetTerrainPath(const std::string &path) { m_TerrainPath = path; }

  // Atmospheric Sky
  void SetAtmosphericSky(Ref<AtmosphericSky> sky) { m_AtmosphericSky = sky; }
  Ref<AtmosphericSky> GetAtmosphericSky() const { return m_AtmosphericSky; }
  bool HasAtmosphericSky() const { return m_AtmosphericSky != nullptr; }
  void EnableAtmosphericSky(bool enable);
  bool IsAtmosphericSkyEnabled() const { return m_UseAtmosphericSky; }

private:
  std::string m_Name;
  std::string m_Filepath;
  World m_World;

  bool m_Running = false;
  bool m_Paused = false;

  std::unordered_map<u64, Entity> m_UUIDToEntity;
  u64 m_NextUUID = 1;

  // Terrain (one terrain per scene/map)
  Ref<Terrain> m_Terrain;
  std::string m_TerrainPath;

  // Atmospheric Sky
  Ref<AtmosphericSky> m_AtmosphericSky;
  bool m_UseAtmosphericSky = true;
};

// Scene Manager for handling multiple scenes
class SceneManager {
public:
  static SceneManager &Get() {
    static SceneManager instance;
    return instance;
  }

  Ref<Scene> CreateScene(const std::string &name = "Untitled Scene");
  Ref<Scene> LoadScene(const std::string &filepath);
  void SaveScene(Ref<Scene> scene, const std::string &filepath);
  void UnloadScene(Ref<Scene> scene);

  void SetActiveScene(Ref<Scene> scene);
  Ref<Scene> GetActiveScene() { return m_ActiveScene; }

  const std::vector<Ref<Scene>> &GetLoadedScenes() const {
    return m_LoadedScenes;
  }

private:
  SceneManager() = default;

  Ref<Scene> m_ActiveScene;
  std::vector<Ref<Scene>> m_LoadedScenes;
};

} // namespace Gini
