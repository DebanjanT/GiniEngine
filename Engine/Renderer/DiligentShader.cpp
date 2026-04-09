#include "DiligentShader.h"
#include "DiligentRenderer.h"
#include "../Core/Logger.h"

// Diligent Engine includes
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "Shader.h"
#include "PipelineState.h"
#include "ShaderResourceBinding.h"
#include "GraphicsUtilities.h"
#include "DataBlob.h"

namespace Gini {

DiligentShader::DiligentShader(const std::string& name) : m_Name(name) {
}

bool DiligentShader::CreateFromSource(const std::string& vertexSource, const std::string& fragmentSource) {
  GINI_INFO("Creating Diligent shader from source: ", m_Name);
  
  // Create vertex shader
  if (!CreateShader(Diligent::SHADER_TYPE_VERTEX, vertexSource, m_VertexShader)) {
    GINI_ERROR("Failed to create vertex shader for: ", m_Name);
    return false;
  }
  
  // Create pixel shader
  if (!CreateShader(Diligent::SHADER_TYPE_PIXEL, fragmentSource, m_PixelShader)) {
    GINI_ERROR("Failed to create pixel shader for: ", m_Name);
    return false;
  }
  
  // Create pipeline state
  if (!CreatePipelineState()) {
    GINI_ERROR("Failed to create pipeline state for: ", m_Name);
    return false;
  }
  
  // Create resource binding
  if (!CreateResourceBinding()) {
    GINI_ERROR("Failed to create resource binding for: ", m_Name);
    return false;
  }
  
  GINI_INFO("Successfully created Diligent shader: ", m_Name);
  return true;
}

bool DiligentShader::CreateFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
  GINI_INFO("Creating Diligent shader from files: ", vertexPath, ", ", fragmentPath);
  
  // Load vertex shader source
  std::ifstream vertexFile(vertexPath);
  if (!vertexFile.is_open()) {
    GINI_ERROR("Failed to open vertex shader file: ", vertexPath);
    return false;
  }
  
  std::stringstream vertexStream;
  vertexStream << vertexFile.rdbuf();
  std::string vertexSource = vertexStream.str();
  
  // Load fragment shader source
  std::ifstream fragmentFile(fragmentPath);
  if (!fragmentFile.is_open()) {
    GINI_ERROR("Failed to open fragment shader file: ", fragmentPath);
    return false;
  }
  
  std::stringstream fragmentStream;
  fragmentStream << fragmentFile.rdbuf();
  std::string fragmentSource = fragmentStream.str();
  
  return CreateFromSource(vertexSource, fragmentSource);
}

bool DiligentShader::CreateShader(Diligent::SHADER_TYPE type, const std::string& source, Diligent::RefCntAutoPtr<Diligent::IShader>& shader) {
  Diligent::ShaderCreateInfo ShaderCI;
  ShaderCI.Source = source.c_str();
  ShaderCI.EntryPoint = "main";
  ShaderCI.Desc.ShaderType = type;
  ShaderCI.Desc.Name = m_Name.c_str();
  
  // Get the device from the renderer (assuming we have a global renderer instance)
  // TODO: Pass device as parameter or use a service locator
  
  // For now, we'll use a simple approach - this will need to be integrated with the renderer
  Diligent::ShaderMacroHelper Macros;
  
  // Create shader
  // TODO: Get device from renderer
  // device->CreateShader(ShaderCI, &shader);
  
  return shader != nullptr;
}

