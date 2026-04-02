#pragma once

#include "Core/Types.h"
#include "Renderer/Texture.h"
#include <string>
#include <vector>

namespace Gini {

// Core Components
struct UUIDComponent {
  u64 uuid = 0;
  UUIDComponent() = default;
  UUIDComponent(u64 id) : uuid(id) {}
};

struct TagComponent {
  std::string tag;
  TagComponent() = default;
  TagComponent(const std::string &t) : tag(t) {}
};

struct HierarchyComponent {
  Entity parent = NullEntity;
  std::vector<Entity> children;
};

struct TransformComponent {
  Vec3 position{0.0f, 0.0f, 0.0f};
  Vec3 rotation{0.0f, 0.0f, 0.0f};
  Vec3 scale{1.0f, 1.0f, 1.0f};

  Mat4 GetTransform() const;
};

struct SpriteComponent {
  Color color = Color::White();
  Ref<Texture2D> texture = nullptr;
  Rect uvRect{0.0f, 0.0f, 1.0f, 1.0f};
  i32 zOrder = 0;
};

// RTS-Specific Components
struct SelectableComponent {
  bool selected = false;
  bool hovered = false;
  f32 selectionRadius = 32.0f;
};

struct UnitComponent {
  std::string unitType;
  i32 playerId = 0;
  f32 health = 100.0f;
  f32 maxHealth = 100.0f;
  f32 armor = 0.0f;
  f32 moveSpeed = 100.0f;
  f32 attackDamage = 10.0f;
  f32 attackRange = 50.0f;
  f32 attackSpeed = 1.0f;
  f32 lineOfSight = 200.0f;
};

struct BuildingComponent {
  std::string buildingType;
  i32 playerId = 0;
  f32 health = 500.0f;
  f32 maxHealth = 500.0f;
  bool isConstructing = false;
  f32 constructionProgress = 0.0f;
  IVec2 tileSize{2, 2};
};

struct ResourceComponent {
  std::string resourceType; // "gold", "wood", "stone", etc.
  i32 amount = 1000;
  i32 maxAmount = 1000;
};

struct MovementComponent {
  Vec2 targetPosition{0.0f, 0.0f};
  std::vector<Vec2> path;
  u32 currentPathIndex = 0;
  bool isMoving = false;
  f32 arrivalThreshold = 5.0f;
};

struct CommandComponent {
  enum class CommandType { None, Move, Attack, Gather, Build, Patrol, Stop };

  CommandType currentCommand = CommandType::None;
  Vec2 targetPosition{0.0f, 0.0f};
  u64 targetEntity = 0; // EnTT entity ID
  bool commandQueued = false;
};

struct AnimationComponent {
  u32 currentFrame = 0;
  u32 frameCount = 1;
  f32 frameTime = 0.1f;
  f32 elapsedTime = 0.0f;
  bool loop = true;
  bool playing = true;
  std::string currentAnimation;
};

struct ColliderComponent {
  enum class Type { Box, Circle };
  Type type = Type::Box;
  Vec2 size{32.0f, 32.0f};
  Vec2 offset{0.0f, 0.0f};
  f32 radius = 16.0f; // For circle collider
  bool isTrigger = false;
};

struct FogOfWarComponent {
  bool revealed = false;
  bool visible = false;
  f32 visionRange = 200.0f;
};

// AI Components
struct AIComponent {
  enum class State { Idle, Moving, Attacking, Gathering, Fleeing, Patrolling };
  State currentState = State::Idle;
  f32 aggroRange = 150.0f;
  f32 fleeHealthThreshold = 0.2f;
  u64 targetEntity = 0;
};

// 3D Rendering Components
enum class MeshType { None = 0, Cube, Sphere, Plane, Cylinder, Custom };

struct MeshComponent {
  MeshType meshType = MeshType::Cube;
  std::string modelPath;
  bool castShadows = true;
  bool receiveShadows = true;
};

struct MaterialComponent {
  Vec3 albedo{1.0f, 1.0f, 1.0f};
  f32 metallic = 0.0f;
  f32 roughness = 0.5f;
  f32 ao = 1.0f;
  Vec3 emissive{0.0f, 0.0f, 0.0f};
  std::string albedoTexturePath;
  std::string normalTexturePath;
  std::string metallicTexturePath;
  std::string roughnessTexturePath;
  std::string aoTexturePath;
};

struct LightComponent {
  int type = 1; // 0=Directional, 1=Point, 2=Spot (matches LightType enum)
  Vec3 color{1.0f, 1.0f, 1.0f};
  f32 intensity = 1.0f;
  f32 range = 10.0f;
  f32 innerConeAngle = 30.0f;
  f32 outerConeAngle = 45.0f;
  bool castShadows = true;
};

struct CameraComponent {
  bool isPrimary = true;
  f32 fov = 45.0f;
  f32 nearClip = 0.1f;
  f32 farClip = 1000.0f;
  f32 aspectRatio = 16.0f / 9.0f;
};

// Skybox Component
struct SkyboxComponent {
  std::string hdrPath;
  std::string cubemapPaths[6]; // For non-HDR cubemaps
  f32 intensity = 1.0f;
  f32 lod = 0.0f;
  bool useHDR = true;
};

} // namespace Gini
