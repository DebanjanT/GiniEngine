#include "ParticleSystem.h"
#include "Core/Logger.h"
#include "Renderer/Shader.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <random>
#include <algorithm>

namespace Gini {

// Static members
std::vector<Ref<ParticleEmitter>> ParticleSystem::s_Emitters;
Vec3 ParticleSystem::s_GlobalGravity = Vec3(0.0f, -9.8f, 0.0f);
f32 ParticleSystem::s_TimeScale = 1.0f;
bool ParticleSystem::s_Initialized = false;

static std::mt19937 s_RandomEngine;
static std::uniform_real_distribution<f32> s_Distribution(0.0f, 1.0f);

// ParticleEmitter implementation
ParticleEmitter::ParticleEmitter(u32 maxParticles) : m_MaxParticles(maxParticles) {
    m_Particles.resize(maxParticles);
}

void ParticleEmitter::Update(f32 deltaTime) {
    if (!m_Active) return;
    
    // Emit new particles based on emission rate
    if (m_Properties.emissionRate > 0.0f) {
        m_EmissionAccumulator += m_Properties.emissionRate * deltaTime;
        while (m_EmissionAccumulator >= 1.0f) {
            EmitParticle(m_Properties);
            m_EmissionAccumulator -= 1.0f;
        }
    }
    
    // Update existing particles
    m_ActiveCount = 0;
    for (auto& particle : m_Particles) {
        if (!particle.active) continue;
        
        particle.lifeRemaining -= deltaTime;
        if (particle.lifeRemaining <= 0.0f) {
            particle.active = false;
            continue;
        }
        
        // Apply gravity and velocity
        particle.velocity += m_Properties.gravity * deltaTime;
        particle.position += particle.velocity * deltaTime;
        particle.rotation += particle.rotationSpeed * deltaTime;
        
        // Interpolate color and size
        f32 life = particle.lifeRemaining / particle.lifetime;
        particle.color = glm::mix(particle.colorEnd, particle.colorBegin, life);
        particle.size = glm::mix(particle.sizeEnd, particle.sizeBegin, life);
        
        m_ActiveCount++;
    }
}

void ParticleEmitter::Emit(const ParticleEmitterProperties& props, u32 count) {
    for (u32 i = 0; i < count; i++) {
        EmitParticle(props);
    }
}

void ParticleEmitter::Burst(const ParticleEmitterProperties& props) {
    u32 count = props.burstCount > 0 ? props.burstCount : 10;
    Emit(props, count);
}

void ParticleEmitter::Clear() {
    for (auto& particle : m_Particles) {
        particle.active = false;
    }
    m_ActiveCount = 0;
}

void ParticleEmitter::EmitParticle(const ParticleEmitterProperties& props) {
    Particle& particle = m_Particles[m_PoolIndex];
    m_PoolIndex = (m_PoolIndex + 1) % m_MaxParticles;
    
    particle.active = true;
    
    // Position with variance
    particle.position = props.position;
    particle.position.x += (RandomFloat(-1.0f, 1.0f)) * props.positionVariance.x;
    particle.position.y += (RandomFloat(-1.0f, 1.0f)) * props.positionVariance.y;
    particle.position.z += (RandomFloat(-1.0f, 1.0f)) * props.positionVariance.z;
    
    // Velocity with variance
    particle.velocity = props.velocity;
    particle.velocity.x += (RandomFloat(-1.0f, 1.0f)) * props.velocityVariance.x;
    particle.velocity.y += (RandomFloat(-1.0f, 1.0f)) * props.velocityVariance.y;
    particle.velocity.z += (RandomFloat(-1.0f, 1.0f)) * props.velocityVariance.z;
    
    // Color
    particle.colorBegin = props.colorBegin;
    particle.colorEnd = props.colorEnd;
    particle.color = props.colorBegin;
    
    // Size with variance
    f32 sizeVariance = (RandomFloat(-1.0f, 1.0f)) * props.sizeVariance;
    particle.sizeBegin = props.sizeBegin + sizeVariance;
    particle.sizeEnd = props.sizeEnd;
    particle.size = particle.sizeBegin;
    
    // Lifetime with variance
    f32 lifetimeVariance = (RandomFloat(-1.0f, 1.0f)) * props.lifetimeVariance;
    particle.lifetime = props.lifetime + lifetimeVariance;
    particle.lifeRemaining = particle.lifetime;
    
    // Rotation
    particle.rotation = RandomFloat(0.0f, 360.0f);
    particle.rotationSpeed = props.rotationSpeed + (RandomFloat(-1.0f, 1.0f)) * props.rotationSpeedVariance;
}

f32 ParticleEmitter::RandomFloat(f32 min, f32 max) {
    return min + s_Distribution(s_RandomEngine) * (max - min);
}

// ParticleSystem implementation
void ParticleSystem::Init() {
    if (s_Initialized) return;
    
    s_RandomEngine.seed(std::random_device{}());
    s_Initialized = true;
    
    GINI_INFO("ParticleSystem initialized");
}

void ParticleSystem::Shutdown() {
    s_Emitters.clear();
    s_Initialized = false;
    GINI_INFO("ParticleSystem shutdown");
}

void ParticleSystem::Update(f32 deltaTime) {
    f32 scaledDelta = deltaTime * s_TimeScale;
    
    for (auto& emitter : s_Emitters) {
        if (emitter) {
            emitter->Update(scaledDelta);
        }
    }
}

void ParticleSystem::Render(const Mat4& viewProjection) {
    // TODO: Implement GPU-based particle rendering with instancing
    // For now, particles can be rendered using the SpriteBatch or a custom particle shader
}

Ref<ParticleEmitter> ParticleSystem::CreateEmitter(u32 maxParticles) {
    auto emitter = CreateRef<ParticleEmitter>(maxParticles);
    s_Emitters.push_back(emitter);
    return emitter;
}

void ParticleSystem::AddEmitter(Ref<ParticleEmitter> emitter) {
    if (emitter) {
        s_Emitters.push_back(emitter);
    }
}

void ParticleSystem::RemoveEmitter(Ref<ParticleEmitter> emitter) {
    s_Emitters.erase(
        std::remove(s_Emitters.begin(), s_Emitters.end(), emitter),
        s_Emitters.end()
    );
}

void ParticleSystem::SetGlobalGravity(const Vec3& gravity) {
    s_GlobalGravity = gravity;
}

void ParticleSystem::SetGlobalTimeScale(f32 scale) {
    s_TimeScale = scale;
}

u32 ParticleSystem::GetTotalParticleCount() {
    u32 total = 0;
    for (const auto& emitter : s_Emitters) {
        if (emitter) {
            total += emitter->GetActiveParticleCount();
        }
    }
    return total;
}

u32 ParticleSystem::GetActiveEmitterCount() {
    return static_cast<u32>(s_Emitters.size());
}

} // namespace Gini
