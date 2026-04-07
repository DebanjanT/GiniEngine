#include "SSAO.h"
#include "Core/Logger.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <random>

namespace Gini {

struct SSAOData {
    u32 ssaoFBO = 0;
    u32 ssaoTexture = 0;
    u32 blurFBO = 0;
    u32 blurTexture = 0;
    u32 noiseTexture = 0;
    u32 quadVAO = 0;
    u32 quadVBO = 0;
    Ref<Shader> ssaoShader;
    Ref<Shader> blurShader;
    std::vector<Vec3> kernel;
    u32 width = 0;
    u32 height = 0;
    bool initialized = false;
};

static SSAOData* s_SSAO = nullptr;

static const char* s_SSAO_VS = R"(
#version 410 core
layout (location = 0) in vec2 a_Position;
layout (location = 1) in vec2 a_TexCoords;
out vec2 v_TexCoords;
void main() {
    v_TexCoords = a_TexCoords;
    gl_Position = vec4(a_Position, 0.0, 1.0);
}
)";

static const char* s_SSAO_FS = R"(
#version 410 core
out float FragColor;
in vec2 v_TexCoords;

uniform sampler2D u_DepthTexture;
uniform sampler2D u_NormalTexture;
uniform sampler2D u_NoiseTexture;

uniform vec3 u_Samples[64];
uniform mat4 u_Projection;
uniform mat4 u_View;
uniform vec2 u_NoiseScale;

const int kernelSize = 32;
const float radius = 0.5;
const float bias = 0.025;

float LinearizeDepth(float depth) {
    float near = 0.1;
    float far = 1000.0;
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main() {
    float depth = texture(u_DepthTexture, v_TexCoords).r;
    if (depth >= 1.0) { FragColor = 1.0; return; }

    float linearDepth = LinearizeDepth(depth);
    vec4 clipPos = vec4(v_TexCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPos4 = inverse(u_Projection) * clipPos;
    vec3 fragPos = viewPos4.xyz / viewPos4.w;

    vec3 normal = texture(u_NormalTexture, v_TexCoords).rgb;
    normal = mat3(u_View) * (normal * 2.0 - 1.0);
    normal = normalize(normal);

    vec3 randomVec = normalize(texture(u_NoiseTexture, v_TexCoords * u_NoiseScale).xyz);
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; ++i) {
        vec3 samplePos = TBN * u_Samples[i];
        samplePos = fragPos + samplePos * radius;

        vec4 offset = u_Projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = LinearizeDepth(texture(u_DepthTexture, offset.xy).r);
        float viewSampleZ = -samplePos.z;
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(linearDepth - sampleDepth));
        occlusion += (sampleDepth <= viewSampleZ - bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(kernelSize));
    FragColor = pow(occlusion, 2.0);
}
)";

static const char* s_SSAOBlur_FS = R"(
#version 410 core
out float FragColor;
in vec2 v_TexCoords;
uniform sampler2D u_SSAOInput;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(u_SSAOInput, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(u_SSAOInput, v_TexCoords + offset).r;
        }
    }
    FragColor = result / 16.0;
}
)";

