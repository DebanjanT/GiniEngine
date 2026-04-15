#define GLM_ENABLE_EXPERIMENTAL
#include "RainSystem.h"
#include "Core/Logger.h"
#include "Renderer/Renderer3D.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <random>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

namespace Gini {

// Static members for WeatherSystem
Ref<RainEmitter> WeatherSystem::s_RainEmitter = nullptr;
Ref<ThunderSystem> WeatherSystem::s_ThunderSystem = nullptr;
bool WeatherSystem::s_Initialized = false;
f32 WeatherSystem::m_WeatherIntensity = 0.0f;

// Random engine for weather effects
static std::mt19937 s_RandomEngine(
    (unsigned int)std::chrono::steady_clock::now().time_since_epoch().count());
static std::uniform_real_distribution<f32> s_Distribution(0.0f, 1.0f);
static std::uniform_real_distribution<f32> s_DistributionNeg1(0.0f, 1.0f);

// RainEmitter Implementation
RainEmitter::RainEmitter(u32 maxParticles) : m_Particles(maxParticles) {
  InitRendering();
}

void RainEmitter::InitRendering() {
  // Optimized rain shader for instanced rendering
  const char *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPosition;
        layout (location = 1) in float aSize;
        
        uniform mat4 uViewProjection;
        uniform vec4 uColor;
        uniform float uTime;
        
        out vec4 vColor;
        
        void main() {
            gl_Position = uViewProjection * vec4(aPosition, 1.0);
            gl_PointSize = aSize;
            vColor = uColor;
        }
    )";

  const char *fragmentShaderSource = R"(
        #version 330 core
        in vec4 vColor;
        out vec4 FragColor;
        
        void main() {
            vec2 coord = gl_PointCoord - vec2(0.5);
            
            // Create cylindrical rain drop (vertical line shape)
            float verticalDist = abs(coord.y);
            float horizontalDist = abs(coord.x);
            
            // Cylindrical shape: narrow horizontally, elongated vertically
            float cylinder = horizontalDist * 8.0; // Narrow width
            float verticalFade = 1.0 - smoothstep(0.4, 0.5, verticalDist); // Fade at ends
            
            // Main drop body
            float alpha = 1.0 - smoothstep(0.0, 0.15, cylinder);
            alpha *= verticalFade;
            
            // Add motion blur effect (streaking)
            vec2 streakCoord = vec2(coord.x, coord.y * 0.3); // Compress vertically
            float streak = 1.0 - smoothstep(0.0, 0.2, length(streakCoord));
            alpha = max(alpha, streak * 0.5);
            
            // Realistic water color with transparency
            vec3 waterColor = mix(vColor.rgb, vec3(0.8, 0.9, 1.0), 0.3);
            
            // Add bright highlight for wet look
            float highlight = 1.0 - smoothstep(0.0, 0.05, horizontalDist);
            waterColor = mix(waterColor, vec3(1.0), highlight * 0.4);
            
            if (alpha < 0.01) discard;
            
            FragColor = vec4(waterColor, vColor.a * alpha * 0.7);
        }
    )";

  // Use the engine's Shader class
  m_RainShader = Shader::Create(vertexShaderSource, fragmentShaderSource);

  // Create VAO/VBO for instanced rendering with persistent buffer
  glGenVertexArrays(1, &m_VAO);
  glGenBuffers(1, &m_VBO);

  glBindVertexArray(m_VAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

  // Define vertex structure for proper interleaving
  struct ParticleVertex {
    Vec3 position;
    f32 size;
  };

  // Pre-allocate VBO with maximum particle count
  size_t bufferSize = m_Particles.maxParticles * sizeof(ParticleVertex);
  glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);

  // Set up vertex attributes for interleaved data
  // Position (location 0)
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                        (void *)0);

  // Size (location 1)
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex),
                        (void *)sizeof(Vec3));

  glBindVertexArray(0);

  GINI_INFO("Rain emitter initialized with {} max particles",
            m_Particles.maxParticles);
}

void RainEmitter::Update(f32 deltaTime) {
  if (!m_Active || !m_Settings.enabled)
    return;

  // Emit new rain particles
  m_EmissionAccumulator += m_Settings.emissionRate * deltaTime;
  u32 emitCount = (u32)m_EmissionAccumulator;
  m_EmissionAccumulator -= emitCount;

  if (emitCount > 0) {
    EmitRain(emitCount);
  }

  // Update existing particles
  UpdateParticles(deltaTime);
}

