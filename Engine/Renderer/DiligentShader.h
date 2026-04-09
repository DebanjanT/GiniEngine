#pragma once

// Define platform for Diligent Engine
#define PLATFORM_MACOS 1

#include "../Core/Types.h"
#include <memory>
#include <string>
#include <vector>

// Diligent Engine includes
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/PipelineState.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Shader.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"

namespace Gini {

// Diligent Engine shader wrapper
class DiligentShader {
public:
  DiligentShader(const std::string &name);
  ~DiligentShader() = default;

  // Shader creation from source
  bool CreateFromSource(const std::string &vertexSource,
                        const std::string &fragmentSource);
  bool CreateFromFiles(const std::string &vertexPath,
                       const std::string &fragmentPath);

  // Pipeline state management
  Diligent::IPipelineState *GetPipelineState() const { return m_PipelineState; }
  Diligent::IShaderResourceBinding *GetResourceBinding() const {
    return m_ResourceBinding;
  }

  // Shader uniforms
  void SetUniform(const std::string &name, const glm::mat4 &matrix);
  void SetUniform(const std::string &name, const glm::vec3 &vector);
  void SetUniform(const std::string &name, const glm::vec4 &vector);
  void SetUniform(const std::string &name, float value);
  void SetUniform(const std::string &name, int value);

  // Texture binding
  void SetTexture(const std::string &name, Diligent::ITexture *texture,
                  u32 slot = 0);

  // Bind/Unbind
  void Bind();
  void Unbind();

  // Getters
  const std::string &GetName() const { return m_Name; }
  bool IsValid() const { return m_PipelineState != nullptr; }

private:
  std::string m_Name;

  // Diligent Engine objects
  Diligent::RefCntAutoPtr<Diligent::IPipelineState> m_PipelineState;
  Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> m_ResourceBinding;
  Diligent::RefCntAutoPtr<Diligent::IShader> m_VertexShader;
  Diligent::RefCntAutoPtr<Diligent::IShader> m_PixelShader;

  // Shader creation helpers
  bool CreateShader(Diligent::SHADER_TYPE type, const std::string &source,
                    Diligent::RefCntAutoPtr<Diligent::IShader> &shader);
  bool CreatePipelineState();
  bool CreateResourceBinding();

  // Uniform helpers
  Diligent::ShaderResourceDesc FindUniform(const std::string &name) const;
  Diligent::ShaderResourceDesc FindTexture(const std::string &name) const;
};

// Diligent Engine shader compiler for SPIR-V
class DiligentShaderCompiler {
public:
  static bool CompileGLSLToSPIRV(const std::string &glslSource,
                                 Diligent::SHADER_TYPE type,
                                 std::vector<u32> &spirv);
  static bool ValidateSPIRV(const std::vector<u32> &spirv,
                            Diligent::SHADER_TYPE type);

private:
  static std::string PreprocessGLSL(const std::string &source,
                                    Diligent::SHADER_TYPE type);
};

} // namespace Gini
