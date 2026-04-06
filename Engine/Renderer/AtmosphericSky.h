#pragma once

#include "Core/Types.h"
#include "Renderer/Camera3D.h"
#include "Renderer/Shader.h"

namespace Gini {

struct SunSettings {
  Vec3 direction = glm::normalize(Vec3(-0.5f, -0.8f, -0.3f));
  Vec3 color = Vec3(1.0f, 0.95f, 0.9f);
  f32 intensity = 3.0f;
  f32 diskSize = 0.02f; // Angular size of sun disk
};

struct AtmosphereSettings {
  // Rayleigh scattering (blue sky)
  Vec3 rayleighCoeff = Vec3(5.8e-6f, 13.5e-6f, 33.1e-6f);
  f32 rayleighScale = 8000.0f; // Scale height in meters

  // Mie scattering (haze/fog)
  Vec3 mieCoeff = Vec3(21e-6f);
  f32 mieScale = 1200.0f;
  f32 mieG = 0.76f; // Anisotropy factor (-1 to 1)

  // Atmosphere dimensions
  f32 planetRadius = 6371000.0f;     // Earth radius in meters
  f32 atmosphereRadius = 6471000.0f; // Atmosphere top

  // Ground settings
  Vec3 groundColor = Vec3(0.37f, 0.35f, 0.34f);
  f32 groundBrightness = 0.4f;
};

struct FogSettings {
  bool enabled = true;
  Vec3 color = Vec3(0.7f, 0.8f, 0.9f);
  f32 density = 0.0001f;
  f32 heightFalloff = 0.001f;
  f32 startDistance = 10.0f;
  f32 maxOpacity = 0.95f;
  bool volumetric = true;
  f32 scatteringIntensity = 0.5f;
};

struct CloudSettings {
  bool enabled = true;
  f32 coverage = 0.5f;     // 0-1, how much of sky is covered
  f32 density = 1.0f;      // Cloud density multiplier
  f32 height = 2000.0f;    // Cloud layer base height in meters
  f32 thickness = 1500.0f; // Cloud layer thickness in meters
  f32 speed = 1.0f;        // Wind speed multiplier
};

class AtmosphericSky {
public:
  AtmosphericSky();
  ~AtmosphericSky();

  void Initialize();
  void Render(const Camera3D &camera);
  void Update(f32 deltaTime) { m_Time += deltaTime * m_Clouds.speed; }

  // Settings access
  SunSettings &GetSunSettings() { return m_Sun; }
  AtmosphereSettings &GetAtmosphereSettings() { return m_Atmosphere; }
  FogSettings &GetFogSettings() { return m_Fog; }
  CloudSettings &GetCloudSettings() { return m_Clouds; }

  const SunSettings &GetSunSettings() const { return m_Sun; }
  const AtmosphereSettings &GetAtmosphereSettings() const {
    return m_Atmosphere;
  }
  const FogSettings &GetFogSettings() const { return m_Fog; }
  const CloudSettings &GetCloudSettings() const { return m_Clouds; }

  // Sun direction helpers
  void SetSunDirection(const Vec3 &dir) {
    m_Sun.direction = glm::normalize(dir);
  }
  void SetSunFromTimeOfDay(f32 timeHours); // 0-24 hours
  Vec3 GetSunDirection() const { return m_Sun.direction; }
  Vec3 GetSunColor() const { return m_Sun.color * m_Sun.intensity; }

  // Get ambient light color based on sky
  Vec3 GetAmbientColor() const;
  Vec3 GetHorizonColor() const;
  Vec3 GetZenithColor() const;

  static Ref<AtmosphericSky> Create();

private:
  void CreateSkyMesh();
  void InitShaders();

  SunSettings m_Sun;
  AtmosphereSettings m_Atmosphere;
  FogSettings m_Fog;
  CloudSettings m_Clouds;

  Ref<Shader> m_SkyShader;
  u32 m_SkyVAO = 0;
  u32 m_SkyVBO = 0;

  f32 m_Time = 0.0f;
  bool m_Initialized = false;
};

} // namespace Gini
