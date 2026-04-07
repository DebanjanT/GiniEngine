#include "AtmosphericSky.h"
#include "Core/Logger.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Gini {

static const char *s_SkyVertexShader = R"(
#version 410 core
layout (location = 0) in vec3 a_Position;

out vec3 v_WorldDir;

uniform mat4 u_InvViewProj;

void main() {
    vec4 clipPos = vec4(a_Position.xy, 1.0, 1.0);
    vec4 worldPos = u_InvViewProj * clipPos;
    v_WorldDir = worldPos.xyz / worldPos.w;
    gl_Position = vec4(a_Position.xy, 0.9999, 1.0);
}
)";

static const char *s_SkyFragmentShader = R"(
#version 410 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec3 gNormal;

in vec3 v_WorldDir;

uniform vec3 u_CameraPos;
uniform vec3 u_SunDir;
uniform vec3 u_SunColor;
uniform float u_SunIntensity;
uniform float u_SunDiskSize;

uniform vec3 u_RayleighCoeff;
uniform float u_RayleighScale;
uniform vec3 u_MieCoeff;
uniform float u_MieScale;
uniform float u_MieG;

uniform float u_PlanetRadius;
uniform float u_AtmosphereRadius;

uniform vec3 u_GroundColor;
uniform float u_GroundBrightness;

// Cloud settings
uniform float u_Time;
uniform float u_CloudCoverage;
uniform float u_CloudDensity;
uniform float u_CloudHeight;
uniform float u_CloudThickness;
uniform float u_CloudQuality;
uniform bool u_CloudsEnabled;

const float PI = 3.14159265359;
const int NUM_SAMPLES = 16;
const int NUM_LIGHT_SAMPLES = 8;
const int CLOUD_SAMPLES = 16;  // Reduced from 32
const int CLOUD_LIGHT_SAMPLES = 4;  // Reduced from 6

// Hash functions for noise
float hash(float n) { return fract(sin(n) * 43758.5453123); }
float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123); }
float hash(vec3 p) { return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453123); }

// 3D Value noise
float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    return mix(
        mix(mix(hash(i + vec3(0,0,0)), hash(i + vec3(1,0,0)), f.x),
            mix(hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)), f.x), f.y),
        mix(mix(hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)), f.x),
            mix(hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)), f.x), f.y), f.z);
}

