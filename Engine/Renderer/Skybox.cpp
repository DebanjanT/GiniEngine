#include "Skybox.h"
#include "Core/Logger.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Gini {

static const char* s_SkyboxVertexShader = R"(
#version 410 core
layout (location = 0) in vec3 a_Position;

out vec3 v_TexCoords;

uniform mat4 u_Projection;
uniform mat4 u_View;

void main() {
    v_TexCoords = a_Position;
    vec4 pos = u_Projection * u_View * vec4(a_Position, 1.0);
    gl_Position = pos.xyww; // Ensure skybox is always at max depth
}
)";

static const char* s_SkyboxFragmentShader = R"(
#version 410 core
out vec4 FragColor;

in vec3 v_TexCoords;

uniform samplerCube u_Skybox;
uniform float u_Intensity;
uniform float u_Lod;

void main() {
    vec3 color = textureLod(u_Skybox, v_TexCoords, u_Lod).rgb;
    color *= u_Intensity;
    
    // Tone mapping for HDR
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}
)";

Skybox::Skybox() {
    InitCube();
    InitShader();
}

Skybox::~Skybox() {
    if (m_CubeVAO) glDeleteVertexArrays(1, &m_CubeVAO);
    if (m_CubeVBO) glDeleteBuffers(1, &m_CubeVBO);
}

void Skybox::InitCube() {
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
    
    glGenVertexArrays(1, &m_CubeVAO);
    glGenBuffers(1, &m_CubeVBO);
    
    glBindVertexArray(m_CubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_CubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}

void Skybox::InitShader() {
    m_Shader = Shader::Create(s_SkyboxVertexShader, s_SkyboxFragmentShader);
}

void Skybox::LoadFromFaces(const std::vector<std::string>& facePaths) {
    m_Cubemap = TextureCube::Create(facePaths);
    GINI_INFO("Skybox loaded from 6 face images");
}

void Skybox::LoadFromHDR(const std::string& hdrPath) {
    m_Cubemap = TextureCube::CreateFromHDR(hdrPath);
    GINI_INFO("Skybox loaded from HDR: ", hdrPath);
}

void Skybox::SetCubemap(Ref<TextureCube> cubemap) {
    m_Cubemap = cubemap;
}

void Skybox::Render(const Camera3D& camera) {
    Render(camera.GetViewMatrix(), camera.GetProjectionMatrix());
}

void Skybox::Render(const Mat4& viewMatrix, const Mat4& projectionMatrix) {
    if (!m_Cubemap || !m_Shader) return;
    
    // Remove translation from view matrix (skybox should stay centered on camera)
    glm::mat4 view = glm::mat4(glm::mat3(viewMatrix));
    
    // Disable depth writing but keep depth testing
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    
    m_Shader->Bind();
    m_Shader->SetMat4("u_View", view);
    m_Shader->SetMat4("u_Projection", projectionMatrix);
    m_Shader->SetFloat("u_Intensity", m_Intensity);
    m_Shader->SetFloat("u_Lod", m_Lod);
    m_Shader->SetInt("u_Skybox", 0);
    
    m_Cubemap->Bind(0);
    
    glBindVertexArray(m_CubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
    // Restore depth state
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

Ref<Skybox> Skybox::Create() {
    return CreateRef<Skybox>();
}

Ref<Skybox> Skybox::CreateFromHDR(const std::string& hdrPath) {
    auto skybox = CreateRef<Skybox>();
    skybox->LoadFromHDR(hdrPath);
    return skybox;
}

Ref<Skybox> Skybox::CreateFromFaces(const std::vector<std::string>& facePaths) {
    auto skybox = CreateRef<Skybox>();
    skybox->LoadFromFaces(facePaths);
    return skybox;
}

} // namespace Gini
