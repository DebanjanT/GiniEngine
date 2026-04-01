#include "Shader.h"
#include "Core/Logger.h"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>

namespace Gini {

Shader::Shader(const std::string& vertexSrc, const std::string& fragmentSrc) {
    Compile(vertexSrc, fragmentSrc);
}

Shader::Shader(const std::string& filepath) {
    std::ifstream file(filepath);
    GINI_ASSERT(file.is_open(), "Failed to open shader file: ", filepath);
    
    std::string vertexSrc, fragmentSrc;
    std::string line;
    enum class ShaderType { None, Vertex, Fragment };
    ShaderType type = ShaderType::None;
    
    while (std::getline(file, line)) {
        if (line.find("#type") != std::string::npos) {
            if (line.find("vertex") != std::string::npos)
                type = ShaderType::Vertex;
            else if (line.find("fragment") != std::string::npos)
                type = ShaderType::Fragment;
        } else {
            if (type == ShaderType::Vertex)
                vertexSrc += line + "\n";
            else if (type == ShaderType::Fragment)
                fragmentSrc += line + "\n";
        }
    }
    
    // Extract name from filepath
    auto lastSlash = filepath.find_last_of("/\\");
    auto lastDot = filepath.rfind('.');
    m_Name = filepath.substr(lastSlash + 1, lastDot - lastSlash - 1);
    
    Compile(vertexSrc, fragmentSrc);
}

Shader::~Shader() {
    glDeleteProgram(m_RendererID);
}

void Shader::Compile(const std::string& vertexSrc, const std::string& fragmentSrc) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* source = vertexSrc.c_str();
    glShaderSource(vertexShader, 1, &source, nullptr);
    glCompileShader(vertexShader);
    
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        GINI_ERROR("Vertex shader compilation failed: ", infoLog);
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    source = fragmentSrc.c_str();
    glShaderSource(fragmentShader, 1, &source, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        GINI_ERROR("Fragment shader compilation failed: ", infoLog);
    }
    
    m_RendererID = glCreateProgram();
    glAttachShader(m_RendererID, vertexShader);
    glAttachShader(m_RendererID, fragmentShader);
    glLinkProgram(m_RendererID);
    
    glGetProgramiv(m_RendererID, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(m_RendererID, 512, nullptr, infoLog);
        GINI_ERROR("Shader linking failed: ", infoLog);
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Shader::Bind() const {
    glUseProgram(m_RendererID);
}

void Shader::Unbind() const {
    glUseProgram(0);
}

i32 Shader::GetUniformLocation(const std::string& name) {
    auto it = m_UniformLocationCache.find(name);
    if (it != m_UniformLocationCache.end())
        return it->second;
    
    i32 location = glGetUniformLocation(m_RendererID, name.c_str());
    m_UniformLocationCache[name] = location;
    return location;
}

void Shader::SetInt(const std::string& name, i32 value) {
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetIntArray(const std::string& name, i32* values, u32 count) {
    glUniform1iv(GetUniformLocation(name), count, values);
}

void Shader::SetFloat(const std::string& name, f32 value) {
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetFloat2(const std::string& name, const Vec2& value) {
    glUniform2f(GetUniformLocation(name), value.x, value.y);
}

void Shader::SetFloat3(const std::string& name, const Vec3& value) {
    glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
}

void Shader::SetFloat4(const std::string& name, const Vec4& value) {
    glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
}

void Shader::SetMat3(const std::string& name, const Mat3& value) {
    glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetMat4(const std::string& name, const Mat4& value) {
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

Ref<Shader> Shader::Create(const std::string& vertexSrc, const std::string& fragmentSrc) {
    return CreateRef<Shader>(vertexSrc, fragmentSrc);
}

Ref<Shader> Shader::CreateFromFile(const std::string& filepath) {
    return CreateRef<Shader>(filepath);
}

namespace Shaders {

const char* GetSpriteVertexShader() {
    return R"(
#version 410 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexIndex;

uniform mat4 u_ViewProjection;

out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexIndex;

void main() {
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
    v_TexIndex = a_TexIndex;
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}
)";
}

const char* GetSpriteFragmentShader() {
    return R"(
#version 410 core
layout(location = 0) out vec4 o_Color;

in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;

uniform sampler2D u_Textures[16];

void main() {
    int index = int(v_TexIndex);
    vec4 texColor = texture(u_Textures[index], v_TexCoord);
    o_Color = texColor * v_Color;
}
)";
}

const char* GetTileMapVertexShader() {
    return R"(
#version 410 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in float a_TileIndex;

uniform mat4 u_ViewProjection;

out vec2 v_TexCoord;
out float v_TileIndex;

void main() {
    v_TexCoord = a_TexCoord;
    v_TileIndex = a_TileIndex;
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}
)";
}

const char* GetTileMapFragmentShader() {
    return R"(
#version 410 core
layout(location = 0) out vec4 o_Color;

in vec2 v_TexCoord;
in float v_TileIndex;

uniform sampler2D u_TileAtlas;
uniform vec2 u_TileSize;
uniform vec2 u_AtlasSize;

void main() {
    int tileIndex = int(v_TileIndex);
    int tilesPerRow = int(u_AtlasSize.x / u_TileSize.x);
    
    int tileX = tileIndex % tilesPerRow;
    int tileY = tileIndex / tilesPerRow;
    
    vec2 tileOffset = vec2(tileX, tileY) * u_TileSize / u_AtlasSize;
    vec2 tileUV = v_TexCoord * u_TileSize / u_AtlasSize + tileOffset;
    
    o_Color = texture(u_TileAtlas, tileUV);
}
)";
}

} // namespace Shaders

} // namespace Gini
