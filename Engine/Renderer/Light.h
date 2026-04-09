#pragma once

#include "Core/Types.h"
#include <vector>

namespace Gini {

// Forward declarations for Diligent Engine integration
class DiligentLight;

enum class LightType { Directional, Point, Spot };

struct DirectionalLight {
  Vec3 direction = Vec3(-0.2f, -1.0f, -0.3f);
  Vec3 color = Vec3(1.0f);
  f32 intensity = 1.0f;
  bool castShadows = true;
};

struct PointLight {
  Vec3 position = Vec3(0.0f);
  Vec3 color = Vec3(1.0f);
  f32 intensity = 1.0f;
  f32 radius = 10.0f;
  f32 constant = 1.0f;
  f32 linear = 0.09f;
  f32 quadratic = 0.032f;
  bool castShadows = false;
};

struct SpotLight {
  Vec3 position = Vec3(0.0f);
  Vec3 direction = Vec3(0.0f, -1.0f, 0.0f);
  Vec3 color = Vec3(1.0f);
  f32 intensity = 1.0f;
  f32 innerCutoff = 12.5f; // degrees
  f32 outerCutoff = 17.5f; // degrees
  f32 radius = 50.0f;
  f32 constant = 1.0f;
  f32 linear = 0.09f;
  f32 quadratic = 0.032f;
  bool castShadows = false;
};

struct AmbientLight {
  Vec3 color = Vec3(0.3f, 0.35f, 0.4f); // Warmer, brighter ambient light
  f32 intensity = 0.5f; // Moderate intensity for better visibility
};

class LightManager {
public:
  static LightManager &Get() {
    static LightManager instance;
    return instance;
  }

  void Clear();

  // Directional light (usually just one - the sun)
  void SetDirectionalLight(const DirectionalLight &light);
  DirectionalLight &GetDirectionalLight() { return m_DirectionalLight; }
  const DirectionalLight &GetDirectionalLight() const {
    return m_DirectionalLight;
  }
  bool HasDirectionalLight() const { return m_HasDirectionalLight; }

  // Point lights
  void AddPointLight(const PointLight &light);
  void RemovePointLight(u32 index);
  PointLight &GetPointLight(u32 index) { return m_PointLights[index]; }
  const std::vector<PointLight> &GetPointLights() const {
    return m_PointLights;
  }
  u32 GetPointLightCount() const {
    return static_cast<u32>(m_PointLights.size());
  }

  // Spot lights
  void AddSpotLight(const SpotLight &light);
  void RemoveSpotLight(u32 index);
  SpotLight &GetSpotLight(u32 index) { return m_SpotLights[index]; }
  const std::vector<SpotLight> &GetSpotLights() const { return m_SpotLights; }
  u32 GetSpotLightCount() const {
    return static_cast<u32>(m_SpotLights.size());
  }

  // Ambient
  void SetAmbientLight(const AmbientLight &light) { m_AmbientLight = light; }
  AmbientLight &GetAmbientLight() { return m_AmbientLight; }
  const AmbientLight &GetAmbientLight() const { return m_AmbientLight; }

  // Upload to shader
  void UploadToShader(class Shader *shader) const;

  // Diligent Engine integration methods
  void CreateDiligentLights();
  void UpdateDiligentLights();
  void RegisterWithHybridManager();
  bool HasDiligentResources() const { return m_HasDiligentResources; }

  static constexpr u32 MAX_POINT_LIGHTS = 32;
  static constexpr u32 MAX_SPOT_LIGHTS = 16;

private:
  LightManager() = default;

  DirectionalLight m_DirectionalLight;
  bool m_HasDirectionalLight = false;

  std::vector<PointLight> m_PointLights;
  std::vector<SpotLight> m_SpotLights;
  AmbientLight m_AmbientLight;

  // Diligent Engine resources
  bool m_HasDiligentResources = false;
};

} // namespace Gini
