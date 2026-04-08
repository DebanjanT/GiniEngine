#include "Renderer3D.h"
#include "Renderer/IBL.h"
#include "Renderer/ShadowMap.h"
#include "Core/Logger.h"

#include <algorithm>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Gini {

struct Renderer3DData {
  Ref<Shader> pbrShader;
  Ref<Shader> skinnedPBRShader;
  Ref<Shader> basicShader;
  Ref<Shader> skyboxShader;
  Ref<Shader> lineShader;

  Ref<Mesh> cubeMesh;
  Ref<Mesh> sphereMesh;
  Ref<Mesh> planeMesh;

  Mat4 viewMatrix;
  Mat4 projectionMatrix;
  Vec3 cameraPosition;

  Renderer3DStats stats;

  bool wireframeMode = false;
};

static Renderer3DData *s_Data = nullptr;

// PBR Shader source
static const char *s_PBRVertexShader = R"(
#version 410 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoords;
out mat3 v_TBN;
out vec3 v_TangentViewDir;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat3 u_NormalMatrix;
uniform vec3 u_CameraPos;

void main() {
    v_WorldPos = vec3(u_Model * vec4(a_Position, 1.0));
    v_Normal = u_NormalMatrix * a_Normal;
    v_TexCoords = a_TexCoords;
    
    vec3 T = normalize(u_NormalMatrix * a_Tangent);
    vec3 B = normalize(u_NormalMatrix * a_Bitangent);
    vec3 N = normalize(v_Normal);
    v_TBN = mat3(T, B, N);
    
    // Tangent-space view direction for parallax mapping
    mat3 TBN_inv = transpose(v_TBN);
    v_TangentViewDir = TBN_inv * (u_CameraPos - v_WorldPos);
    
    gl_Position = u_Projection * u_View * vec4(v_WorldPos, 1.0);
}
)";

static const char *s_PBRFragmentShader = R"(
#version 410 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec3 gNormal;

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoords;
in mat3 v_TBN;
in vec3 v_TangentViewDir;

// Material
uniform vec3 u_Material_albedo;
uniform float u_Material_metallic;
uniform float u_Material_roughness;
uniform float u_Material_ao;
uniform vec3 u_Material_emissive;

// Textures
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_RoughnessMap;
uniform sampler2D u_AOMap;
uniform sampler2D u_HeightMap;

uniform int u_HasAlbedoMap;
uniform int u_HasNormalMap;
uniform int u_HasMetallicMap;
uniform int u_HasRoughnessMap;
uniform int u_HasAOMap;
uniform int u_HasHeightMap;
uniform float u_HeightScale;

// Lights
struct AmbientLight {
    vec3 color;
    float intensity;
};

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float radius;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float innerCutoff;
    float outerCutoff;
    float constant;
    float linear;
    float quadratic;
};

uniform AmbientLight u_AmbientLight;
uniform DirectionalLight u_DirectionalLight;
uniform int u_HasDirectionalLight;
uniform PointLight u_PointLights[32];
uniform int u_PointLightCount;
uniform SpotLight u_SpotLights[16];
uniform int u_SpotLightCount;

uniform vec3 u_CameraPos;

// IBL
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_PrefilterMap;
uniform sampler2D u_BRDFLUT;
uniform float u_MaxReflectionLod;
uniform int u_HasIBL;

// Shadow mapping (CSM)
uniform sampler2DArray u_ShadowMap;
uniform mat4 u_LightSpaceMatrices[3];
uniform float u_CascadeSplits[3];
uniform int u_CascadeCount;
uniform int u_HasShadows;
uniform mat4 u_View;

const float PI = 3.14159265359;

