#include "PostProcess.h"
#include "Core/Logger.h"
#include <glad/gl.h>

namespace Gini {

struct PostProcessData {
  Ref<Shader> tonemapShader;
  u32 quadVAO = 0;
  u32 quadVBO = 0;
};

static PostProcessData *s_PPData = nullptr;

// Diligent Engine resources
bool PostProcess::s_HasDiligentResources = false;

static const char *s_PPVertexShader = R"(
#version 410 core
layout (location = 0) in vec2 a_Position;
layout (location = 1) in vec2 a_TexCoords;

out vec2 v_TexCoords;

void main() {
    v_TexCoords = a_TexCoords;
    gl_Position = vec4(a_Position, 0.0, 1.0);
}
)";

static const char *s_PPFragmentShader = R"(
#version 410 core
out vec4 FragColor;

in vec2 v_TexCoords;

uniform sampler2D u_HDRBuffer;
uniform sampler2D u_SSAOBuffer;
uniform float u_Exposure;
uniform float u_Gamma;
uniform int u_HasSSAO;

vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdrColor = texture(u_HDRBuffer, v_TexCoords).rgb;

    float ao = 1.0;
    if (u_HasSSAO == 1) {
        ao = texture(u_SSAOBuffer, v_TexCoords).r;
    }
    hdrColor *= ao;

    vec3 mapped = ACESFilm(hdrColor * u_Exposure);
    mapped = pow(mapped, vec3(1.0 / u_Gamma));

    FragColor = vec4(mapped, 1.0);
}
)";

void PostProcess::Init() {
  s_PPData = new PostProcessData();
  s_PPData->tonemapShader =
      Shader::Create(s_PPVertexShader, s_PPFragmentShader);
  InitQuad();
  GINI_INFO("PostProcess initialized (ACES tonemapping)");
}

void PostProcess::Shutdown() {
  if (s_PPData) {
    if (s_PPData->quadVAO)
      glDeleteVertexArrays(1, &s_PPData->quadVAO);
    if (s_PPData->quadVBO)
      glDeleteBuffers(1, &s_PPData->quadVBO);
    delete s_PPData;
    s_PPData = nullptr;
  }
}

void PostProcess::InitQuad() {
  float quadVertices[] = {-1.0f, 1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,
                          0.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f,

                          -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  -1.0f,
                          1.0f,  0.0f, 1.0f, 1.0f,  1.0f,  1.0f};

  glGenVertexArrays(1, &s_PPData->quadVAO);
  glGenBuffers(1, &s_PPData->quadVBO);
  glBindVertexArray(s_PPData->quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, s_PPData->quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glBindVertexArray(0);
}

void PostProcess::Resolve(u32 hdrColorTexture, u32 ssaoTexture, f32 exposure,
                          f32 gamma) {
  if (!s_PPData || !s_PPData->tonemapShader)
    return;

  glDisable(GL_DEPTH_TEST);

  s_PPData->tonemapShader->Bind();
  s_PPData->tonemapShader->SetFloat("u_Exposure", exposure);
  s_PPData->tonemapShader->SetFloat("u_Gamma", gamma);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, hdrColorTexture);
  s_PPData->tonemapShader->SetInt("u_HDRBuffer", 0);

  if (ssaoTexture) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ssaoTexture);
    s_PPData->tonemapShader->SetInt("u_SSAOBuffer", 1);
    s_PPData->tonemapShader->SetInt("u_HasSSAO", 1);
  } else {
    s_PPData->tonemapShader->SetInt("u_HasSSAO", 0);
  }

  glBindVertexArray(s_PPData->quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);

  glEnable(GL_DEPTH_TEST);
}

// Diligent Engine integration implementations
void PostProcess::CreateDiligentPostProcess() {
  // Diligent Engine not available, this is a no-op
  // OpenGL post-processing is already functional
  s_HasDiligentResources = false;
}

void PostProcess::UpdateDiligentPostProcess() {
  // Diligent Engine not available, this is a no-op
  // OpenGL post-processing updates are handled by the existing system
}

void PostProcess::RegisterWithHybridManager() {
  // Diligent Engine not available, this is a no-op
  // OpenGL post-processing is already functional
}

} // namespace Gini