void RainEmitter::EmitRain(u32 count) {
  for (u32 i = 0;
       i < count && m_Particles.activeCount < m_Particles.maxParticles; i++) {
    u32 index = m_Particles.poolIndex;
    m_Particles.poolIndex =
        (m_Particles.poolIndex + 1) % m_Particles.maxParticles;

    // Activate particle
    if (!m_Particles.active[index]) {
      m_Particles.active[index] = true;
      m_Particles.activeCount++;

      // Set position with area variance
      m_Particles.positions[index] =
          m_Settings.position +
          Vec3((s_Distribution(s_RandomEngine) - 0.5f) * m_Settings.areaSize.x,
               0.0f,
               (s_Distribution(s_RandomEngine) - 0.5f) * m_Settings.areaSize.z);

      // Set velocity (mostly downward with wind)
      Vec3 wind = Vec3(m_Settings.windStrength +
                           (s_Distribution(s_RandomEngine) - 0.5f) *
                               m_Settings.windVariation,
                       -m_Settings.fallSpeed,
                       (s_Distribution(s_RandomEngine) - 0.5f) *
                           m_Settings.windVariation * 0.5f);
      m_Particles.velocities[index] = wind;

      // Set lifetime (based on height to ground)
      f32 distanceToGround =
          m_Particles.positions[index].y - m_Settings.groundLevel;
      m_Particles.lifeTimes[index] = distanceToGround / m_Settings.fallSpeed;
    }
  }
}

void RainEmitter::UpdateParticles(f32 deltaTime) {
  m_Particles.activeCount = 0;

  for (u32 i = 0; i < m_Particles.maxParticles; i++) {
    if (!m_Particles.active[i])
      continue;

    // Update position
    m_Particles.positions[i] += m_Particles.velocities[i] * deltaTime;

    // Update lifetime
    m_Particles.lifeTimes[i] -= deltaTime;

    // Kill if below ground or lifetime expired
    if (m_Particles.positions[i].y < m_Settings.groundLevel ||
        m_Particles.lifeTimes[i] <= 0.0f) {
      m_Particles.active[i] = false;
    } else {
      m_Particles.activeCount++;
    }
  }
}