void SSAO::Init(u32 width, u32 height) {
    s_SSAO = new SSAOData();
    s_SSAO->width = width;
    s_SSAO->height = height;

    s_SSAO->ssaoShader = Shader::Create(s_SSAO_VS, s_SSAO_FS);
    s_SSAO->blurShader = Shader::Create(s_SSAO_VS, s_SSAOBlur_FS);

    // Generate kernel
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;
    s_SSAO->kernel.resize(64);
    for (u32 i = 0; i < 64; i++) {
        Vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator)
        );
        sample = glm::normalize(sample) * randomFloats(generator);
        f32 scale = static_cast<f32>(i) / 64.0f;
        scale = 0.1f + scale * scale * (1.0f - 0.1f);
        sample *= scale;
        s_SSAO->kernel[i] = sample;
    }

    // Generate noise texture (4x4)
    std::vector<Vec3> ssaoNoise;
    for (u32 i = 0; i < 16; i++) {
        Vec3 noise(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            0.0f
        );
        ssaoNoise.push_back(noise);
    }
    glGenTextures(1, &s_SSAO->noiseTexture);
    glBindTexture(GL_TEXTURE_2D, s_SSAO->noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, ssaoNoise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Quad VAO
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };
    glGenVertexArrays(1, &s_SSAO->quadVAO);
    glGenBuffers(1, &s_SSAO->quadVBO);
    glBindVertexArray(s_SSAO->quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_SSAO->quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // Create FBOs
    auto createFBO = [](u32& fbo, u32& tex, u32 w, u32 h) {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    };

    createFBO(s_SSAO->ssaoFBO, s_SSAO->ssaoTexture, width, height);
    createFBO(s_SSAO->blurFBO, s_SSAO->blurTexture, width, height);

    s_SSAO->initialized = true;
    GINI_INFO("SSAO initialized ({}x{})", width, height);
}

void SSAO::Shutdown() {
    if (s_SSAO) {
        if (s_SSAO->ssaoFBO) glDeleteFramebuffers(1, &s_SSAO->ssaoFBO);
        if (s_SSAO->ssaoTexture) glDeleteTextures(1, &s_SSAO->ssaoTexture);
        if (s_SSAO->blurFBO) glDeleteFramebuffers(1, &s_SSAO->blurFBO);
        if (s_SSAO->blurTexture) glDeleteTextures(1, &s_SSAO->blurTexture);
        if (s_SSAO->noiseTexture) glDeleteTextures(1, &s_SSAO->noiseTexture);
        if (s_SSAO->quadVAO) glDeleteVertexArrays(1, &s_SSAO->quadVAO);
        if (s_SSAO->quadVBO) glDeleteBuffers(1, &s_SSAO->quadVBO);
        delete s_SSAO;
        s_SSAO = nullptr;
    }
}

void SSAO::Resize(u32 width, u32 height) {
    if (!s_SSAO) return;
    s_SSAO->width = width;
    s_SSAO->height = height;

    glBindTexture(GL_TEXTURE_2D, s_SSAO->ssaoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, s_SSAO->blurTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, nullptr);
}

void SSAO::Render(u32 depthTexture, u32 normalTexture,
                  const Mat4& projection, const Mat4& view) {
    if (!s_SSAO || !s_SSAO->initialized) return;

    // SSAO pass
    glBindFramebuffer(GL_FRAMEBUFFER, s_SSAO->ssaoFBO);
    glViewport(0, 0, s_SSAO->width, s_SSAO->height);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    s_SSAO->ssaoShader->Bind();
    for (u32 i = 0; i < 64; i++)
        s_SSAO->ssaoShader->SetVec3("u_Samples[" + std::to_string(i) + "]", s_SSAO->kernel[i]);
    s_SSAO->ssaoShader->SetMat4("u_Projection", projection);
    s_SSAO->ssaoShader->SetMat4("u_View", view);
    s_SSAO->ssaoShader->SetVec2("u_NoiseScale",
        Vec2(static_cast<f32>(s_SSAO->width) / 4.0f, static_cast<f32>(s_SSAO->height) / 4.0f));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    s_SSAO->ssaoShader->SetInt("u_DepthTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    s_SSAO->ssaoShader->SetInt("u_NormalTexture", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, s_SSAO->noiseTexture);
    s_SSAO->ssaoShader->SetInt("u_NoiseTexture", 2);

    glBindVertexArray(s_SSAO->quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Blur pass
    glBindFramebuffer(GL_FRAMEBUFFER, s_SSAO->blurFBO);
    glClear(GL_COLOR_BUFFER_BIT);

    s_SSAO->blurShader->Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_SSAO->ssaoTexture);
    s_SSAO->blurShader->SetInt("u_SSAOInput", 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
}

u32 SSAO::GetSSAOTexture() { return s_SSAO ? s_SSAO->blurTexture : 0; }
bool SSAO::IsInitialized() { return s_SSAO && s_SSAO->initialized; }

} // namespace Gini
