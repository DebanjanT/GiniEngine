#include "DiligentRenderer.h"
#include "../Core/Logger.h"
#include "Terrain/Terrain.h"

// Diligent Engine includes
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Common/interface/BasicMath.hpp"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/PipelineState.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Sampler.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Shader.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/SwapChain.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsTools/interface/GraphicsUtilities.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Platforms/interface/NativeWindow.h"

namespace Gini {

DiligentRenderer::DiligentRenderer() = default;

DiligentRenderer::~DiligentRenderer() { Shutdown(); }

bool DiligentRenderer::Initialize(void *nativeWindowHandle, u32 width,
                                  u32 height) {
  m_Width = width;
  m_Height = height;

  GINI_INFO("Initializing Diligent Engine renderer...");

  // Create device and swap chain
  if (!CreateDeviceAndSwapChain(nativeWindowHandle)) {
    GINI_ERROR("Failed to create Diligent Engine device and swap chain");
    return false;
  }

  // Create default pipeline states
  if (!CreateDefaultPipelines()) {
    GINI_ERROR("Failed to create default pipeline states");
    return false;
  }

  // Setup render targets
  SetupRenderTargets();

  m_IsInitialized = true;
  GINI_INFO("Diligent Engine renderer initialized successfully");
  return true;
}

void DiligentRenderer::Shutdown() {
  if (m_IsInitialized) {
    GINI_INFO("Shutting down Diligent Engine renderer...");

    // Release pipeline states
    m_DefaultPipeline.Release();
    m_PBRPipeline.Release();
    m_SkyboxPipeline.Release();
    m_TerrainPipeline.Release();

    // Release core objects
    if (m_SwapChain) {
      m_SwapChain->Release();
      m_SwapChain = nullptr;
    }
    if (m_Context) {
      m_Context->Release();
      m_Context = nullptr;
    }
    if (m_Device) {
      m_Device->Release();
      m_Device = nullptr;
    }

    m_IsInitialized = false;
    GINI_INFO("Diligent Engine renderer shutdown complete");
  }
}

void DiligentRenderer::BeginFrame() {
  if (!m_IsInitialized)
    return;

  // Set default render target
  Diligent::ITextureView *pRTV = m_SwapChain->GetCurrentBackBufferRTV();
  Diligent::ITextureView *pDSV = m_SwapChain->GetDepthBufferDSV();
  m_Context->SetRenderTargets(
      1, &pRTV, pDSV, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Clear render target
  const float ClearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};
  m_Context->ClearRenderTarget(
      pRTV, ClearColor, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
  m_Context->ClearDepthStencil(
      pDSV, Diligent::CLEAR_DEPTH_FLAG, 1.0f, 0,
      Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

  // Set viewport
  Diligent::Viewport viewport;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = static_cast<float>(m_Width);
  viewport.Height = static_cast<float>(m_Height);
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  m_Context->SetViewports(1, &viewport, static_cast<Diligent::Uint32>(m_Width),
                          static_cast<Diligent::Uint32>(m_Height));
}

void DiligentRenderer::EndFrame() {
  if (!m_IsInitialized)
    return;

  // Reset state
  m_Context->SetPipelineState(nullptr);
  m_Context->CommitShaderResources(
      nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void DiligentRenderer::Present() {
  if (!m_IsInitialized)
    return;

  m_SwapChain->Present();
}

void DiligentRenderer::BeginScene(const Camera3D &camera) {
  if (!m_IsInitialized)
    return;

  m_CurrentCamera = const_cast<Camera3D *>(&camera);
  m_ViewMatrix = camera.GetViewMatrix();
  m_ProjectionMatrix = camera.GetProjectionMatrix();
}

void DiligentRenderer::EndScene() {
  if (!m_IsInitialized)
    return;

  m_CurrentCamera = nullptr;
}

void DiligentRenderer::DrawModel3D(const Ref<Model3D> &model,
                                   const glm::mat4 &transform) {
  if (!m_IsInitialized || !model)
    return;

  // Get meshes from model
  const auto &meshes = model->GetMeshes();

  for (const auto &mesh : meshes) {
    DrawMesh3D(mesh, model->GetMaterial(0),
               transform); // TODO: Handle multiple materials
  }
}

void DiligentRenderer::DrawMesh3D(const Ref<Mesh3D> &mesh,
                                  const Ref<MaterialAsset> &material,
                                  const glm::mat4 &transform) {
  if (!m_IsInitialized || !mesh || !material)
    return;

  // Set PBR pipeline
  m_Context->SetPipelineState(m_PBRPipeline);

  // TODO: Set shader resources (material, transforms, etc.)

  // TODO: Draw mesh
  // m_Context->DrawIndexed(...)
}

void DiligentRenderer::DrawSkybox(const Ref<TextureCube> &skybox) {
  if (!m_IsInitialized || !skybox)
    return;

  // Set skybox pipeline
  m_Context->SetPipelineState(m_SkyboxPipeline);

  // TODO: Set skybox texture and draw
}

void DiligentRenderer::DrawTerrain(const Terrain *terrain) {
  if (!m_IsInitialized || !terrain)
    return;

  // Set terrain pipeline
  m_Context->SetPipelineState(m_TerrainPipeline);

  // TODO: Draw terrain
}

void DiligentRenderer::Resize(u32 width, u32 height) {
  if (!m_IsInitialized)
    return;

  m_Width = width;
  m_Height = height;

  // Resize swap chain
  m_SwapChain->Resize(width, height);
}

bool DiligentRenderer::CreateDeviceAndSwapChain(void *nativeWindowHandle) {
  // Create Vulkan engine factory
  Diligent::RefCntAutoPtr<Diligent::IEngineFactory> pEngineFactory;
  pEngineFactory = Diligent::GetEngineFactoryVk();

  if (!pEngineFactory) {
    GINI_ERROR("Failed to get Vulkan engine factory");
    return false;
  }

  // Engine creation description
  Diligent::EngineVkCreateInfo EngineCI;
  EngineCI.GraphicsAPIVersion = Diligent::Version{1, 0};
  EngineCI.EnableValidation = true; // Enable validation layer for debugging

  // Create device and context
  Diligent::GetEngineFactoryVk()->CreateDeviceAndContextsVk(EngineCI, &m_Device,
                                                            &m_Context);

  if (!m_Device || !m_Context) {
    GINI_ERROR("Failed to create Diligent Engine device and context");
    return false;
  }

  // Create swap chain
  Diligent::SwapChainDesc SCDesc;
  SCDesc.Width = m_Width;
  SCDesc.Height = m_Height;
  SCDesc.ColorBufferFormat = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
  SCDesc.DepthBufferFormat = Diligent::TEX_FORMAT_D32_FLOAT;

  // Native window handle for macOS
  Diligent::MacOSNativeWindow Window(nativeWindowHandle);

  Diligent::GetEngineFactoryVk()->CreateSwapChainVk(m_Device, m_Context, SCDesc,
                                                    Window, &m_SwapChain);

  if (!m_SwapChain) {
    GINI_ERROR("Failed to create Diligent Engine swap chain");
    return false;
  }

  return true;
}

bool DiligentRenderer::CreateDefaultPipelines() {
  // Create PBR pipeline
  Diligent::GraphicsPipelineStateCreateInfo PSOCreateInfo("PBR Pipeline");

  // TODO: Configure pipeline state
  m_Device->CreateGraphicsPipelineState(PSOCreateInfo, &m_PBRPipeline);

  if (!m_PBRPipeline) {
    GINI_ERROR("Failed to create PBR pipeline");
    return false;
  }

  // TODO: Create other pipelines (skybox, terrain, etc.)

  return true;
}

void DiligentRenderer::SetupRenderTargets() {
  // Setup will be handled by BeginFrame for now
}

// DiligentTexture implementation
DiligentTexture::DiligentTexture(Diligent::ITexture *texture)
    : m_Texture(texture) {
  if (m_Texture) {
    Diligent::TextureViewDesc SRVDesc;
    SRVDesc.ViewType = Diligent::TEXTURE_VIEW_SHADER_RESOURCE;
    SRVDesc.Format = m_Texture->GetDesc().Format;
    m_Texture->CreateView(SRVDesc, &m_SRV);
  }
}

u32 DiligentTexture::GetWidth() const {
  return m_Texture ? m_Texture->GetDesc().Width : 0;
}

u32 DiligentTexture::GetHeight() const {
  return m_Texture ? m_Texture->GetDesc().Height : 0;
}

Diligent::TEXTURE_FORMAT DiligentTexture::GetFormat() const {
  return m_Texture ? m_Texture->GetDesc().Format : Diligent::TEX_FORMAT_UNKNOWN;
}

// DiligentShader implementation
DiligentShader::DiligentShader(Diligent::IShader *vertexShader,
                               Diligent::IShader *pixelShader)
    : m_VertexShader(vertexShader), m_PixelShader(pixelShader) {}

// DiligentBuffer implementation
DiligentBuffer::DiligentBuffer(Diligent::IBuffer *buffer) : m_Buffer(buffer) {}

void DiligentBuffer::UpdateData(const void *data, u32 size, u32 offset) {
  if (m_Buffer && data) {
    // Get the immediate context from the device
    Diligent::IDeviceContext *context = nullptr;
    // TODO: Get context from renderer or pass it as parameter
    // For now, this will need to be implemented when we have proper context
    // management

    if (context) {
      context->UpdateBuffer(
          m_Buffer, 0, size, data,
          Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
  }
}

} // namespace Gini