void RainEmitter::Render(const Mat4 &viewProjection) {
  if (!m_Active || m_Particles.activeCount == 0) {
    return;
  }

  // Bind shader and set uniforms
  m_RainShader->Bind();
  m_RainShader->SetMat4("uViewProjection", viewProjection);
  m_RainShader->SetVec4("uColor", m_Settings.color);

  // Map VBO memory for efficient batch update
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

  // Use glMapBuffer for efficient data transfer
  struct ParticleVertex {
    Vec3 position;
    f32 size;
  };

  void *bufferData = glMapBufferRange(
      GL_ARRAY_BUFFER, 0, m_Particles.activeCount * sizeof(ParticleVertex),
      GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

  if (bufferData) {
    // Batch write all active particles in one go
    ParticleVertex *vertices = static_cast<ParticleVertex *>(bufferData);

    u32 writeIndex = 0;
    for (u32 i = 0; i < m_Particles.maxParticles; i++) {
      if (m_Particles.active[i]) {
        vertices[writeIndex].position = m_Particles.positions[i];
        vertices[writeIndex].size = m_Settings.size;
        writeIndex++;
      }
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
  }

  // Enable point sprites for rain rendering
  glEnable(GL_PROGRAM_POINT_SIZE);

  // Disable depth testing to render rain in front of everything
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Render all particles in one draw call
  glBindVertexArray(m_VAO);
  glDrawArrays(GL_POINTS, 0, m_Particles.activeCount);
  glBindVertexArray(0);

  // Restore state
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_PROGRAM_POINT_SIZE);
  m_RainShader->Unbind();
}

void RainEmitter::Clear() {
  std::fill(m_Particles.active.begin(), m_Particles.active.end(), false);
  m_Particles.activeCount = 0;
  m_Particles.poolIndex = 0;
}

// ThunderSystem Implementation
ThunderSystem::ThunderSystem() {
  m_Bolts.resize(8); // Reduced from 32 for performance
  InitRendering();
}

void ThunderSystem::InitRendering() {
  // Lightning shader
  const char *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPosition;
        
        uniform mat4 uViewProjection;
        
        void main() {
            gl_Position = uViewProjection * vec4(aPosition, 1.0);
        }
    )";

  const char *fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        uniform vec4 uColor;
        uniform float uIntensity;
        uniform float uTime;
        
        void main() {
            // Create neon-like lightning with pulsing effect
            float pulse = sin(uTime * 20.0) * 0.3 + 0.7;
            vec3 neonColor = uColor.rgb * 1.5; // Brighten for neon effect
            
            // Add white core for electric look
            vec3 coreColor = mix(neonColor, vec3(1.0), 0.6);
            
            FragColor = vec4(coreColor, uColor.a) * uIntensity * pulse;
        }
    )";

  // Use the engine's Shader class
  m_LightningShader = Shader::Create(vertexShaderSource, fragmentShaderSource);

  // Glow shader (simple blur)
  const char *glowVertexSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPosition;
        layout (location = 1) in vec2 aTexCoord;
        
        out vec2 vTexCoord;
        
        void main() {
            vTexCoord = aTexCoord;
            gl_Position = vec4(aPosition, 0.0, 1.0);
        }
    )";

  const char *glowFragmentSource = R"(
        #version 330 core
        in vec2 vTexCoord;
        out vec4 FragColor;
        
        uniform sampler2D uTexture;
        uniform vec4 uGlowColor;
        uniform float uGlowRadius;
        
        void main() {
            vec4 texColor = texture(uTexture, vTexCoord);
            
            // Simple blur
            vec4 blurred = vec4(0.0);
            float samples = 0.0;
            
            for(float x = -uGlowRadius; x <= uGlowRadius; x += 0.5) {
                for(float y = -uGlowRadius; y <= uGlowRadius; y += 0.5) {
                    vec2 offset = vec2(x, y) * 0.005;
                    blurred += texture(uTexture, vTexCoord + offset);
                    samples += 1.0;
                }
            }
            
            blurred /= samples;
            FragColor = texColor + blurred * uGlowColor;
        }
    )";

  // Use the engine's Shader class for glow
  m_GlowShader = Shader::Create(glowVertexSource, glowFragmentSource);

  // Create glow framebuffer
  FramebufferSpec glowSpec;
  glowSpec.width = 1024;
  glowSpec.height = 1024;
  glowSpec.samples = 1;
  m_GlowFramebuffer = Framebuffer::Create(glowSpec);

  // Create VAO/VBO for lightning rendering
  glGenVertexArrays(1, &m_VAO);
  glGenBuffers(1, &m_VBO);

  glBindVertexArray(m_VAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), (void *)0);
  glBindVertexArray(0);
}

void ThunderSystem::Update(f32 deltaTime) {
  if (!m_Active || !m_Settings.enabled)
    return;

  // Update existing bolts
  m_ActiveBoltCount = 0;

  // Decay environmental flash
  m_EnvironmentalFlash =
      std::max(0.0f, m_EnvironmentalFlash - deltaTime * 2.0f);
  for (auto &bolt : m_Bolts) {
    if (bolt.active) {
      bolt.lifeRemaining -= deltaTime;
      bolt.intensity = bolt.lifeRemaining / bolt.lifetime;

      if (bolt.lifeRemaining <= 0.0f) {
        bolt.active = false;
      } else {
        m_ActiveBoltCount++;
      }
    }
  }

  // Generate new lightning strikes
  m_TimeSinceLastStrike += deltaTime;
  if (m_TimeSinceLastStrike >= m_NextStrikeTime) {
    StrikeRandom();
    m_TimeSinceLastStrike = 0.0f;

    // Trigger environmental flash
    m_EnvironmentalFlash = 1.0f;

    // Schedule next strike
    m_NextStrikeTime =
        m_Settings.strikeFrequency +
        (s_Distribution(s_RandomEngine) - 0.5f) * m_Settings.strikeVariation;
    m_NextStrikeTime =
        std::max(0.5f, m_NextStrikeTime); // Minimum 0.5 seconds between strikes
  }
}

