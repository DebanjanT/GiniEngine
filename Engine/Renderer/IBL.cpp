#include "IBL.h"
#include "Core/Logger.h"
#include "Renderer/AtmosphericSky.h"
#include "Renderer/Camera3D.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Gini {

struct IBLData {
    u32 irradianceMap = 0;
    u32 prefilterMap = 0;
    u32 brdfLUT = 0;
    u32 captureFBO = 0;
    u32 captureRBO = 0;
    u32 envCubemap = 0;
    u32 skyCaptureFBO = 0;
    Ref<Shader> equirectToCubemapShader;
    Ref<Shader> irradianceShader;
    Ref<Shader> prefilterShader;
    Ref<Shader> brdfShader;
    Ref<Shader> skyCaptureShader;
    u32 cubeVAO = 0;
    u32 cubeVBO = 0;
    u32 quadVAO = 0;
    u32 quadVBO = 0;
    f32 maxReflectionLod = 4.0f;
    bool ready = false;
};

static IBLData* s_IBL = nullptr;

static const char* s_IBLIrradianceVS = R"(
#version 410 core
layout (location = 0) in vec3 a_Position;
out vec3 v_LocalPos;
uniform mat4 u_Projection;
uniform mat4 u_View;
void main() {
    v_LocalPos = a_Position;
    gl_Position = u_Projection * u_View * vec4(a_Position, 1.0);
}
)";

static const char* s_IBLIrradianceFS = R"(
#version 410 core
out vec4 FragColor;
in vec3 v_LocalPos;
uniform samplerCube u_EnvironmentMap;

const float PI = 3.14159265359;

void main() {
    vec3 N = normalize(v_LocalPos);
    vec3 irradiance = vec3(0.0);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            vec3 tangentSample = vec3(sin(theta) * cos(phi),
                                       sin(theta) * sin(phi),
                                       cos(theta));
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
            irradiance += texture(u_EnvironmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / nrSamples);
    FragColor = vec4(irradiance, 1.0);
}
)";

static const char* s_IBLPrefilterFS = R"(
#version 410 core
out vec4 FragColor;
in vec3 v_LocalPos;
uniform samplerCube u_EnvironmentMap;
uniform float u_Roughness;

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

void main() {
    vec3 N = normalize(v_LocalPos);
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, u_Roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            prefilteredColor += texture(u_EnvironmentMap, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;
    FragColor = vec4(prefilteredColor, 1.0);
}
)";

static const char* s_BRDFLUT_VS = R"(
#version 410 core
layout (location = 0) in vec2 a_Position;
layout (location = 1) in vec2 a_TexCoords;
out vec2 v_TexCoords;
void main() {
    v_TexCoords = a_TexCoords;
    gl_Position = vec4(a_Position, 0.0, 1.0);
}
)";

static const char* s_BRDFLUT_FS = R"(
#version 410 core
out vec2 FragColor;
in vec2 v_TexCoords;
const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float a = roughness;
    float k = (a * a) / 2.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec2 IntegrateBRDF(float NdotV, float roughness) {
    vec3 V = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);
    float A = 0.0;
    float B = 0.0;
    vec3 N = vec3(0.0, 0.0, 1.0);
    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);
        if (NdotL > 0.0) {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    return vec2(A, B) / float(SAMPLE_COUNT);
}

void main() {
    FragColor = IntegrateBRDF(v_TexCoords.x, v_TexCoords.y);
}
)";

