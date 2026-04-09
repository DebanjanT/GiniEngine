#pragma once

// Define platform for Diligent Engine
#define PLATFORM_MACOS 1

#include "../Core/Types.h"
#include <memory>
#include <string>

// Diligent Engine includes
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/PipelineState.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Shader.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Sampler.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp"

namespace Gini {

// Forward declarations
class DiligentShader;
class DiligentBuffer;
class DiligentTexture;

// Diligent Engine pipeline wrapper
class DiligentPipeline {
public:
    DiligentPipeline();
    ~DiligentPipeline() = default;

    // Pipeline creation
    bool CreateGraphicsPipeline(const std::string& name, 
                               Diligent::GraphicsPipelineStateCreateInfo& createInfo);
    bool CreateComputePipeline(const std::string& name,
                              Diligent::ComputePipelineStateCreateInfo& createInfo);

    // Shader management
    void SetVertexShader(Ref<DiligentShader> shader);
    void SetPixelShader(Ref<DiligentShader> shader);
    void SetComputeShader(Ref<DiligentShader> shader);

    // Pipeline state configuration
    void SetInputLayout(Diligent::InputLayoutDesc& layout);
    void SetPrimitiveTopology(Diligent::PRIMITIVE_TOPOLOGY topology);
    void SetRasterizerState(Diligent::RasterizerStateDesc& rasterizer);
    void SetDepthStencilState(Diligent::DepthStencilStateDesc& depthStencil);
    void SetBlendState(Diligent::BlendStateDesc& blend);
    void SetRenderTargetFormats(const Diligent::TEXTURE_FORMAT* formats, u32 count);
    void SetDepthStencilFormat(Diligent::TEXTURE_FORMAT format);
    void SetSampleCount(u32 count);

    // Resource binding
    void BindConstantBuffer(u32 slot, Ref<DiligentBuffer> buffer);
    void BindShaderResource(u32 slot, Ref<DiligentTexture> texture);
    void BindSampler(u32 slot, Diligent::ISampler* sampler);

    // Pipeline operations
    void Bind();
    void Draw(Diligent::DrawAttribs& drawAttribs);
    void DrawIndexed(Diligent::DrawIndexedAttribs& drawAttribs);
    void Dispatch(u32 x, u32 y, u32 z);

    // Getters
    Diligent::IPipelineState* GetPipeline() const { return m_Pipeline; }
    Diligent::IShaderResourceBinding* GetResourceBinding() const { return m_ResourceBinding; }
    bool IsValid() const { return m_Pipeline != nullptr; }

private:
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> m_Pipeline;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> m_ResourceBinding;
    
    // Pipeline configuration
    Diligent::GraphicsPipelineStateCreateInfo m_GraphicsCreateInfo;
    Diligent::ComputePipelineStateCreateInfo m_ComputeCreateInfo;
    bool m_IsGraphicsPipeline = true;
    
    // Resource bindings
    std::vector<Ref<DiligentBuffer>> m_ConstantBuffers;
    std::vector<Ref<DiligentTexture>> m_ShaderResources;
    std::vector<Diligent::RefCntAutoPtr<Diligent::ISampler>> m_Samplers;
};

// Pipeline factory for creating common pipeline types
class DiligentPipelineFactory {
public:
    // Standard rendering pipelines
    static Ref<DiligentPipeline> CreatePBRPipeline();
    static Ref<DiligentPipeline> CreateSkyboxPipeline();
    static Ref<DiligentPipeline> CreateTerrainPipeline();
    static Ref<DiligentPipeline> CreateShadowMapPipeline();
    static Ref<DiligentPipeline> CreatePostProcessPipeline(const std::string& shader);
    
    // Compute pipelines
    static Ref<DiligentPipeline> CreateComputePipeline(const std::string& shader);
    
    // Utility pipelines
    static Ref<DiligentPipeline> CreateDebugPipeline();
    static Ref<DiligentPipeline> CreateWireframePipeline();
};

// Pipeline utilities
namespace DiligentPipelineUtils {
    Diligent::InputLayoutDesc CreateDefaultInputLayout();
    Diligent::InputLayoutDesc CreatePBRInputLayout();
    Diligent::InputLayoutDesc CreateSkyboxInputLayout();
    Diligent::InputLayoutDesc CreateTerrainInputLayout();
    
    Diligent::RasterizerStateDesc CreateDefaultRasterizerState();
    Diligent::RasterizerStateDesc CreateWireframeRasterizerState();
    Diligent::RasterizerStateDesc CreateShadowMapRasterizerState();
    
    Diligent::DepthStencilStateDesc CreateDefaultDepthStencilState();
    Diligent::DepthStencilStateDesc CreateNoDepthTestState();
    Diligent::DepthStencilStateDesc CreateShadowMapDepthState();
    
    Diligent::BlendStateDesc CreateDefaultBlendState();
    Diligent::BlendStateDesc CreateAlphaBlendState();
    Diligent::BlendStateDesc CreateAdditiveBlendState();
    
    Diligent::SamplerDesc CreateDefaultSampler();
    Diligent::SamplerDesc CreateLinearSampler();
    Diligent::SamplerDesc CreateAnisotropicSampler();
    Diligent::SamplerDesc CreateShadowMapSampler();
}

// Pipeline state cache for efficient pipeline management
class DiligentPipelineCache {
public:
    static DiligentPipelineCache& GetInstance();
    
    Ref<DiligentPipeline> GetPipeline(const std::string& name);
    void AddPipeline(const std::string& name, Ref<DiligentPipeline> pipeline);
    void RemovePipeline(const std::string& name);
    void ClearCache();
    
    u32 GetPipelineCount() const { return m_Pipelines.size(); }

private:
    DiligentPipelineCache() = default;
    std::unordered_map<std::string, Ref<DiligentPipeline>> m_Pipelines;
};

} // namespace Gini
