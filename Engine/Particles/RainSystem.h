#pragma once

#include "Core/Types.h"
#include "Renderer/Framebuffer.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture.h"
#include <memory>
#include <vector>

namespace Gini {

// Optimized rain particle structure (SoA - Structure of Arrays for SIMD)
struct RainParticles {
  std::vector<Vec3> positions;
  std::vector<Vec3> velocities;
  std::vector<f32> lifeTimes;
  std::vector<bool> active;

  u32 maxParticles;
  u32 activeCount;
  u32 poolIndex;

  RainParticles(u32 maxCount)
      : maxParticles(maxCount), activeCount(0), poolIndex(0) {
    positions.resize(maxCount);
    velocities.resize(maxCount);
    lifeTimes.resize(maxCount);
    active.resize(maxCount, false);
  }
};

struct RainSettings {
  Vec3 position =
      Vec3(0.0f, 15.0f, 0.0f); // Rain spawn height (very close to camera)
  Vec3 areaSize = Vec3(30.0f, 0.0f, 30.0f); // Rain area (intimate, close-up)

  f32 fallSpeed = 50.0f;    // Base fall speed
  f32 windStrength = 5.0f;  // Wind effect
  f32 windVariation = 2.0f; // Wind randomness

  Vec4 color =
      Vec4(0.9f, 0.9f, 1.0f, 0.9f); // Rain color (brighter, more opaque)
  f32 size = 0.08f; // Rain drop size (realistic cylindrical drops)

  f32 emissionRate =
      500.0f; // Particles per second (further reduced for performance)
  f32 groundLevel = 0.0f; // Kill particles below this

  bool enabled = true;
};

class RainEmitter {
public:
  RainEmitter(u32 maxParticles = 5000);
  ~RainEmitter() = default;

  void Update(f32 deltaTime);
  void Render(const Mat4 &viewProjection);

  void SetSettings(const RainSettings &settings) { m_Settings = settings; }
  RainSettings &GetSettings() { return m_Settings; }

  void SetActive(bool active) { m_Active = active; }
  bool IsActive() const { return m_Active; }

  u32 GetActiveParticleCount() const { return m_Particles.activeCount; }
  u32 GetMaxParticles() const { return m_Particles.maxParticles; }

  void Clear();

private:
  void EmitRain(u32 count);
  void UpdateParticles(f32 deltaTime);

  RainParticles m_Particles;
  RainSettings m_Settings;

  f32 m_EmissionAccumulator = 0.0f;
  bool m_Active = true;

  // Rendering
  Ref<Shader> m_RainShader;
  u32 m_VAO = 0;
  u32 m_VBO = 0;

  void InitRendering();
};

// Lightning bolt segment
struct LightningSegment {
  Vec3 start;
  Vec3 end;
  f32 thickness;
  f32 intensity;
};

struct LightningBolt {
  std::vector<LightningSegment> segments;
  Vec3 origin;
  Vec3 target;
  f32 lifetime;
  f32 lifeRemaining;
  f32 intensity;
  bool active;

  LightningBolt()
      : lifetime(0.2f), lifeRemaining(0.2f), intensity(1.0f), active(false) {}
};

struct ThunderSettings {
  f32 strikeFrequency = 15.0f; // Average time between strikes (seconds) -
                               // further reduced frequency
  f32 strikeVariation = 8.0f;  // Random variation in strike timing
  f32 boltIntensity = 1.0f;
  f32 glowRadius = 2.0f;
  Vec4 boltColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
  Vec4 glowColor = Vec4(0.5f, 0.7f, 1.0f, 0.3f);

  bool enabled = true;
  bool enableGlow = true; // Enable for better thunder effects
  bool enableBranches = true;
};

class ThunderSystem {
public:
  ThunderSystem();
  ~ThunderSystem() = default;

  void Update(f32 deltaTime);
  void Render(const Mat4 &viewProjection);

  void SetSettings(const ThunderSettings &settings) { m_Settings = settings; }
  ThunderSettings &GetSettings() { return m_Settings; }

  void SetActive(bool active) { m_Active = active; }
  bool IsActive() const { return m_Active; }

  void StrikeLightning(const Vec3 &origin, const Vec3 &target);
  void StrikeRandom();

  u32 GetActiveBoltCount() const { return m_ActiveBoltCount; }

  // Environmental flash for thunder strikes
  f32 GetEnvironmentalFlash() const { return m_EnvironmentalFlash; }

private:
  void GenerateLightningBolt(const Vec3 &origin, const Vec3 &target,
                             LightningBolt &bolt);
  void SubdivideSegment(Vec3 start, Vec3 end, f32 intensity,
                        std::vector<LightningSegment> &segments, u32 depth = 0);
  void RenderBolt(const LightningBolt &bolt, const Mat4 &viewProjection);
  void ApplyGlowEffect();

  std::vector<LightningBolt> m_Bolts;
  ThunderSettings m_Settings;

  f32 m_TimeSinceLastStrike = 0.0f;
  f32 m_NextStrikeTime = 5.0f;
  u32 m_ActiveBoltCount = 0;
  f32 m_EnvironmentalFlash = 0.0f;
  bool m_Active = true;

  // Rendering
  Ref<Shader> m_LightningShader;
  Ref<Framebuffer> m_GlowFramebuffer;
  Ref<Shader> m_GlowShader;
  u32 m_VAO = 0;
  u32 m_VBO = 0;

  void InitRendering();
  Vec3 RandomVec3(f32 min, f32 max);
};

// Combined weather system
class WeatherSystem {
public:
  static void Init();
  static void Shutdown();

  static void Update(f32 deltaTime);
  static void Render(const Mat4 &viewProjection);
  static void UpdateRainPosition(const Vec3 &cameraPosition);

  static Ref<RainEmitter> GetRainEmitter() { return s_RainEmitter; }
  static Ref<ThunderSystem> GetThunderSystem() { return s_ThunderSystem; }

  static void
  SetWeatherIntensity(f32 intensity); // 0-1, affects rain and thunder
  static void StartRain();
  static void StopRain();
  static void StartThunderstorm();
  static void StopThunderstorm();

  static bool IsRaining() { return s_RainEmitter && s_RainEmitter->IsActive(); }
  static bool IsThunderActive() {
    return s_ThunderSystem && s_ThunderSystem->IsActive();
  }

private:
  static Ref<RainEmitter> s_RainEmitter;
  static Ref<ThunderSystem> s_ThunderSystem;
  static bool s_Initialized;
  static f32 m_WeatherIntensity;
};

} // namespace Gini
