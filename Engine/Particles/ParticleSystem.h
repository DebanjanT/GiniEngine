#pragma once

#include "Core/Types.h"
#include "Renderer/Texture.h"
#include <vector>

namespace Gini {

struct Particle {
    Vec3 position = Vec3(0.0f);
    Vec3 velocity = Vec3(0.0f);
    Vec4 color = Vec4(1.0f);
    Vec4 colorBegin = Vec4(1.0f);
    Vec4 colorEnd = Vec4(1.0f, 1.0f, 1.0f, 0.0f);
    f32 size = 1.0f;
    f32 sizeBegin = 1.0f;
    f32 sizeEnd = 0.0f;
    f32 rotation = 0.0f;
    f32 rotationSpeed = 0.0f;
    f32 lifetime = 1.0f;
    f32 lifeRemaining = 1.0f;
    bool active = false;
};

struct ParticleEmitterProperties {
    Vec3 position = Vec3(0.0f);
    Vec3 positionVariance = Vec3(0.0f);
    
    Vec3 velocity = Vec3(0.0f, 1.0f, 0.0f);
    Vec3 velocityVariance = Vec3(0.5f);
    
    Vec3 gravity = Vec3(0.0f, -9.8f, 0.0f);
    
    Vec4 colorBegin = Vec4(1.0f, 0.5f, 0.0f, 1.0f);
    Vec4 colorEnd = Vec4(1.0f, 0.0f, 0.0f, 0.0f);
    
    f32 sizeBegin = 0.5f;
    f32 sizeEnd = 0.0f;
    f32 sizeVariance = 0.1f;
    
    f32 lifetime = 2.0f;
    f32 lifetimeVariance = 0.5f;
    
    f32 rotationSpeed = 0.0f;
    f32 rotationSpeedVariance = 0.0f;
    
    f32 emissionRate = 10.0f; // particles per second
    u32 burstCount = 0; // particles per burst (0 = continuous)
    
    Ref<Texture2D> texture;
    bool additiveBlending = true;
};

class ParticleEmitter {
public:
    ParticleEmitter(u32 maxParticles = 1000);
    ~ParticleEmitter() = default;
    
    void Update(f32 deltaTime);
    void Emit(const ParticleEmitterProperties& props, u32 count = 1);
    void Burst(const ParticleEmitterProperties& props);
    void Clear();
    
    void SetProperties(const ParticleEmitterProperties& props) { m_Properties = props; }
    ParticleEmitterProperties& GetProperties() { return m_Properties; }
    
    void SetActive(bool active) { m_Active = active; }
    bool IsActive() const { return m_Active; }
    
    const std::vector<Particle>& GetParticles() const { return m_Particles; }
    u32 GetActiveParticleCount() const { return m_ActiveCount; }
    u32 GetMaxParticles() const { return m_MaxParticles; }
    
private:
    void EmitParticle(const ParticleEmitterProperties& props);
    f32 RandomFloat(f32 min, f32 max);
    
    std::vector<Particle> m_Particles;
    ParticleEmitterProperties m_Properties;
    
    u32 m_MaxParticles;
    u32 m_ActiveCount = 0;
    u32 m_PoolIndex = 0;
    
    f32 m_EmissionAccumulator = 0.0f;
    bool m_Active = true;
};

class ParticleSystem {
public:
    static void Init();
    static void Shutdown();
    
    static void Update(f32 deltaTime);
    static void Render(const Mat4& viewProjection);
    
    static Ref<ParticleEmitter> CreateEmitter(u32 maxParticles = 1000);
    static void AddEmitter(Ref<ParticleEmitter> emitter);
    static void RemoveEmitter(Ref<ParticleEmitter> emitter);
    
    static void SetGlobalGravity(const Vec3& gravity);
    static void SetGlobalTimeScale(f32 scale);
    
    static u32 GetTotalParticleCount();
    static u32 GetActiveEmitterCount();
    
private:
    static std::vector<Ref<ParticleEmitter>> s_Emitters;
    static Vec3 s_GlobalGravity;
    static f32 s_TimeScale;
    static bool s_Initialized;
};

} // namespace Gini