void ThunderSystem::StrikeLightning(const Vec3 &origin, const Vec3 &target) {
  // Find inactive bolt
  for (auto &bolt : m_Bolts) {
    if (!bolt.active) {
      bolt.active = true;
      bolt.origin = origin;
      bolt.target = target;
      bolt.lifeRemaining = bolt.lifetime;
      bolt.intensity = 1.0f;
      bolt.segments.clear();

      GenerateLightningBolt(origin, target, bolt);
      m_ActiveBoltCount++;
      break;
    }
  }
}

void ThunderSystem::GenerateLightningBolt(const Vec3 &origin,
                                          const Vec3 &target,
                                          LightningBolt &bolt) {
  // Main bolt
  SubdivideSegment(origin, target, 1.0f, bolt.segments, 0);

  // Add branches if enabled
  if (m_Settings.enableBranches) {
    u32 numBranches = 3 + s_Distribution(s_RandomEngine) * 3;

    for (u32 i = 0; i < numBranches; i++) {
      // Choose a random point along the main bolt
      if (bolt.segments.empty())
        break;

      u32 segmentIndex =
          (u32)(s_Distribution(s_RandomEngine) * bolt.segments.size());
      Vec3 branchOrigin = bolt.segments[segmentIndex].start;

      // Create branch endpoint
      Vec3 branchDirection = RandomVec3(-1.0f, 1.0f);
      f32 branchLength = 10.0f + s_Distribution(s_RandomEngine) * 20.0f;
      Vec3 branchTarget =
          branchOrigin + normalize(branchDirection) * branchLength;

      // Add branch segments with lower intensity
      SubdivideSegment(branchOrigin, branchTarget, 0.5f, bolt.segments, 0);
    }
  }
}

void ThunderSystem::SubdivideSegment(Vec3 start, Vec3 end, f32 intensity,
                                     std::vector<LightningSegment> &segments,
                                     u32 depth) {
  if (depth >= 4 || length(end - start) < 2.0f) {
    segments.push_back({start, end, 0.1f * intensity, intensity});
    return;
  }

  Vec3 mid = (start + end) * 0.5f;

  // Add random displacement
  Vec3 displacement = RandomVec3(-1.0f, 1.0f);
  displacement.y *= 0.3f; // Less vertical displacement
  mid += displacement * (length(end - start) * 0.2f);

  // Recursively subdivide
  SubdivideSegment(start, mid, intensity, segments, depth + 1);
  SubdivideSegment(mid, end, intensity, segments, depth + 1);
}

void ThunderSystem::StrikeRandom() {
  // Generate random lightning in the sky
  Vec3 origin = RandomVec3(-100.0f, 100.0f);
  origin.y = 100.0f + s_Distribution(s_RandomEngine) * 50.0f;

  Vec3 target = origin + RandomVec3(-50.0f, 50.0f);
  target.y = 0.0f + s_Distribution(s_RandomEngine) * 20.0f;

  StrikeLightning(origin, target);
}

void ThunderSystem::Render(const Mat4 &viewProjection) {
  if (m_ActiveBoltCount == 0)
    return;

  if (m_Settings.enableGlow) {
    // Render to glow framebuffer first
    m_GlowFramebuffer->Bind();
    Renderer3D::SetClearColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    Renderer3D::Clear();
  }

  // Render all active bolts
  for (const auto &bolt : m_Bolts) {
    if (bolt.active) {
      RenderBolt(bolt, viewProjection);
    }
  }

  if (m_Settings.enableGlow) {
    m_GlowFramebuffer->Unbind();

    // Apply glow effect
    ApplyGlowEffect();
  }
}

void ThunderSystem::RenderBolt(const LightningBolt &bolt,
                               const Mat4 &viewProjection) {
  if (bolt.segments.empty())
    return;

  // Collect all vertices
  std::vector<Vec3> vertices;
  for (const auto &segment : bolt.segments) {
    vertices.push_back(segment.start);
    vertices.push_back(segment.end);
  }

  if (vertices.empty())
    return;

  // Update VBO
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vec3), vertices.data(),
               GL_DYNAMIC_DRAW);

  // Bind shader and render
  m_LightningShader->Bind();
  m_LightningShader->SetMat4("uViewProjection", viewProjection);
  m_LightningShader->SetVec4("uColor", m_Settings.boltColor);
  m_LightningShader->SetFloat("uIntensity",
                              bolt.intensity * m_Settings.boltIntensity);
  m_LightningShader->SetFloat("uTime", (f32)glfwGetTime());

  // Enable blending for neon glow effect
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(3.0f); // Thicker lines for better visibility

  glBindVertexArray(m_VAO);
  glDrawArrays(GL_LINES, 0, vertices.size());
  glBindVertexArray(0);

  // Restore state
  glLineWidth(1.0f);
  glDisable(GL_BLEND);
  m_LightningShader->Unbind();
}

