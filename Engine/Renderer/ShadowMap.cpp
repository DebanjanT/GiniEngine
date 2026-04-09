#include "ShadowMap.h"
#include "Core/Logger.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Gini {

struct ShadowMapData {
  u32 depthFBO = 0;
  u32 depthTextureArray = 0;
  Ref<Shader> depthShader;
  std::vector<Mat4> lightSpaceMatrices;
  std::vector<f32> cascadeSplits;
  bool initialized = false;
};

static ShadowMapData *s_Shadow = nullptr;

// Diligent Engine resources
bool ShadowMap::s_HasDiligentResources = false;

static const char *s_DepthVertexShader = R"(
#version 410 core
layout (location = 0) in vec3 a_Position;

uniform mat4 u_Model;
uniform mat4 u_LightSpaceMatrix;

void main() {
    gl_Position = u_LightSpaceMatrix * u_Model * vec4(a_Position, 1.0);
}
)";

static const char *s_DepthFragmentShader = R"(
#version 410 core
void main() {
}
)";

void ShadowMap::Init() {
  s_Shadow = new ShadowMapData();

  s_Shadow->depthShader =
      Shader::Create(s_DepthVertexShader, s_DepthFragmentShader);
  s_Shadow->lightSpaceMatrices.resize(NUM_CASCADES, Mat4(1.0f));
  s_Shadow->cascadeSplits.resize(NUM_CASCADES, 0.0f);

  glGenFramebuffers(1, &s_Shadow->depthFBO);

  glGenTextures(1, &s_Shadow->depthTextureArray);
  glBindTexture(GL_TEXTURE_2D_ARRAY, s_Shadow->depthTextureArray);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, SHADOW_MAP_SIZE,
               SHADOW_MAP_SIZE, NUM_CASCADES, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
               nullptr);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

  glBindFramebuffer(GL_FRAMEBUFFER, s_Shadow->depthFBO);
  glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            s_Shadow->depthTextureArray, 0, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  s_Shadow->initialized = true;
  GINI_INFO("Shadow mapping initialized ({}x{}, {} cascades)", SHADOW_MAP_SIZE,
            SHADOW_MAP_SIZE, NUM_CASCADES);
}

void ShadowMap::Shutdown() {
  if (s_Shadow) {
    if (s_Shadow->depthFBO)
      glDeleteFramebuffers(1, &s_Shadow->depthFBO);
    if (s_Shadow->depthTextureArray)
      glDeleteTextures(1, &s_Shadow->depthTextureArray);
    delete s_Shadow;
    s_Shadow = nullptr;
  }
}

static void ComputeCascadeSplits(f32 nearPlane, f32 farPlane,
                                 std::vector<f32> &splits) {
  f32 lambda = 0.75f;
  f32 ratio = farPlane / nearPlane;
  u32 numCascades = static_cast<u32>(splits.size());

  for (u32 i = 0; i < numCascades; i++) {
    f32 p = static_cast<f32>(i + 1) / static_cast<f32>(numCascades);
    f32 logSplit = nearPlane * std::pow(ratio, p);
    f32 uniformSplit = nearPlane + (farPlane - nearPlane) * p;
    splits[i] = lambda * logSplit + (1.0f - lambda) * uniformSplit;
  }
}

static Mat4 ComputeLightSpaceMatrix(const Camera3D &camera,
                                    const DirectionalLight &light,
                                    f32 nearSplit, f32 farSplit) {
  Mat4 proj = glm::perspective(glm::radians(camera.GetFOV()),
                               camera.GetAspectRatio(), nearSplit, farSplit);
  Mat4 view = camera.GetViewMatrix();
  Mat4 invViewProj = glm::inverse(proj * view);

  std::vector<Vec4> frustumCorners;
  for (u32 x = 0; x < 2; x++) {
    for (u32 y = 0; y < 2; y++) {
      for (u32 z = 0; z < 2; z++) {
        Vec4 pt = invViewProj *
                  Vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
        frustumCorners.push_back(pt / pt.w);
      }
    }
  }

  Vec3 center(0.0f);
  for (const auto &c : frustumCorners)
    center += Vec3(c);
  center /= static_cast<f32>(frustumCorners.size());

  Vec3 lightDir = glm::normalize(light.direction);
  Mat4 lightView =
      glm::lookAt(center - lightDir, center, Vec3(0.0f, 1.0f, 0.0f));

  f32 minX = std::numeric_limits<f32>::max(),
      maxX = std::numeric_limits<f32>::lowest();
  f32 minY = std::numeric_limits<f32>::max(),
      maxY = std::numeric_limits<f32>::lowest();
  f32 minZ = std::numeric_limits<f32>::max(),
      maxZ = std::numeric_limits<f32>::lowest();

  for (const auto &c : frustumCorners) {
    Vec4 transformed = lightView * c;
    minX = std::min(minX, transformed.x);
    maxX = std::max(maxX, transformed.x);
    minY = std::min(minY, transformed.y);
    maxY = std::max(maxY, transformed.y);
    minZ = std::min(minZ, transformed.z);
    maxZ = std::max(maxZ, transformed.z);
  }

  f32 zMult = 10.0f;
  minZ = (minZ < 0) ? minZ * zMult : minZ / zMult;
  maxZ = (maxZ < 0) ? maxZ / zMult : maxZ * zMult;

  Mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
  return lightProj * lightView;
}

