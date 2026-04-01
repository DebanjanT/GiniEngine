#pragma once

#include "Core/Types.h"
#include <string>
#include <unordered_map>

namespace Gini {

class Shader {
public:
  Shader(const std::string &vertexSrc, const std::string &fragmentSrc);
  Shader(const std::string &filepath);
  ~Shader();

  void Bind() const;
  void Unbind() const;

  // Uniforms
  void SetInt(const std::string &name, i32 value);
  void SetIntArray(const std::string &name, i32 *values, u32 count);
  void SetFloat(const std::string &name, f32 value);
  void SetFloat2(const std::string &name, const Vec2 &value);
  void SetFloat3(const std::string &name, const Vec3 &value);
  void SetFloat4(const std::string &name, const Vec4 &value);
  void SetMat3(const std::string &name, const Mat3 &value);
  void SetMat4(const std::string &name, const Mat4 &value);

  // Aliases for convenience
  void SetVec2(const std::string &name, const Vec2 &value) {
    SetFloat2(name, value);
  }
  void SetVec3(const std::string &name, const Vec3 &value) {
    SetFloat3(name, value);
  }
  void SetVec4(const std::string &name, const Vec4 &value) {
    SetFloat4(name, value);
  }

  u32 GetID() const { return m_RendererID; }
  const std::string &GetName() const { return m_Name; }

  static Ref<Shader> Create(const std::string &vertexSrc,
                            const std::string &fragmentSrc);
  static Ref<Shader> CreateFromFile(const std::string &filepath);

private:
  void Compile(const std::string &vertexSrc, const std::string &fragmentSrc);
  i32 GetUniformLocation(const std::string &name);

  u32 m_RendererID = 0;
  std::string m_Name;
  mutable std::unordered_map<std::string, i32> m_UniformLocationCache;
};

// Built-in shaders for RTS
namespace Shaders {
const char *GetSpriteVertexShader();
const char *GetSpriteFragmentShader();
const char *GetTileMapVertexShader();
const char *GetTileMapFragmentShader();
} // namespace Shaders

} // namespace Gini