void ThunderSystem::ApplyGlowEffect() {
  // Simple glow post-processing
  // In a real implementation, you'd render this to a full-screen quad
  // For now, we'll skip the complex glow effect
}

Vec3 ThunderSystem::RandomVec3(f32 min, f32 max) {
  return Vec3(min + s_Distribution(s_RandomEngine) * (max - min),
              min + s_Distribution(s_RandomEngine) * (max - min),
              min + s_Distribution(s_RandomEngine) * (max - min));
}

// WeatherSystem Implementation
void WeatherSystem::Init() {
  if (s_Initialized)
    return;

  s_RainEmitter = CreateRef<RainEmitter>(10000);
  s_ThunderSystem = CreateRef<ThunderSystem>();

  s_Initialized = true;
  GINI_INFO("Weather system initialized");
}

void WeatherSystem::Shutdown() {
  s_RainEmitter.reset();
  s_ThunderSystem.reset();
  s_Initialized = false;
}

void WeatherSystem::Update(f32 deltaTime) {
  if (!s_Initialized)
    return;

  s_RainEmitter->Update(deltaTime);
  s_ThunderSystem->Update(deltaTime);
}

void WeatherSystem::Render(const Mat4 &viewProjection) {
  if (s_RainEmitter)
    s_RainEmitter->Render(viewProjection);

  if (s_ThunderSystem)
    s_ThunderSystem->Render(viewProjection);
}

void WeatherSystem::UpdateRainPosition(const Vec3 &cameraPosition) {
  if (s_RainEmitter) {
    RainSettings &settings = s_RainEmitter->GetSettings();
    // Position rain slightly in front of and above camera for immersion
    settings.position = cameraPosition + Vec3(0.0f, 10.0f, 5.0f);
  }
}

void WeatherSystem::SetWeatherIntensity(f32 intensity) {
  m_WeatherIntensity = std::clamp(intensity, 0.0f, 1.0f);

  if (s_RainEmitter) {
    RainSettings &rain = s_RainEmitter->GetSettings();
    rain.emissionRate = 1000.0f + m_WeatherIntensity * 4000.0f;
    rain.enabled = m_WeatherIntensity > 0.1f;

    // Also activate/deactivate the emitter itself
    bool shouldActivate = m_WeatherIntensity > 0.1f;
    s_RainEmitter->SetActive(shouldActivate);
  }

  if (s_ThunderSystem) {
    ThunderSettings &thunder = s_ThunderSystem->GetSettings();
    thunder.strikeFrequency = 10.0f - m_WeatherIntensity * 8.0f;
    thunder.enabled = m_WeatherIntensity > 0.5f;

    // Also activate/deactivate the thunder system
    s_ThunderSystem->SetActive(m_WeatherIntensity > 0.5f);
  }
}

void WeatherSystem::StartRain() {
  if (s_RainEmitter) {
    s_RainEmitter->SetActive(true);
    s_RainEmitter->GetSettings().enabled = true;
  }
}

void WeatherSystem::StopRain() {
  if (s_RainEmitter) {
    s_RainEmitter->SetActive(false);
    s_RainEmitter->GetSettings().enabled = false;
  }
}

void WeatherSystem::StartThunderstorm() {
  StartRain();
  if (s_ThunderSystem) {
    s_ThunderSystem->SetActive(true);
    s_ThunderSystem->GetSettings().enabled = true;
  }
}

void WeatherSystem::StopThunderstorm() {
  StopRain();
  if (s_ThunderSystem) {
    s_ThunderSystem->SetActive(false);
    s_ThunderSystem->GetSettings().enabled = false;
  }
}

} // namespace Gini