// PBR functions
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float ShadowCalculation(vec3 worldPos, vec3 normal, vec3 lightDir) {
    if (u_HasShadows == 0) return 0.0;

    vec4 viewPos = u_View * vec4(worldPos, 1.0);
    float depthValue = -viewPos.z;

    int cascade = u_CascadeCount - 1;
    for (int i = 0; i < u_CascadeCount; i++) {
        if (depthValue < u_CascadeSplits[i]) {
            cascade = i;
            break;
        }
    }

    vec4 lightSpacePos = u_LightSpaceMatrices[cascade] * vec4(worldPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(u_ShadowMap, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(u_ShadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize.xy, float(cascade))).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

// Parallax Occlusion Mapping
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir) {
    const int minLayers = 8;
    const int maxLayers = 32;
    float numLayers = mix(float(maxLayers), float(minLayers),
                          abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));

    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = viewDir.xy / viewDir.z * u_HeightScale;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(u_HeightMap, currentTexCoords).r;

    for (int i = 0; i < maxLayers; i++) {
        if (currentLayerDepth >= currentDepthMapValue) break;
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(u_HeightMap, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    // Occlusion interpolation for smoother results
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(u_HeightMap, prevTexCoords).r
                        - currentLayerDepth + layerDepth;
    float weight = afterDepth / (afterDepth - beforeDepth);
    return prevTexCoords * weight + currentTexCoords * (1.0 - weight);
}

void main() {
    // Apply parallax mapping to texture coordinates
    vec2 texCoords = v_TexCoords;
    if (u_HasHeightMap == 1) {
        vec3 tangentViewDir = normalize(v_TangentViewDir);
        texCoords = ParallaxMapping(v_TexCoords, tangentViewDir);
        // Discard fragments outside [0,1] to avoid edge artifacts
        if (texCoords.x > 1.0 || texCoords.y > 1.0 ||
            texCoords.x < 0.0 || texCoords.y < 0.0)
            discard;
    }

    // Get material properties
    vec3 albedo = u_Material_albedo;
    float metallic = u_Material_metallic;
    float roughness = u_Material_roughness;
    float ao = u_Material_ao;
    
    if (u_HasAlbedoMap == 1) {
        albedo = pow(texture(u_AlbedoMap, texCoords).rgb, vec3(2.2));
    }
    if (u_HasMetallicMap == 1) {
        metallic = texture(u_MetallicMap, texCoords).r;
    }
    if (u_HasRoughnessMap == 1) {
        roughness = texture(u_RoughnessMap, texCoords).r;
    }
    if (u_HasAOMap == 1) {
        ao = texture(u_AOMap, texCoords).r;
    }
    
    // Normal mapping
    vec3 N = normalize(v_Normal);
    if (u_HasNormalMap == 1) {
        N = texture(u_NormalMap, texCoords).rgb;
        N = N * 2.0 - 1.0;
        N = normalize(v_TBN * N);
    }
    
    vec3 V = normalize(u_CameraPos - v_WorldPos);
    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    vec3 Lo = vec3(0.0);
    
    // Directional light
    if (u_HasDirectionalLight == 1) {
        vec3 L = normalize(-u_DirectionalLight.direction);
        vec3 H = normalize(V + L);
        vec3 radiance = u_DirectionalLight.color * u_DirectionalLight.intensity;
        
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    // Point lights
    for (int i = 0; i < u_PointLightCount; i++) {
        vec3 L = normalize(u_PointLights[i].position - v_WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(u_PointLights[i].position - v_WorldPos);
        float attenuation = 1.0 / (u_PointLights[i].constant + 
                                   u_PointLights[i].linear * distance + 
                                   u_PointLights[i].quadratic * distance * distance);
        vec3 radiance = u_PointLights[i].color * u_PointLights[i].intensity * attenuation;
        
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    // Spot lights
    for (int i = 0; i < u_SpotLightCount; i++) {
        vec3 L = normalize(u_SpotLights[i].position - v_WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(u_SpotLights[i].position - v_WorldPos);
        float attenuation = 1.0 / (u_SpotLights[i].constant +
                                   u_SpotLights[i].linear * distance +
                                   u_SpotLights[i].quadratic * distance * distance);

        float theta = dot(L, normalize(-u_SpotLights[i].direction));
        float epsilon = u_SpotLights[i].innerCutoff - u_SpotLights[i].outerCutoff;
        float spotIntensity = clamp((theta - u_SpotLights[i].outerCutoff) / epsilon, 0.0, 1.0);

        vec3 radiance = u_SpotLights[i].color * u_SpotLights[i].intensity * attenuation * spotIntensity;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Shadow
    float shadow = 0.0;
    if (u_HasDirectionalLight == 1) {
        vec3 L = normalize(-u_DirectionalLight.direction);
        shadow = ShadowCalculation(v_WorldPos, N, L);
    }
    Lo *= (1.0 - shadow);

    // Ambient / IBL
    vec3 ambient;
    if (u_HasIBL == 1) {
        vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 irradiance = texture(u_IrradianceMap, N).rgb;
        vec3 diffuseIBL = irradiance * albedo;

        vec3 R = reflect(-V, N);
        vec3 prefilteredColor = textureLod(u_PrefilterMap, R, roughness * u_MaxReflectionLod).rgb;
        vec2 brdf = texture(u_BRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specularIBL = prefilteredColor * (F * brdf.x + brdf.y);

        ambient = (kD * diffuseIBL + specularIBL) * ao;
    } else {
        ambient = u_AmbientLight.color * u_AmbientLight.intensity * albedo * ao;
    }

    // Emissive
    vec3 emissive = u_Material_emissive;

    vec3 color = ambient + Lo + emissive;

    FragColor = vec4(color, 1.0);
    gNormal = N * 0.5 + 0.5;
}
)";

// Basic shader for simple rendering
static const char *s_BasicVertexShader = R"(
#version 410 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;

out vec3 v_Normal;
out vec2 v_TexCoords;
out vec3 v_WorldPos;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat3 u_NormalMatrix;

void main() {
    v_WorldPos = vec3(u_Model * vec4(a_Position, 1.0));
    v_Normal = u_NormalMatrix * a_Normal;
    v_TexCoords = a_TexCoords;
    gl_Position = u_Projection * u_View * vec4(v_WorldPos, 1.0);
}
)";

static const char *s_BasicFragmentShader = R"(
#version 410 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec3 gNormal;

in vec3 v_Normal;
in vec2 v_TexCoords;
in vec3 v_WorldPos;

uniform vec4 u_Color;
uniform sampler2D u_Texture;
uniform int u_HasTexture;
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;

void main() {
    vec3 norm = normalize(v_Normal);
    vec3 lightDir = normalize(-u_LightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor;
    vec3 ambient = vec3(0.2);
    
    vec4 texColor = u_Color;
    if (u_HasTexture == 1) {
        texColor = texture(u_Texture, v_TexCoords) * u_Color;
    }
    
    vec3 result = (ambient + diffuse) * texColor.rgb;
    FragColor = vec4(result, texColor.a * u_Color.a);
    gNormal = norm * 0.5 + 0.5;
}
)";

// Line shader
static const char *s_LineVertexShader = R"(
#version 410 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;

out vec4 v_Color;

uniform mat4 u_ViewProjection;

void main() {
    v_Color = a_Color;
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}
)";

static const char *s_LineFragmentShader = R"(
#version 410 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec3 gNormal;

in vec4 v_Color;

void main() {
    FragColor = v_Color;
    gNormal = vec3(0.0, 0.5, 0.0);
}
)";

// Skinned PBR vertex shader (adds bone transform)
static const char *s_SkinnedPBRVertexShader = R"(
#version 410 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;
layout (location = 5) in ivec4 a_BoneIDs;
layout (location = 6) in vec4 a_BoneWeights;

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoords;
out mat3 v_TBN;
out vec3 v_TangentViewDir;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat3 u_NormalMatrix;
uniform vec3 u_CameraPos;

const int MAX_BONES = 100;
uniform mat4 u_BoneMatrices[MAX_BONES];
uniform int u_HasBones;

void main() {
    mat4 boneTransform = mat4(1.0);
    if (u_HasBones == 1) {
        boneTransform = mat4(0.0);
        for (int i = 0; i < 4; i++) {
            if (a_BoneIDs[i] >= 0 && a_BoneIDs[i] < MAX_BONES) {
                boneTransform += u_BoneMatrices[a_BoneIDs[i]] * a_BoneWeights[i];
            }
        }
    }

    vec4 localPos = boneTransform * vec4(a_Position, 1.0);
    v_WorldPos = vec3(u_Model * localPos);

    mat3 boneMat3 = mat3(boneTransform);
    v_Normal = u_NormalMatrix * (boneMat3 * a_Normal);
    v_TexCoords = a_TexCoords;

    vec3 T = normalize(u_NormalMatrix * (boneMat3 * a_Tangent));
    vec3 B = normalize(u_NormalMatrix * (boneMat3 * a_Bitangent));
    vec3 N = normalize(v_Normal);
    v_TBN = mat3(T, B, N);

    mat3 TBN_inv = transpose(v_TBN);
    v_TangentViewDir = TBN_inv * (u_CameraPos - v_WorldPos);

    gl_Position = u_Projection * u_View * vec4(v_WorldPos, 1.0);
}
)";

void Renderer3D::Init() {
  s_Data = new Renderer3DData();

  GINI_INFO("Initializing 3D Renderer");

  // Enable depth testing
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // Enable face culling
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  // Enable blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  InitShaders();
  InitPrimitives();
}

void Renderer3D::Shutdown() {
  GINI_INFO("Shutting down 3D Renderer");
  delete s_Data;
  s_Data = nullptr;
}

void Renderer3D::InitShaders() {
  s_Data->pbrShader = Shader::Create(s_PBRVertexShader, s_PBRFragmentShader);
  s_Data->skinnedPBRShader =
      Shader::Create(s_SkinnedPBRVertexShader, s_PBRFragmentShader);
  s_Data->basicShader =
      Shader::Create(s_BasicVertexShader, s_BasicFragmentShader);
  s_Data->lineShader = Shader::Create(s_LineVertexShader, s_LineFragmentShader);
}

void Renderer3D::InitPrimitives() {
  s_Data->cubeMesh = Mesh::CreateCube(1.0f);
  s_Data->sphereMesh = Mesh::CreateSphere(1.0f, 32, 16);
  s_Data->planeMesh = Mesh::CreatePlane(1.0f, 1.0f);
}

void Renderer3D::BeginScene(const Camera3D &camera) {
  BeginScene(camera.GetViewMatrix(), camera.GetProjectionMatrix(),
             camera.GetPosition());
}

void Renderer3D::BeginScene(const Mat4 &viewMatrix,
                            const Mat4 &projectionMatrix,
                            const Vec3 &cameraPosition) {
  s_Data->viewMatrix = viewMatrix;
  s_Data->projectionMatrix = projectionMatrix;
  s_Data->cameraPosition = cameraPosition;
  s_Data->stats.Reset();
}

void Renderer3D::EndScene() {
  // Flush any batched draws
}

void Renderer3D::SetClearColor(const Color &color) {
  glClearColor(color.r, color.g, color.b, color.a);
}

void Renderer3D::Clear() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }

void Renderer3D::SetViewport(i32 x, i32 y, i32 width, i32 height) {
  glViewport(x, y, width, height);
}

void Renderer3D::DrawMesh(const Ref<Mesh> &mesh, const Mat4 &transform,
                          const Color &color) {
  if (!mesh)
    return;

  s_Data->basicShader->Bind();
  s_Data->basicShader->SetMat4("u_Model", transform);
  s_Data->basicShader->SetMat4("u_View", s_Data->viewMatrix);
  s_Data->basicShader->SetMat4("u_Projection", s_Data->projectionMatrix);
  s_Data->basicShader->SetMat3("u_NormalMatrix",
                               glm::transpose(glm::inverse(Mat3(transform))));
  s_Data->basicShader->SetFloat4("u_Color",
                                 Vec4(color.r, color.g, color.b, color.a));
  s_Data->basicShader->SetInt("u_HasTexture", 0);
  s_Data->basicShader->SetVec3("u_LightDir", Vec3(-0.2f, -1.0f, -0.3f));
  s_Data->basicShader->SetVec3("u_LightColor", Vec3(1.0f));

  mesh->Draw();

  s_Data->stats.drawCalls++;
  s_Data->stats.triangles += mesh->GetIndexCount() / 3;
  s_Data->stats.vertices += mesh->GetVertexCount();
  s_Data->stats.meshesDrawn++;
}

void Renderer3D::DrawMesh(const Ref<Mesh> &mesh, const Mat4 &transform,
                          const Ref<Texture2D> &texture) {
  if (!mesh)
    return;

  s_Data->basicShader->Bind();
  s_Data->basicShader->SetMat4("u_Model", transform);
  s_Data->basicShader->SetMat4("u_View", s_Data->viewMatrix);
  s_Data->basicShader->SetMat4("u_Projection", s_Data->projectionMatrix);
  s_Data->basicShader->SetMat3("u_NormalMatrix",
                               glm::transpose(glm::inverse(Mat3(transform))));
  s_Data->basicShader->SetFloat4("u_Color", Vec4(1.0f));
  s_Data->basicShader->SetVec3("u_LightDir", Vec3(-0.2f, -1.0f, -0.3f));
  s_Data->basicShader->SetVec3("u_LightColor", Vec3(1.0f));

  if (texture) {
    texture->Bind(0);
    s_Data->basicShader->SetInt("u_Texture", 0);
    s_Data->basicShader->SetInt("u_HasTexture", 1);
  } else {
    s_Data->basicShader->SetInt("u_HasTexture", 0);
  }

  mesh->Draw();

  s_Data->stats.drawCalls++;
  s_Data->stats.triangles += mesh->GetIndexCount() / 3;
  s_Data->stats.vertices += mesh->GetVertexCount();
  s_Data->stats.meshesDrawn++;
}

void Renderer3D::DrawMesh(const Ref<Mesh> &mesh, const Mat4 &transform,
                          const Material3D &material) {
  if (!mesh)
    return;

  s_Data->pbrShader->Bind();
  s_Data->pbrShader->SetMat4("u_Model", transform);
  s_Data->pbrShader->SetMat4("u_View", s_Data->viewMatrix);
  s_Data->pbrShader->SetMat4("u_Projection", s_Data->projectionMatrix);
  s_Data->pbrShader->SetMat3("u_NormalMatrix",
                             glm::transpose(glm::inverse(Mat3(transform))));
  s_Data->pbrShader->SetVec3("u_CameraPos", s_Data->cameraPosition);

  // Material properties
  s_Data->pbrShader->SetVec3("u_Material_albedo", material.albedo);
  s_Data->pbrShader->SetFloat("u_Material_metallic", material.metallic);
  s_Data->pbrShader->SetFloat("u_Material_roughness", material.roughness);
  s_Data->pbrShader->SetFloat("u_Material_ao", material.ao);
  s_Data->pbrShader->SetVec3("u_Material_emissive", material.emissive);

  // Upload lights
  LightManager::Get().UploadToShader(s_Data->pbrShader.get());

  // Bind textures
  u32 textureUnit = 0;

  if (material.albedoMap) {
    material.albedoMap->Bind(textureUnit);
    s_Data->pbrShader->SetInt("u_AlbedoMap", textureUnit++);
    s_Data->pbrShader->SetInt("u_HasAlbedoMap", 1);
  } else {
    s_Data->pbrShader->SetInt("u_HasAlbedoMap", 0);
  }

  if (material.normalMap) {
    material.normalMap->Bind(textureUnit);
    s_Data->pbrShader->SetInt("u_NormalMap", textureUnit++);
    s_Data->pbrShader->SetInt("u_HasNormalMap", 1);
  } else {
    s_Data->pbrShader->SetInt("u_HasNormalMap", 0);
  }

  if (material.metallicMap) {
    material.metallicMap->Bind(textureUnit);
    s_Data->pbrShader->SetInt("u_MetallicMap", textureUnit++);
    s_Data->pbrShader->SetInt("u_HasMetallicMap", 1);
  } else {
    s_Data->pbrShader->SetInt("u_HasMetallicMap", 0);
  }

  if (material.roughnessMap) {
    material.roughnessMap->Bind(textureUnit);
    s_Data->pbrShader->SetInt("u_RoughnessMap", textureUnit++);
    s_Data->pbrShader->SetInt("u_HasRoughnessMap", 1);
  } else {
    s_Data->pbrShader->SetInt("u_HasRoughnessMap", 0);
  }

  if (material.aoMap) {
    material.aoMap->Bind(textureUnit);
    s_Data->pbrShader->SetInt("u_AOMap", textureUnit++);
    s_Data->pbrShader->SetInt("u_HasAOMap", 1);
  } else {
    s_Data->pbrShader->SetInt("u_HasAOMap", 0);
  }

  if (material.heightMap) {
    material.heightMap->Bind(textureUnit);
    s_Data->pbrShader->SetInt("u_HeightMap", textureUnit++);
    s_Data->pbrShader->SetInt("u_HasHeightMap", 1);
    s_Data->pbrShader->SetFloat("u_HeightScale", material.heightScale);
  } else {
    s_Data->pbrShader->SetInt("u_HasHeightMap", 0);
  }

  // View matrix for shadow cascade selection
  s_Data->pbrShader->SetMat4("u_View", s_Data->viewMatrix);

  // Bind IBL textures (units 10-12)
  IBL::BindIBLTextures(s_Data->pbrShader.get(), 10);

  // Bind shadow maps (unit 13)
  if (ShadowMap::IsInitialized()) {
    ShadowMap::BindShadowMaps(s_Data->pbrShader.get(), 13);
  } else {
    s_Data->pbrShader->SetInt("u_HasShadows", 0);
  }

  mesh->Draw();

  s_Data->stats.drawCalls++;
  s_Data->stats.triangles += mesh->GetIndexCount() / 3;
  s_Data->stats.vertices += mesh->GetVertexCount();
  s_Data->stats.meshesDrawn++;
}

void Renderer3D::DrawModel(const Ref<Model> &model, const Mat4 &transform) {
  if (!model)
    return;

  const auto &meshes = model->GetMeshes();
  const auto &materials = model->GetMaterials();
  const auto &materialIndices = model->GetMeshMaterialIndices();

  for (u32 i = 0; i < meshes.size(); i++) {
    if (i < materialIndices.size() && materialIndices[i] >= 0 &&
        materialIndices[i] < static_cast<i32>(materials.size())) {
      DrawMesh(meshes[i], transform, materials[materialIndices[i]]);
    } else {
      Material3D defaultMat;
      DrawMesh(meshes[i], transform, defaultMat);
    }
  }
}

void Renderer3D::DrawModel(const Ref<Model> &model, const Vec3 &position,
                           const Vec3 &rotation, const Vec3 &scale) {
  Mat4 transform = glm::translate(Mat4(1.0f), position);
  transform = glm::rotate(transform, glm::radians(rotation.x), Vec3(1, 0, 0));
  transform = glm::rotate(transform, glm::radians(rotation.y), Vec3(0, 1, 0));
  transform = glm::rotate(transform, glm::radians(rotation.z), Vec3(0, 0, 1));
  transform = glm::scale(transform, scale);

  DrawModel(model, transform);
}

void Renderer3D::DrawCube(const Vec3 &position, const Vec3 &size,
                          const Color &color) {
  Mat4 transform = glm::translate(Mat4(1.0f), position);
  transform = glm::scale(transform, size);
  DrawMesh(s_Data->cubeMesh, transform, color);
}

void Renderer3D::DrawCube(const Vec3 &position, const Vec3 &size,
                          const Ref<Texture2D> &texture) {
  Mat4 transform = glm::translate(Mat4(1.0f), position);
  transform = glm::scale(transform, size);
  DrawMesh(s_Data->cubeMesh, transform, texture);
}

void Renderer3D::DrawSphere(const Vec3 &position, f32 radius,
                            const Color &color) {
  Mat4 transform = glm::translate(Mat4(1.0f), position);
  transform = glm::scale(transform, Vec3(radius));
  DrawMesh(s_Data->sphereMesh, transform, color);
}

void Renderer3D::DrawPlane(const Vec3 &position, const Vec2 &size,
                           const Color &color) {
  Mat4 transform = glm::translate(Mat4(1.0f), position);
  transform = glm::scale(transform, Vec3(size.x, 1.0f, size.y));
  DrawMesh(s_Data->planeMesh, transform, color);
}

void Renderer3D::DrawSkinnedModel(const Ref<Model> &model,
                                  const Mat4 &transform,
                                  const std::vector<Mat4> &boneMatrices) {
  if (!model)
    return;

  const auto &meshes = model->GetMeshes();
  const auto &materials = model->GetMaterials();
  const auto &materialIndices = model->GetMeshMaterialIndices();

  auto shader = s_Data->skinnedPBRShader;
  shader->Bind();
  shader->SetMat4("u_Model", transform);
  shader->SetMat4("u_View", s_Data->viewMatrix);
  shader->SetMat4("u_Projection", s_Data->projectionMatrix);
  shader->SetMat3("u_NormalMatrix",
                   glm::transpose(glm::inverse(Mat3(transform))));
  shader->SetVec3("u_CameraPos", s_Data->cameraPosition);

  if (!boneMatrices.empty()) {
    shader->SetInt("u_HasBones", 1);
    u32 count =
        static_cast<u32>(std::min(boneMatrices.size(), size_t(100)));
    for (u32 i = 0; i < count; i++) {
      std::string uniformName = "u_BoneMatrices[" + std::to_string(i) + "]";
      shader->SetMat4(uniformName, boneMatrices[i]);
    }
  } else {
    shader->SetInt("u_HasBones", 0);
  }

  LightManager::Get().UploadToShader(shader.get());
  IBL::BindIBLTextures(shader.get(), 10);
  if (ShadowMap::IsInitialized()) {
    ShadowMap::BindShadowMaps(shader.get(), 13);
  } else {
    shader->SetInt("u_HasShadows", 0);
  }

  for (u32 i = 0; i < meshes.size(); i++) {
    Material3D mat;
    if (i < materialIndices.size() && materialIndices[i] >= 0 &&
        materialIndices[i] < static_cast<i32>(materials.size())) {
      mat = materials[materialIndices[i]];
    }

    shader->SetVec3("u_Material_albedo", mat.albedo);
    shader->SetFloat("u_Material_metallic", mat.metallic);
    shader->SetFloat("u_Material_roughness", mat.roughness);
    shader->SetFloat("u_Material_ao", mat.ao);
    shader->SetVec3("u_Material_emissive", mat.emissive);

    u32 texUnit = 0;
    auto bindTex = [&](Ref<Texture2D> &tex, const char *mapUniform,
                       const char *hasUniform) {
      if (tex) {
        tex->Bind(texUnit);
        shader->SetInt(mapUniform, texUnit++);
        shader->SetInt(hasUniform, 1);
      } else {
        shader->SetInt(hasUniform, 0);
      }
    };
    bindTex(mat.albedoMap, "u_AlbedoMap", "u_HasAlbedoMap");
    bindTex(mat.normalMap, "u_NormalMap", "u_HasNormalMap");
    bindTex(mat.metallicMap, "u_MetallicMap", "u_HasMetallicMap");
    bindTex(mat.roughnessMap, "u_RoughnessMap", "u_HasRoughnessMap");
    bindTex(mat.aoMap, "u_AOMap", "u_HasAOMap");
    bindTex(mat.heightMap, "u_HeightMap", "u_HasHeightMap");
    if (!mat.heightMap) {
      shader->SetInt("u_HasHeightMap", 0);
    }

    meshes[i]->Draw();

    s_Data->stats.drawCalls++;
    s_Data->stats.triangles += meshes[i]->GetIndexCount() / 3;
    s_Data->stats.vertices += meshes[i]->GetVertexCount();
    s_Data->stats.meshesDrawn++;
  }
}

Ref<Shader> Renderer3D::GetPBRShader() { return s_Data->pbrShader; }

Ref<Shader> Renderer3D::GetSkinnedPBRShader() {
  return s_Data->skinnedPBRShader;
}

Ref<Shader> Renderer3D::GetBasicShader() { return s_Data->basicShader; }

Ref<Shader> Renderer3D::GetSkyboxShader() { return s_Data->skyboxShader; }

const Renderer3DStats &Renderer3D::GetStats() { return s_Data->stats; }

void Renderer3D::ResetStats() { s_Data->stats.Reset(); }

void Renderer3D::SetWireframeMode(bool enabled) {
  s_Data->wireframeMode = enabled;
  glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
}

void Renderer3D::SetDepthTest(bool enabled) {
  if (enabled) {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }
}

void Renderer3D::SetCullFace(bool enabled) {
  if (enabled) {
    glEnable(GL_CULL_FACE);
  } else {
    glDisable(GL_CULL_FACE);
  }
}

void Renderer3D::DrawSkybox(const Ref<class TextureCube> &cubemap) {
  // TODO: Implement skybox rendering
}

void Renderer3D::DrawLine(const Vec3 &start, const Vec3 &end,
                          const Color &color) {
  // Create line vertex data
  float vertices[] = {start.x, start.y, start.z, color.r, color.g,
                      color.b, color.a, end.x,   end.y,   end.z,
                      color.r, color.g, color.b, color.a};

  // Create VAO and VBO for the line
  GLuint vao, vbo;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

  // Position attribute
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)0);

  // Color attribute
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                        (void *)(3 * sizeof(float)));

  // Use line shader
  s_Data->lineShader->Bind();
  Mat4 viewProjection = s_Data->projectionMatrix * s_Data->viewMatrix;
  s_Data->lineShader->SetMat4("u_ViewProjection", viewProjection);

  // Draw line
  glDrawArrays(GL_LINES, 0, 2);

  // Cleanup
  glBindVertexArray(0);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);

  s_Data->stats.drawCalls++;
}

void Renderer3D::DrawWireCube(const Vec3 &position, const Vec3 &size,
                              const Color &color) {
  bool wasWireframe = s_Data->wireframeMode;
  SetWireframeMode(true);
  DrawCube(position, size, color);
  SetWireframeMode(wasWireframe);
}

void Renderer3D::DrawWireSphere(const Vec3 &position, f32 radius,
                                const Color &color) {
  bool wasWireframe = s_Data->wireframeMode;
  SetWireframeMode(true);
  DrawSphere(position, radius, color);
  SetWireframeMode(wasWireframe);
}

void Renderer3D::DrawGrid(f32 size, u32 divisions, const Color &color) {
  // TODO: Implement grid rendering
}

} // namespace Gini