void ShadowMap::BeginShadowPass(const Camera3D &camera,
                                const DirectionalLight &light) {
  if (!s_Shadow || !s_Shadow->initialized)
    return;

  f32 nearPlane = 0.1f;
  f32 farPlane = 200.0f;
  ComputeCascadeSplits(nearPlane, farPlane, s_Shadow->cascadeSplits);

  f32 prevSplit = nearPlane;
  for (u32 i = 0; i < NUM_CASCADES; i++) {
    s_Shadow->lightSpaceMatrices[i] = ComputeLightSpaceMatrix(
        camera, light, prevSplit, s_Shadow->cascadeSplits[i]);
    prevSplit = s_Shadow->cascadeSplits[i];
  }

  glBindFramebuffer(GL_FRAMEBUFFER, s_Shadow->depthFBO);
  glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
  glEnable(GL_DEPTH_TEST);
  glCullFace(GL_FRONT);
}

void ShadowMap::EndShadowPass() {
  if (!s_Shadow)
    return;
  glCullFace(GL_BACK);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::BindShadowMaps(Shader *shader, u32 startTextureUnit) {
  if (!s_Shadow || !s_Shadow->initialized || !shader) {
    GINI_WARN("ShadowMap::BindShadowMaps: Shadow system not initialized or "
              "shader is null");
    return;
  }

  GINI_DEBUG("ShadowMap::BindShadowMaps: Binding shadow maps starting at unit ",
             startTextureUnit);

  glActiveTexture(GL_TEXTURE0 + startTextureUnit);
  glBindTexture(GL_TEXTURE_2D_ARRAY, s_Shadow->depthTextureArray);
  shader->SetInt("u_ShadowMap", static_cast<i32>(startTextureUnit));

  for (u32 i = 0; i < NUM_CASCADES; i++) {
    std::string name = "u_LightSpaceMatrices[" + std::to_string(i) + "]";
    shader->SetMat4(name, s_Shadow->lightSpaceMatrices[i]);
  }

  shader->SetInt("u_CascadeCount", static_cast<i32>(NUM_CASCADES));

  for (u32 i = 0; i < NUM_CASCADES; i++) {
    std::string name = "u_CascadeSplits[" + std::to_string(i) + "]";
    shader->SetFloat(name, s_Shadow->cascadeSplits[i]);
  }

  shader->SetInt("u_HasShadows", 1);
  GINI_DEBUG("ShadowMap::BindShadowMaps: Shadow maps bound successfully, "
             "u_HasShadows set to 1");
}

u32 ShadowMap::GetShadowMapTexture() {
  return s_Shadow ? s_Shadow->depthTextureArray : 0;
}

const std::vector<Mat4> &ShadowMap::GetLightSpaceMatrices() {
  static std::vector<Mat4> empty;
  return s_Shadow ? s_Shadow->lightSpaceMatrices : empty;
}

const std::vector<f32> &ShadowMap::GetCascadeSplits() {
  static std::vector<f32> empty;
  return s_Shadow ? s_Shadow->cascadeSplits : empty;
}

bool ShadowMap::IsInitialized() { return s_Shadow && s_Shadow->initialized; }

Ref<Shader> ShadowMap::GetDepthShader() {
  return s_Shadow ? s_Shadow->depthShader : nullptr;
}

// Diligent Engine integration implementations
void ShadowMap::CreateDiligentShadowMap() {
  // Diligent Engine not available, this is a no-op
  // OpenGL shadow mapping is already functional
  s_HasDiligentResources = false;
}

void ShadowMap::UpdateDiligentShadowMap() {
  // Diligent Engine not available, this is a no-op
  // OpenGL shadow mapping updates are handled by the existing system
}

void ShadowMap::RegisterWithHybridManager() {
  // Diligent Engine not available, this is a no-op
  // OpenGL shadow mapping is already functional
}

} // namespace Gini