// FBM for cloud shapes
float fbm(vec3 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise3D(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return value;
}

// Optimized cloud density function with LOD
float cloudDensity(vec3 pos, float lod) {
    // Normalize position for cloud layer
    float heightFraction = (pos.y - u_CloudHeight) / u_CloudThickness;
    if (heightFraction < 0.0 || heightFraction > 1.0) return 0.0;
    
    // Height-based density falloff (rounder at bottom, wispy at top)
    float heightGradient = smoothstep(0.0, 0.2, heightFraction) * smoothstep(1.0, 0.7, heightFraction);
    
    // Sample noise at different scales with LOD
    vec3 windOffset = vec3(u_Time * 0.01, 0.0, u_Time * 0.005);
    vec3 samplePos = pos * 0.0001 + windOffset;
    
    // LOD-based noise sampling
    float lodFactor = clamp(lod * 2.0, 0.5, 4.0);
    float baseShape = fbm(samplePos * lodFactor, 3);
    
    // Only sample detail at close range
    float detail = 0.0;
    if (lod < 0.5) {
        detail = fbm(samplePos * 4.0 * lodFactor + vec3(100.0), 2) * 0.3;
    }
    
    float density = baseShape + detail;
    
    // Apply coverage threshold
    density = smoothstep(1.0 - u_CloudCoverage, 1.0, density);
    density *= heightGradient;
    density *= u_CloudDensity;
    
    return max(0.0, density);
}

// Ray-sphere intersection
vec2 RaySphereIntersect(vec3 rayOrigin, vec3 rayDir, float radius) {
    float b = dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - radius * radius;
    float d = b * b - c;
    if (d < 0.0) return vec2(-1.0);
    d = sqrt(d);
    return vec2(-b - d, -b + d);
}

// Ray-plane intersection for cloud layer
vec2 RayCloudLayerIntersect(vec3 rayOrigin, vec3 rayDir, float cloudBottom, float cloudTop) {
    float tBottom = (cloudBottom - rayOrigin.y) / rayDir.y;
    float tTop = (cloudTop - rayOrigin.y) / rayDir.y;
    
    if (tBottom > tTop) {
        float temp = tBottom;
        tBottom = tTop;
        tTop = temp;
    }
    
    return vec2(max(0.0, tBottom), max(0.0, tTop));
}

// Phase functions
float RayleighPhase(float cosTheta) {
    return 3.0 / (16.0 * PI) * (1.0 + cosTheta * cosTheta);
}

float MiePhase(float cosTheta, float g) {
    float g2 = g * g;
    float num = 3.0 * (1.0 - g2) * (1.0 + cosTheta * cosTheta);
    float denom = (8.0 * PI) * (2.0 + g2) * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
    return num / denom;
}

// Henyey-Greenstein phase function for clouds
float HenyeyGreenstein(float cosTheta, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

// Density at height
float GetDensity(float height, float scaleHeight) {
    return exp(-height / scaleHeight);
}

// Optimized cloud marching with LOD and early exits
vec4 marchClouds(vec3 rayOrigin, vec3 rayDir, vec3 sunDir, float maxDist, float distance) {
    float cloudBottom = u_CloudHeight;
    float cloudTop = u_CloudHeight + u_CloudThickness;
    
    vec2 cloudHit = RayCloudLayerIntersect(rayOrigin, rayDir, cloudBottom, cloudTop);
    if (cloudHit.y <= cloudHit.x) return vec4(0.0);
    
    float startDist = cloudHit.x;
    float endDist = min(cloudHit.y, maxDist);
    if (startDist >= endDist) return vec4(0.0);
    
    // LOD based on distance and quality setting
    float lod = distance / 10000.0; // LOD based on distance
    lod = mix(lod, lod * 2.0, 1.0 - u_CloudQuality); // Quality affects LOD
    
    // Adaptive sample count based on LOD and quality
    int maxSamples = int(mix(float(CLOUD_SAMPLES), 8.0, u_CloudQuality));
    int minSamples = int(mix(4.0, 2.0, u_CloudQuality));
    int samples = int(mix(float(maxSamples), float(minSamples), clamp(lod, 0.0, 1.0)));
    float stepSize = (endDist - startDist) / float(samples);
    
    vec3 lightColor = u_SunColor * u_SunIntensity;
    float transmittance = 1.0;
    vec3 scatteredLight = vec3(0.0);
    
    // Phase function for forward scattering
    float cosTheta = dot(rayDir, sunDir);
    float phase = mix(HenyeyGreenstein(cosTheta, 0.6), HenyeyGreenstein(cosTheta, -0.3), 0.3);
    
    // Pre-calculate ambient
    vec3 ambient = vec3(0.4, 0.5, 0.7) * 0.3;
    
    for (int i = 0; i < samples; i++) {
        float t = startDist + (float(i) + 0.5) * stepSize;
        vec3 pos = rayOrigin + rayDir * t;
        
        float density = cloudDensity(pos, lod);
        if (density > 0.01) { // Increased threshold for early exit
            // Simplified light calculation for distant clouds
            float lightTransmittance = 1.0;
            float lodThreshold = mix(0.5, 0.3, u_CloudQuality); // Quality affects threshold
            if (lod < lodThreshold) {
                // Full light marching only for close clouds
                int lightSamples = int(mix(float(CLOUD_LIGHT_SAMPLES), 2.0, u_CloudQuality));
                float lightStepSize = u_CloudThickness / float(lightSamples);
                for (int j = 0; j < lightSamples; j++) {
                    vec3 lightPos = pos + sunDir * float(j) * lightStepSize;
                    float lightDensity = cloudDensity(lightPos, lod);
                    lightTransmittance *= exp(-lightDensity * lightStepSize * 0.5);
                }
            } else {
                // Approximate light for distant clouds
                lightTransmittance = mix(0.7, 0.8, u_CloudQuality); // Quality affects approximation
            }
            
            // Combine direct and ambient lighting
            vec3 lighting = lightColor * lightTransmittance * phase + ambient;
            
            // Beer-Lambert absorption
            float extinction = density * stepSize * 0.5;
            float sampleTransmittance = exp(-extinction);
            
            // Energy-conserving scattering
            vec3 integScatter = (lighting - lighting * sampleTransmittance) / max(extinction, 0.0001);
            scatteredLight += transmittance * integScatter;
            transmittance *= sampleTransmittance;
            
            // Early exit with higher threshold
            if (transmittance < 0.05) break;
        }
    }
    
    return vec4(scatteredLight, 1.0 - transmittance);
}

void main() {
    vec3 rayDir = normalize(v_WorldDir - u_CameraPos);
    vec3 sunDir = normalize(-u_SunDir);
    
    // Camera position relative to planet center (at surface level)
    vec3 rayOrigin = vec3(0.0, u_PlanetRadius + 1.0, 0.0);
    
    // Check if ray hits atmosphere
    vec2 atmosphereHit = RaySphereIntersect(rayOrigin, rayDir, u_AtmosphereRadius);
    if (atmosphereHit.y < 0.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        gNormal = vec3(0.0);
        return;
    }
    
    // Check if ray hits ground
    vec2 groundHit = RaySphereIntersect(rayOrigin, rayDir, u_PlanetRadius);
    bool hitsGround = groundHit.x > 0.0;
    
    float rayLength = hitsGround ? groundHit.x : atmosphereHit.y;
    float stepSize = rayLength / float(NUM_SAMPLES);
    
    vec3 rayleighSum = vec3(0.0);
    vec3 mieSum = vec3(0.0);
    float opticalDepthR = 0.0;
    float opticalDepthM = 0.0;
    
    // March along view ray
    for (int i = 0; i < NUM_SAMPLES; i++) {
        vec3 samplePos = rayOrigin + rayDir * (float(i) + 0.5) * stepSize;
        float height = length(samplePos) - u_PlanetRadius;
        
        // Density at this point
        float densityR = GetDensity(height, u_RayleighScale) * stepSize;
        float densityM = GetDensity(height, u_MieScale) * stepSize;
        
        opticalDepthR += densityR;
        opticalDepthM += densityM;
        
        // Light ray to sun
        vec2 sunHit = RaySphereIntersect(samplePos, sunDir, u_AtmosphereRadius);
        float sunRayLength = sunHit.y;
        float sunStepSize = sunRayLength / float(NUM_LIGHT_SAMPLES);
        
        float sunOpticalDepthR = 0.0;
        float sunOpticalDepthM = 0.0;
        
        bool inShadow = false;
        for (int j = 0; j < NUM_LIGHT_SAMPLES; j++) {
            vec3 sunSamplePos = samplePos + sunDir * (float(j) + 0.5) * sunStepSize;
            float sunHeight = length(sunSamplePos) - u_PlanetRadius;
            
            if (sunHeight < 0.0) {
                inShadow = true;
                break;
            }
            
            sunOpticalDepthR += GetDensity(sunHeight, u_RayleighScale) * sunStepSize;
            sunOpticalDepthM += GetDensity(sunHeight, u_MieScale) * sunStepSize;
        }
        
        if (!inShadow) {
            vec3 tau = u_RayleighCoeff * (opticalDepthR + sunOpticalDepthR) +
                       u_MieCoeff * 1.1 * (opticalDepthM + sunOpticalDepthM);
            vec3 attenuation = exp(-tau);
            
            rayleighSum += densityR * attenuation;
            mieSum += densityM * attenuation;
        }
    }
    
    // Phase functions
    float cosTheta = dot(rayDir, sunDir);
    float rayleighPhase = RayleighPhase(cosTheta);
    float miePhase = MiePhase(cosTheta, u_MieG);
    
    // Final color
    vec3 rayleigh = rayleighSum * u_RayleighCoeff * rayleighPhase;
    vec3 mie = mieSum * u_MieCoeff * miePhase;
    
    vec3 skyColor = (rayleigh + mie) * u_SunColor * u_SunIntensity;
    
    // Add sun disk
    float sunAngle = acos(cosTheta);
    float sunDisk = smoothstep(u_SunDiskSize * 1.1, u_SunDiskSize * 0.9, sunAngle);
    skyColor += sunDisk * u_SunColor * u_SunIntensity * 10.0;
    
    // Ground color if looking down
    if (hitsGround) {
        float NdotL = max(dot(vec3(0.0, 1.0, 0.0), sunDir), 0.0);
        vec3 groundLit = u_GroundColor * u_GroundBrightness * (0.3 + 0.7 * NdotL);
        
        // Apply atmospheric extinction
        vec3 tau = u_RayleighCoeff * opticalDepthR + u_MieCoeff * 1.1 * opticalDepthM;
        vec3 extinction = exp(-tau);
        
        skyColor = groundLit * extinction + skyColor;
    }
    
    // Volumetric clouds with optimizations
    if (u_CloudsEnabled && rayDir.y > -0.1) {
        // Calculate distance for LOD
        float distance = length(rayDir);
        
        // Skip clouds for very distant rays (performance optimization)
        if (distance < 50000.0) {
            // Use world-space ray for clouds (scaled down for cloud layer)
            vec3 cloudRayOrigin = vec3(0.0, 0.0, 0.0);
            vec4 clouds = marchClouds(cloudRayOrigin, rayDir, sunDir, 100000.0, distance);
            
            // Blend clouds with sky
            skyColor = mix(skyColor, clouds.rgb, clouds.a);
        }
    }
    
    FragColor = vec4(skyColor, 1.0);
    gNormal = vec3(0.0);
}
)";

AtmosphericSky::AtmosphericSky() {}

AtmosphericSky::~AtmosphericSky() {
  if (m_SkyVAO)
    glDeleteVertexArrays(1, &m_SkyVAO);
  if (m_SkyVBO)
    glDeleteBuffers(1, &m_SkyVBO);
}

void AtmosphericSky::Initialize() {
  if (m_Initialized)
    return;

  InitShaders();
  CreateSkyMesh();

  m_Initialized = true;
  GINI_INFO("Atmospheric sky system initialized");
}

void AtmosphericSky::InitShaders() {
  m_SkyShader = Shader::Create(s_SkyVertexShader, s_SkyFragmentShader);
}

void AtmosphericSky::CreateSkyMesh() {
  // Full-screen quad
  float vertices[] = {-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 0.0f,
                      -1.0f, -1.0f, 0.0f, 1.0f, 1.0f,  0.0f, -1.0f, 1.0f, 0.0f};

  glGenVertexArrays(1, &m_SkyVAO);
  glGenBuffers(1, &m_SkyVBO);

  glBindVertexArray(m_SkyVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_SkyVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

  glBindVertexArray(0);
}

void AtmosphericSky::Render(const Camera3D &camera) {
  if (!m_Initialized) {
    Initialize();
  }

  // Disable depth write but keep depth test
  glDepthMask(GL_FALSE);
  glDepthFunc(GL_LEQUAL);

  m_SkyShader->Bind();

  // Calculate inverse view-projection matrix
  Mat4 view = camera.GetViewMatrix();
  Mat4 proj = camera.GetProjectionMatrix();

  // Remove translation from view matrix for sky
  Mat4 viewNoTranslation = Mat4(Mat3(view));
  Mat4 invViewProj = glm::inverse(proj * viewNoTranslation);

  m_SkyShader->SetMat4("u_InvViewProj", invViewProj);
  m_SkyShader->SetVec3("u_CameraPos", Vec3(0.0f));

  // Sun settings
  m_SkyShader->SetVec3("u_SunDir", m_Sun.direction);
  m_SkyShader->SetVec3("u_SunColor", m_Sun.color);
  m_SkyShader->SetFloat("u_SunIntensity", m_Sun.intensity);
  m_SkyShader->SetFloat("u_SunDiskSize", m_Sun.diskSize);

  // Atmosphere settings
  m_SkyShader->SetVec3("u_RayleighCoeff", m_Atmosphere.rayleighCoeff);
  m_SkyShader->SetFloat("u_RayleighScale", m_Atmosphere.rayleighScale);
  m_SkyShader->SetVec3("u_MieCoeff", m_Atmosphere.mieCoeff);
  m_SkyShader->SetFloat("u_MieScale", m_Atmosphere.mieScale);
  m_SkyShader->SetFloat("u_MieG", m_Atmosphere.mieG);
  m_SkyShader->SetFloat("u_PlanetRadius", m_Atmosphere.planetRadius);
  m_SkyShader->SetFloat("u_AtmosphereRadius", m_Atmosphere.atmosphereRadius);
  m_SkyShader->SetVec3("u_GroundColor", m_Atmosphere.groundColor);
  m_SkyShader->SetFloat("u_GroundBrightness", m_Atmosphere.groundBrightness);

  // Cloud settings
  m_SkyShader->SetInt("u_CloudsEnabled", m_Clouds.enabled ? 1 : 0);
  m_SkyShader->SetFloat("u_Time", m_Time);
  m_SkyShader->SetFloat("u_CloudCoverage", m_Clouds.coverage);
  m_SkyShader->SetFloat("u_CloudDensity", m_Clouds.density);
  m_SkyShader->SetFloat("u_CloudHeight", m_Clouds.height);
  m_SkyShader->SetFloat("u_CloudThickness", m_Clouds.thickness);
  m_SkyShader->SetFloat("u_CloudQuality", m_Clouds.quality);

  glBindVertexArray(m_SkyVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);

  // Restore depth state
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
}

void AtmosphericSky::Render(const Mat4 &viewMatrix, const Mat4 &projectionMatrix) {
  if (!m_Initialized) {
    Initialize();
  }

  glDepthMask(GL_FALSE);
  glDepthFunc(GL_LEQUAL);

  m_SkyShader->Bind();

  Mat4 viewNoTranslation = Mat4(Mat3(viewMatrix));
  Mat4 invViewProj = glm::inverse(projectionMatrix * viewNoTranslation);

  m_SkyShader->SetMat4("u_InvViewProj", invViewProj);
  m_SkyShader->SetVec3("u_CameraPos", Vec3(0.0f));

  m_SkyShader->SetVec3("u_SunDir", m_Sun.direction);
  m_SkyShader->SetVec3("u_SunColor", m_Sun.color);
  m_SkyShader->SetFloat("u_SunIntensity", m_Sun.intensity);
  m_SkyShader->SetFloat("u_SunDiskSize", m_Sun.diskSize);

  m_SkyShader->SetVec3("u_RayleighCoeff", m_Atmosphere.rayleighCoeff);
  m_SkyShader->SetFloat("u_RayleighScale", m_Atmosphere.rayleighScale);
  m_SkyShader->SetVec3("u_MieCoeff", m_Atmosphere.mieCoeff);
  m_SkyShader->SetFloat("u_MieScale", m_Atmosphere.mieScale);
  m_SkyShader->SetFloat("u_MieG", m_Atmosphere.mieG);
  m_SkyShader->SetFloat("u_PlanetRadius", m_Atmosphere.planetRadius);
  m_SkyShader->SetFloat("u_AtmosphereRadius", m_Atmosphere.atmosphereRadius);
  m_SkyShader->SetVec3("u_GroundColor", m_Atmosphere.groundColor);
  m_SkyShader->SetFloat("u_GroundBrightness", m_Atmosphere.groundBrightness);

  m_SkyShader->SetInt("u_CloudsEnabled", 0);
  m_SkyShader->SetFloat("u_Time", m_Time);
  m_SkyShader->SetFloat("u_CloudCoverage", m_Clouds.coverage);
  m_SkyShader->SetFloat("u_CloudDensity", m_Clouds.density);
  m_SkyShader->SetFloat("u_CloudHeight", m_Clouds.height);
  m_SkyShader->SetFloat("u_CloudThickness", m_Clouds.thickness);
  m_SkyShader->SetFloat("u_CloudQuality", m_Clouds.quality);

  glBindVertexArray(m_SkyVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);

  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
}

void AtmosphericSky::SetSunFromTimeOfDay(f32 timeHours) {
  // Convert time to sun angle
  // 6:00 = sunrise (east), 12:00 = noon (top), 18:00 = sunset (west)
  f32 angle = (timeHours - 6.0f) / 12.0f * 3.14159f;

  m_Sun.direction = Vec3(-cos(angle), -sin(angle), 0.2f);
  m_Sun.direction = glm::normalize(m_Sun.direction);

  // Adjust sun color based on time
  f32 sunHeight = -m_Sun.direction.y;

  if (sunHeight < 0.0f) {
    // Night
    m_Sun.intensity = 0.0f;
    m_Sun.color = Vec3(0.2f, 0.2f, 0.4f);
  } else if (sunHeight < 0.2f) {
    // Sunrise/sunset
    f32 t = sunHeight / 0.2f;
    m_Sun.intensity = t * 2.0f;
    m_Sun.color = glm::mix(Vec3(1.0f, 0.4f, 0.1f), Vec3(1.0f, 0.95f, 0.9f), t);
  } else {
    // Day
    m_Sun.intensity = 3.0f;
    m_Sun.color = Vec3(1.0f, 0.98f, 0.95f);
  }
}

Vec3 AtmosphericSky::GetAmbientColor() const {
  // Approximate ambient from sky
  f32 sunHeight = -m_Sun.direction.y;

  if (sunHeight < 0.0f) {
    return Vec3(0.02f, 0.02f, 0.05f); // Night
  } else if (sunHeight < 0.2f) {
    f32 t = sunHeight / 0.2f;
    return glm::mix(Vec3(0.1f, 0.05f, 0.02f), Vec3(0.15f, 0.2f, 0.3f), t);
  } else {
    return Vec3(0.15f, 0.2f, 0.3f); // Day
  }
}

Vec3 AtmosphericSky::GetHorizonColor() const {
  f32 sunHeight = -m_Sun.direction.y;

  if (sunHeight < 0.0f) {
    return Vec3(0.05f, 0.05f, 0.1f);
  } else if (sunHeight < 0.2f) {
    return Vec3(0.8f, 0.4f, 0.2f);
  } else {
    return Vec3(0.6f, 0.7f, 0.9f);
  }
}

Vec3 AtmosphericSky::GetZenithColor() const {
  f32 sunHeight = -m_Sun.direction.y;

  if (sunHeight < 0.0f) {
    return Vec3(0.01f, 0.01f, 0.03f);
  } else if (sunHeight < 0.2f) {
    f32 t = sunHeight / 0.2f;
    return glm::mix(Vec3(0.2f, 0.1f, 0.3f), Vec3(0.2f, 0.4f, 0.8f), t);
  } else {
    return Vec3(0.2f, 0.4f, 0.8f);
  }
}

Ref<AtmosphericSky> AtmosphericSky::Create() {
  return CreateRef<AtmosphericSky>();
}

} // namespace Gini