static void InitCubeVAO() {
    float vertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };
    glGenVertexArrays(1, &s_IBL->cubeVAO);
    glGenBuffers(1, &s_IBL->cubeVBO);
    glBindVertexArray(s_IBL->cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_IBL->cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

static void InitQuadVAO() {
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };
    glGenVertexArrays(1, &s_IBL->quadVAO);
    glGenBuffers(1, &s_IBL->quadVBO);
    glBindVertexArray(s_IBL->quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_IBL->quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

void IBL::Init() {
    s_IBL = new IBLData();

    InitCubeVAO();
    InitQuadVAO();

    s_IBL->irradianceShader = Shader::Create(s_IBLIrradianceVS, s_IBLIrradianceFS);
    s_IBL->prefilterShader = Shader::Create(s_IBLIrradianceVS, s_IBLPrefilterFS);
    s_IBL->brdfShader = Shader::Create(s_BRDFLUT_VS, s_BRDFLUT_FS);

    glGenFramebuffers(1, &s_IBL->captureFBO);
    glGenRenderbuffers(1, &s_IBL->captureRBO);

    GenerateBRDFLUT();
    GenerateDefaultIBL();

    GINI_INFO("IBL system initialized");
}

void IBL::Shutdown() {
    if (s_IBL) {
        if (s_IBL->irradianceMap) glDeleteTextures(1, &s_IBL->irradianceMap);
        if (s_IBL->prefilterMap) glDeleteTextures(1, &s_IBL->prefilterMap);
        if (s_IBL->brdfLUT) glDeleteTextures(1, &s_IBL->brdfLUT);
        if (s_IBL->envCubemap) glDeleteTextures(1, &s_IBL->envCubemap);
        if (s_IBL->captureFBO) glDeleteFramebuffers(1, &s_IBL->captureFBO);
        if (s_IBL->captureRBO) glDeleteRenderbuffers(1, &s_IBL->captureRBO);
        if (s_IBL->cubeVAO) glDeleteVertexArrays(1, &s_IBL->cubeVAO);
        if (s_IBL->cubeVBO) glDeleteBuffers(1, &s_IBL->cubeVBO);
        if (s_IBL->quadVAO) glDeleteVertexArrays(1, &s_IBL->quadVAO);
        if (s_IBL->quadVBO) glDeleteBuffers(1, &s_IBL->quadVBO);
        delete s_IBL;
        s_IBL = nullptr;
    }
}

void IBL::GenerateBRDFLUT() {
    if (!s_IBL) return;

    glGenTextures(1, &s_IBL->brdfLUT);
    glBindTexture(GL_TEXTURE_2D, s_IBL->brdfLUT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, s_IBL->captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, s_IBL->captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_IBL->brdfLUT, 0);

    glViewport(0, 0, 512, 512);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    s_IBL->brdfShader->Bind();
    glBindVertexArray(s_IBL->quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GINI_INFO("BRDF LUT generated (512x512)");
}

void IBL::GenerateDefaultIBL() {
    if (!s_IBL) return;

    const u32 faceSize = 16;
    std::vector<float> pixels(faceSize * faceSize * 3);

    auto fillFace = [&](float topR, float topG, float topB,
                        float botR, float botG, float botB) {
        for (u32 y = 0; y < faceSize; y++) {
            float t = static_cast<float>(y) / static_cast<float>(faceSize - 1);
            float r = topR + (botR - topR) * t;
            float g = topG + (botG - topG) * t;
            float b = topB + (botB - topB) * t;
            for (u32 x = 0; x < faceSize; x++) {
                u32 idx = (y * faceSize + x) * 3;
                pixels[idx + 0] = r;
                pixels[idx + 1] = g;
                pixels[idx + 2] = b;
            }
        }
    };

    if (s_IBL->envCubemap) glDeleteTextures(1, &s_IBL->envCubemap);
    glGenTextures(1, &s_IBL->envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, s_IBL->envCubemap);

    // +X (right), -X (left), +Z (front), -Z (back): neutral gradient
    // +Y (up): sky color, -Y (down): ground color
    for (u32 i = 0; i < 6; i++) {
        if (i == 2) {
            // +Y face (top): sky blue
            fillFace(0.15f, 0.2f, 0.35f,  0.15f, 0.2f, 0.35f);
        } else if (i == 3) {
            // -Y face (bottom): dark ground
            fillFace(0.02f, 0.02f, 0.02f,  0.02f, 0.02f, 0.02f);
        } else {
            // Side faces: gradient from sky to ground
            fillFace(0.15f, 0.2f, 0.35f,  0.04f, 0.04f, 0.05f);
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     faceSize, faceSize, 0, GL_RGB, GL_FLOAT, pixels.data());
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GenerateFromCubemap(s_IBL->envCubemap, faceSize);
    GINI_INFO("Default IBL generated (fallback environment)");
}

void IBL::GenerateFromCubemap(u32 envCubemap, u32 size) {
    if (!s_IBL || envCubemap == 0) return;

    Mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    Mat4 captureViews[] = {
        glm::lookAt(Vec3(0), Vec3( 1, 0, 0), Vec3(0,-1, 0)),
        glm::lookAt(Vec3(0), Vec3(-1, 0, 0), Vec3(0,-1, 0)),
        glm::lookAt(Vec3(0), Vec3( 0, 1, 0), Vec3(0, 0, 1)),
        glm::lookAt(Vec3(0), Vec3( 0,-1, 0), Vec3(0, 0,-1)),
        glm::lookAt(Vec3(0), Vec3( 0, 0, 1), Vec3(0,-1, 0)),
        glm::lookAt(Vec3(0), Vec3( 0, 0,-1), Vec3(0,-1, 0))
    };

    // Generate irradiance map (32x32)
    if (s_IBL->irradianceMap) glDeleteTextures(1, &s_IBL->irradianceMap);
    glGenTextures(1, &s_IBL->irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, s_IBL->irradianceMap);
    for (u32 i = 0; i < 6; i++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, s_IBL->captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, s_IBL->captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    s_IBL->irradianceShader->Bind();
    s_IBL->irradianceShader->SetInt("u_EnvironmentMap", 0);
    s_IBL->irradianceShader->SetMat4("u_Projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glViewport(0, 0, 32, 32);
    for (u32 i = 0; i < 6; i++) {
        s_IBL->irradianceShader->SetMat4("u_View", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, s_IBL->irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(s_IBL->cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    // Generate prefilter map (128x128, 5 mip levels)
    if (s_IBL->prefilterMap) glDeleteTextures(1, &s_IBL->prefilterMap);
    glGenTextures(1, &s_IBL->prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, s_IBL->prefilterMap);
    for (u32 i = 0; i < 6; i++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    s_IBL->prefilterShader->Bind();
    s_IBL->prefilterShader->SetInt("u_EnvironmentMap", 0);
    s_IBL->prefilterShader->SetMat4("u_Projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    u32 maxMipLevels = 5;
    for (u32 mip = 0; mip < maxMipLevels; mip++) {
        u32 mipWidth = static_cast<u32>(128 * std::pow(0.5, mip));
        u32 mipHeight = mipWidth;
        glBindRenderbuffer(GL_RENDERBUFFER, s_IBL->captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        f32 roughness = static_cast<f32>(mip) / static_cast<f32>(maxMipLevels - 1);
        s_IBL->prefilterShader->SetFloat("u_Roughness", roughness);
        for (u32 i = 0; i < 6; i++) {
            s_IBL->prefilterShader->SetMat4("u_View", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, s_IBL->prefilterMap, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindVertexArray(s_IBL->cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    s_IBL->ready = true;
    GINI_INFO("IBL maps generated from cubemap");
}

void IBL::CaptureAtmosphericSky(AtmosphericSky* sky, const Camera3D& camera) {
    if (!s_IBL || !sky) return;

    u32 captureSize = 256;
    if (s_IBL->envCubemap) glDeleteTextures(1, &s_IBL->envCubemap);
    glGenTextures(1, &s_IBL->envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, s_IBL->envCubemap);
    for (u32 i = 0; i < 6; i++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     captureSize, captureSize, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    Mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    Mat4 captureViews[] = {
        glm::lookAt(Vec3(0), Vec3( 1, 0, 0), Vec3(0,-1, 0)),
        glm::lookAt(Vec3(0), Vec3(-1, 0, 0), Vec3(0,-1, 0)),
        glm::lookAt(Vec3(0), Vec3( 0, 1, 0), Vec3(0, 0, 1)),
        glm::lookAt(Vec3(0), Vec3( 0,-1, 0), Vec3(0, 0,-1)),
        glm::lookAt(Vec3(0), Vec3( 0, 0, 1), Vec3(0,-1, 0)),
        glm::lookAt(Vec3(0), Vec3( 0, 0,-1), Vec3(0,-1, 0))
    };

    u32 skyCaptureFBO;
    glGenFramebuffers(1, &skyCaptureFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, skyCaptureFBO);

    u32 depthRBO;
    glGenRenderbuffers(1, &depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, captureSize, captureSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);

    glViewport(0, 0, captureSize, captureSize);

    for (u32 i = 0; i < 6; i++) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, s_IBL->envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sky->Render(captureViews[i], captureProjection);
    }

    glDeleteFramebuffers(1, &skyCaptureFBO);
    glDeleteRenderbuffers(1, &depthRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GenerateFromCubemap(s_IBL->envCubemap, 256);
}

u32 IBL::GetIrradianceMap() { return s_IBL ? s_IBL->irradianceMap : 0; }
u32 IBL::GetPrefilterMap() { return s_IBL ? s_IBL->prefilterMap : 0; }
u32 IBL::GetBRDFLUT() { return s_IBL ? s_IBL->brdfLUT : 0; }
f32 IBL::GetMaxReflectionLod() { return s_IBL ? s_IBL->maxReflectionLod : 4.0f; }

void IBL::BindIBLTextures(Shader* shader, u32 startTextureUnit) {
    if (!s_IBL || !shader) return;

    if (s_IBL->ready && s_IBL->irradianceMap) {
        glActiveTexture(GL_TEXTURE0 + startTextureUnit);
        glBindTexture(GL_TEXTURE_CUBE_MAP, s_IBL->irradianceMap);
        shader->SetInt("u_IrradianceMap", static_cast<i32>(startTextureUnit));

        glActiveTexture(GL_TEXTURE0 + startTextureUnit + 1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, s_IBL->prefilterMap);
        shader->SetInt("u_PrefilterMap", static_cast<i32>(startTextureUnit + 1));

        glActiveTexture(GL_TEXTURE0 + startTextureUnit + 2);
        glBindTexture(GL_TEXTURE_2D, s_IBL->brdfLUT);
        shader->SetInt("u_BRDFLUT", static_cast<i32>(startTextureUnit + 2));

        shader->SetFloat("u_MaxReflectionLod", s_IBL->maxReflectionLod);
        shader->SetInt("u_HasIBL", 1);
    } else {
        shader->SetInt("u_HasIBL", 0);
    }
}

bool IBL::IsReady() { return s_IBL && s_IBL->ready; }

} // namespace Gini