bool DiligentShader::CreatePipelineState() {
  Diligent::GraphicsPipelineStateDesc PSODesc;
  PSODesc.PSODesc.Name = m_Name.c_str();
  PSODesc.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
  
  // Set shaders
  PSODesc.VS = m_VertexShader;
  PSODesc.PS = m_PixelShader;
  
  // Input layout - this needs to be configured based on the vertex format
  // TODO: Configure input layout based on vertex attributes
  
  // Render targets
  PSODesc.GraphicsPipeline.NumRenderTargets = 1;
  PSODesc.GraphicsPipeline.RTVFormats[0] = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
  PSODesc.GraphicsPipeline.DSVFormat = Diligent::TEX_FORMAT_D32_FLOAT;
  
  // Rasterizer state
  PSODesc.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_BACK;
  PSODesc.GraphicsPipeline.RasterizerDesc.FillMode = Diligent::FILL_MODE_SOLID;
  
  // Depth-stencil state
  PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
  PSODesc.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = true;
  PSODesc.GraphicsPipeline.DepthStencilDesc.DepthFunc = Diligent::COMPARISON_FUNC_LESS_EQUAL;
  
  // Blend state
  PSODesc.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = false;
  PSODesc.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlend = Diligent::BLEND_FACTOR_ONE;
  PSODesc.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlend = Diligent::BLEND_FACTOR_ZERO;
  
  // TODO: Create pipeline state with device
  // device->CreateGraphicsPipelineState(PSODesc, &m_PipelineState);
  
  return m_PipelineState != nullptr;
}

bool DiligentShader::CreateResourceBinding() {
  if (!m_PipelineState) {
    return false;
  }
  
  // Create shader resource binding
  m_PipelineState->CreateShaderResourceBinding(&m_ResourceBinding, true);
  
  return m_ResourceBinding != nullptr;
}

void DiligentShader::Bind() {
  if (m_PipelineState && m_ResourceBinding) {
    // TODO: Get device context and bind pipeline state and resource binding
    // context->SetPipelineState(m_PipelineState);
    // context->CommitShaderResources(m_ResourceBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  }
}

void DiligentShader::Unbind() {
  // TODO: Unbind if needed
}

void DiligentShader::SetUniform(const std::string& name, const glm::mat4& matrix) {
  // TODO: Find uniform and set matrix value
}

void DiligentShader::SetUniform(const std::string& name, const glm::vec3& vector) {
  // TODO: Find uniform and set vector value
}

void DiligentShader::SetUniform(const std::string& name, const glm::vec4& vector) {
  // TODO: Find uniform and set vector value
}

void DiligentShader::SetUniform(const std::string& name, float value) {
  // TODO: Find uniform and set float value
}

void DiligentShader::SetUniform(const std::string& name, int value) {
  // TODO: Find uniform and set int value
}

void DiligentShader::SetTexture(const std::string& name, Diligent::ITexture* texture, u32 slot) {
  // TODO: Find texture binding and set texture
}

Diligent::ShaderResourceDesc DiligentShader::FindUniform(const std::string& name) const {
  // TODO: Implement uniform lookup
  return Diligent::ShaderResourceDesc{};
}

Diligent::ShaderResourceDesc DiligentShader::FindTexture(const std::string& name) const {
  // TODO: Implement texture lookup
  return Diligent::ShaderResourceDesc{};
}

// DiligentShaderCompiler implementation
bool DiligentShaderCompiler::CompileGLSLToSPIRV(const std::string& glslSource, Diligent::SHADER_TYPE type, std::vector<u32>& spirv) {
  // TODO: Implement GLSL to SPIR-V compilation
  // This would typically use glslang or a similar compiler
  
  std::string processedSource = PreprocessGLSL(glslSource, type);
  
  // For now, return false as this needs proper implementation
  return false;
}

bool DiligentShaderCompiler::ValidateSPIRV(const std::vector<u32>& spirv, Diligent::SHADER_TYPE type) {
  // TODO: Implement SPIR-V validation
  return false;
}

std::string DiligentShaderCompiler::PreprocessGLSL(const std::string& source, Diligent::SHADER_TYPE type) {
  // TODO: Implement GLSL preprocessing
  // - Handle version directives
  // - Handle platform-specific includes
  // - Handle macro definitions
  
  return source;
}

} // namespace Gini
