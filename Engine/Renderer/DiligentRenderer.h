#pragma once

// Define platform for Diligent Engine
#define PLATFORM_MACOS 1

#include "../Core/Types.h"
#include "Camera3D.h"
#include "Framebuffer.h"
#include "Material3D.h"
#include "Model3D.h"
#include <memory>
#include <vector>

// Diligent Engine includes
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Common/interface/BasicMath.hpp"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Sampler.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/SwapChain.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"

namespace Gini {

// Diligent Engine wrapper for Gini Engine
class DiligentRenderer {
public:
  DiligentRenderer();
  ~DiligentRenderer();

  // Initialization
  bool Initialize(void *nativeWindowHandle, u32 width, u32 height);
  void Shutdown();

  // Frame management
  void BeginFrame();
  void EndFrame();
  void Present();

  // Rendering commands
  void BeginScene(const Camera3D &camera);
  void EndScene();

  // Drawing methods
  void DrawModel3D(const Ref<Model3D> &model, const glm::mat4 &transform);
  void DrawMesh3D(const Ref<Mesh3D> &mesh, const Ref<MaterialAsset> &material,
                  const glm::mat4 &transform);
  void DrawSkybox(const Ref<TextureCube> &skybox);
  void DrawTerrain(const class Terrain *terrain);

  // Resource management
  Ref<class DiligentTexture> CreateTexture(const std::string &filepath);
  Ref<class DiligentTexture> CreateTexture(u32 width, u32 height, void *data);
  Ref<class DiligentShader> CreateShader(const std::string &vertexSource,
                                         const std::string &fragmentSource);
  Ref<class DiligentBuffer> CreateVertexBuffer(const void *data, u32 size);
  Ref<class DiligentBuffer> CreateIndexBuffer(const void *data, u32 size);

  // Getters
  Diligent::IRenderDevice *GetDevice() const { return m_Device; }
  Diligent::IDeviceContext *GetContext() const { return m_Context; }
  Diligent::ISwapChain *GetSwapChain() const { return m_SwapChain; }

  // Window management
  void Resize(u32 width, u32 height);
  u32 GetWidth() const { return m_Width; }
  u32 GetHeight() const { return m_Height; }

private:
  // Diligent Engine core objects
  Diligent::IRenderDevice *m_Device = nullptr;
  Diligent::IDeviceContext *m_Context = nullptr;
  Diligent::ISwapChain *m_SwapChain = nullptr;

  // Engine state
  u32 m_Width = 0;
  u32 m_Height = 0;
  bool m_IsInitialized = false;

  // Current frame state
  Camera3D *m_CurrentCamera = nullptr;
  glm::mat4 m_ViewMatrix;
  glm::mat4 m_ProjectionMatrix;

  // Default pipeline states
  Diligent::RefCntAutoPtr<Diligent::IPipelineState> m_DefaultPipeline;
  Diligent::RefCntAutoPtr<Diligent::IPipelineState> m_PBRPipeline;
  Diligent::RefCntAutoPtr<Diligent::IPipelineState> m_SkyboxPipeline;
  Diligent::RefCntAutoPtr<Diligent::IPipelineState> m_TerrainPipeline;

  // Resource pools
  std::vector<Ref<class DiligentTexture>> m_Textures;
  std::vector<Ref<class DiligentShader>> m_Shaders;
  std::vector<Ref<class DiligentBuffer>> m_Buffers;

  // Initialization helpers
  bool CreateDeviceAndSwapChain(void *nativeWindowHandle);
  bool CreateDefaultPipelines();
  void SetupRenderTargets();

  // Pipeline creation helpers
  Diligent::RefCntAutoPtr<Diligent::IPipelineState>
  CreateGraphicsPipeline(const std::string &vertexShader,
                         const std::string &fragmentShader,
                         const Diligent::PipelineStateDesc &desc);
};

// Diligent Engine resource wrappers
class DiligentTexture {
public:
  DiligentTexture(Diligent::ITexture *texture);
  ~DiligentTexture() = default;

  Diligent::ITexture *GetTexture() const { return m_Texture; }
  Diligent::ITextureView *GetSRV() const { return m_SRV; }

  u32 GetWidth() const;
  u32 GetHeight() const;
  Diligent::TEXTURE_FORMAT GetFormat() const;

private:
  Diligent::RefCntAutoPtr<Diligent::ITexture> m_Texture;
  Diligent::RefCntAutoPtr<Diligent::ITextureView> m_SRV;
};

class DiligentShader {
public:
  DiligentShader(Diligent::IShader *vertexShader,
                 Diligent::IShader *pixelShader);
  ~DiligentShader() = default;

  Diligent::IShader *GetVertexShader() const { return m_VertexShader; }
  Diligent::IShader *GetPixelShader() const { return m_PixelShader; }

private:
  Diligent::RefCntAutoPtr<Diligent::IShader> m_VertexShader;
  Diligent::RefCntAutoPtr<Diligent::IShader> m_PixelShader;
};

class DiligentBuffer {
public:
  DiligentBuffer(Diligent::IBuffer *buffer);
  ~DiligentBuffer() = default;

  Diligent::IBuffer *GetBuffer() const { return m_Buffer; }

  void UpdateData(const void *data, u32 size, u32 offset = 0);

private:
  Diligent::RefCntAutoPtr<Diligent::IBuffer> m_Buffer;
};

} // namespace Gini
