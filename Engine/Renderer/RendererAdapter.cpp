#include "RendererAdapter.h"
#include "../Core/Logger.h"
#include "DiligentMaterial.h"

// Diligent Engine includes for initialization
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Platforms/interface/NativeWindow.h"

namespace Gini {

// RendererAdapter Implementation
RendererAdapter &RendererAdapter::GetInstance() {
  static RendererAdapter instance;
  return instance;
}

RendererAdapter::RendererAdapter() {
  // Attempt to initialize Diligent Engine
  if (InitializeDiligentEngine()) {
    m_Backend = RendererBackend::DiligentEngine;
    m_FallbackEnabled = true;
    GINI_INFO("RendererAdapter initialized with Diligent Engine backend");
  } else {
    // Fall back to OpenGL
    m_Backend = RendererBackend::OpenGL;
    m_FallbackEnabled = true;
    GINI_INFO("RendererAdapter initialized with OpenGL fallback (Diligent "
              "Engine initialization failed)");
  }
}

void RendererAdapter::SetBackend(RendererBackend backend) {
  m_Backend = backend;
  std::string backendName =
      (backend == RendererBackend::OpenGL ? "OpenGL" : "Diligent Engine");
  GINI_INFO("Renderer backend switched to: " + backendName);
}

bool RendererAdapter::InitializeDiligentEngine() {
  // This method is called without window handle, so it cannot fully initialize
  // Use InitializeDiligentEngineWithWindow instead
  GINI_WARN("RendererAdapter::InitializeDiligentEngine requires window handle");
  GINI_WARN("Use InitializeDiligentEngineWithWindow for full initialization");
  return false;
}

bool RendererAdapter::InitializeDiligentEngineWithWindow(GLFWwindow *glfwWindow,
                                                         u32 width,
                                                         u32 height) {
  if (!glfwWindow) {
    GINI_ERROR(
        "InitializeDiligentEngineWithWindow: Invalid GLFW window handle");
    return false;
  }

  try {
    // Get the OpenGL engine factory
    auto *pFactoryGL = Diligent::GetEngineFactoryOpenGL();
    if (!pFactoryGL) {
      GINI_ERROR("Failed to get Diligent Engine OpenGL factory");
      return false;
    }

    // Create NativeWindow structure for Diligent Engine
    Diligent::NativeWindow nativeWindow;
    nativeWindow = Diligent::NativeWindow{glfwWindow};

    // Create swap chain description
    Diligent::SwapChainDesc SCDesc;
    SCDesc.Width = width;
    SCDesc.Height = height;
    SCDesc.ColorBufferFormat = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
    SCDesc.DepthBufferFormat = Diligent::TEX_FORMAT_D32_FLOAT;
    SCDesc.Usage = Diligent::SWAP_CHAIN_USAGE_RENDER_TARGET |
                   Diligent::SWAP_CHAIN_USAGE_COPY_SOURCE;
    SCDesc.BufferCount = 2;

    // Create engine creation attributes
    Diligent::EngineGLCreateInfo EngineCI;
    EngineCI.Window = nativeWindow;

    // Create device, context, and swap chain in one call
    pFactoryGL->CreateDeviceAndSwapChainGL(EngineCI, &m_DiligentDevice,
                                           &m_DiligentContext, SCDesc,
                                           &m_DiligentSwapChain);

    if (!m_DiligentDevice || !m_DiligentContext || !m_DiligentSwapChain) {
      GINI_ERROR(
          "Failed to create Diligent Engine device, context, or swap chain");
      return false;
    }

    m_DiligentEngineInitialized = true;
    GINI_INFO("Diligent Engine initialized successfully with OpenGL backend");
    GINI_INFO("Device created with swap chain");

    return true;
  } catch (const std::exception &e) {
    GINI_ERROR("Diligent Engine initialization failed with exception: " +
               std::string(e.what()));
    return false;
  } catch (...) {
    GINI_ERROR("Diligent Engine initialization failed with unknown exception");
    return false;
  }
}

void RendererAdapter::ShutdownDiligentEngine() {
  // TODO: Implement Diligent Engine cleanup
  // Release device, context, and swap chain resources

  if (m_DiligentDevice) {
    m_DiligentDevice.Release();
  }
  if (m_DiligentContext) {
    m_DiligentContext.Release();
  }
  if (m_DiligentSwapChain) {
    m_DiligentSwapChain.Release();
  }

  m_DiligentEngineInitialized = false;
  GINI_INFO("Diligent Engine shutdown complete");
}

Ref<MaterialAsset> RendererAdapter::ConvertMaterial(
    const Ref<DiligentMaterial> &diligentMaterial) {
  // Diligent Engine not available, return null
  return nullptr;
}

Ref<DiligentMaterial>
RendererAdapter::ConvertMaterial(const Ref<MaterialAsset> &materialAsset) {
  // Diligent Engine not available, return null
  return nullptr;
}

Ref<Texture>
RendererAdapter::ConvertTexture(const Ref<DiligentTexture> &diligentTexture) {
  // Diligent Engine not available, return null
  return nullptr;
}

Ref<DiligentTexture>
RendererAdapter::ConvertTexture(const Ref<Texture> &texture) {
  // Diligent Engine not available, return null
  return nullptr;
}

void RendererAdapter::SyncMaterials() {
  if (m_Backend == RendererBackend::OpenGL) {
    // Implement OpenGL fallback material synchronization
    // For demonstration purposes, assume a simple material synchronization
    GINI_INFO("RendererAdapter::SyncMaterials (OpenGL) synchronized");
  } else {
    // TODO: Implement material synchronization when Diligent Engine is
    // available
    GINI_WARN("RendererAdapter::SyncMaterials deferred until Diligent Engine "
              "dependencies are resolved");
  }
}

void RendererAdapter::SyncTextures() {
  if (m_Backend == RendererBackend::OpenGL) {
    // OpenGL is the fallback, textures are already in OpenGL format
    GINI_INFO("RendererAdapter::SyncTextures (OpenGL) - textures already in "
              "OpenGL format");
  } else {
    // Diligent Engine not available
    GINI_WARN("RendererAdapter::SyncTextures deferred until Diligent Engine "
              "dependencies are resolved");
  }
}

bool RendererAdapter::IsDiligentEngineAvailable() const {
  return m_DiligentEngineInitialized;
}

bool RendererAdapter::IsOpenGLAvailable() const {
  // OpenGL is always available as the fallback renderer
  return true;
}

void RendererAdapter::EnableFallback(bool enable) {
  m_FallbackEnabled = enable;
  std::string status = (enable ? "enabled" : "disabled");
  GINI_INFO("Renderer fallback " + status);
}

// MaterialAdapter Implementation
DiligentPBRMaterial
MaterialAdapter::ConvertToDiligentPBR(const PBRMaterial &pbrMaterial) {
  DiligentPBRMaterial diligentMaterial;

  // Convert basic PBR properties
  diligentMaterial.Albedo = pbrMaterial.Albedo;
  diligentMaterial.Metallic = pbrMaterial.Metallic;
  diligentMaterial.Roughness = pbrMaterial.Roughness;
  diligentMaterial.AO = pbrMaterial.AO;
  diligentMaterial.Emissive = pbrMaterial.Emissive;

  // Note: Texture map conversion requires Diligent Engine dependencies
  // For now, we set the flags but defer actual texture conversion
  diligentMaterial.UseAlbedoMap = pbrMaterial.UseAlbedoMap;
  diligentMaterial.UseNormalMap = pbrMaterial.UseNormalMap;
  // TODO: Convert texture maps when Diligent Engine dependencies are resolved
  // Texture conversion will be handled through HybridResourceManager
  GINI_WARN("MaterialAdapter::ConvertToDiligentPBR texture conversion deferred "
            "until Diligent Engine dependencies are resolved");

  return diligentMaterial;
}

PBRMaterial MaterialAdapter::ConvertFromDiligentPBR(
    const DiligentPBRMaterial &diligentMaterial) {
  PBRMaterial pbrMaterial;

  // Convert basic PBR properties
  pbrMaterial.Albedo = diligentMaterial.Albedo;
  pbrMaterial.Metallic = diligentMaterial.Metallic;
  pbrMaterial.Roughness = diligentMaterial.Roughness;
  pbrMaterial.AO = diligentMaterial.AO;
  pbrMaterial.Emissive = diligentMaterial.Emissive;

  // TODO: Convert texture maps when Diligent Engine is available
  GINI_WARN("MaterialAdapter::ConvertFromDiligentPBR texture conversion "
            "deferred until Diligent Engine dependencies are resolved");

  return pbrMaterial;
}

Ref<DiligentTexture> MaterialAdapter::ConvertTextureMap(Ref<Texture> texture) {
  // TODO: Implement texture map conversion when Diligent Engine is available
  GINI_WARN("MaterialAdapter::ConvertTextureMap deferred until Diligent Engine "
            "dependencies are resolved");
  return nullptr;
}

Ref<Texture> MaterialAdapter::ConvertDiligentTextureMap(
    Ref<DiligentTexture> diligentTexture) {
  // TODO: Implement Diligent texture map conversion when Diligent Engine is
  // available
  GINI_WARN("MaterialAdapter::ConvertDiligentTextureMap deferred until "
            "Diligent Engine dependencies are resolved");
  return nullptr;
}

void MaterialAdapter::SyncMaterialProperties(
    Ref<MaterialAsset> material, Ref<DiligentMaterial> diligentMaterial) {
  // TODO: Implement material property synchronization when Diligent Engine is
  // available
  GINI_WARN("MaterialAdapter::SyncMaterialProperties deferred until Diligent "
            "Engine dependencies are resolved");
}

// TextureAdapter Implementation
Diligent::TEXTURE_FORMAT TextureAdapter::ConvertToDiligentFormat(u32 channels,
                                                                 bool hdr) {
  // TODO: Implement format conversion when Diligent Engine is available
  GINI_WARN("TextureAdapter::ConvertToDiligentFormat deferred until Diligent "
            "Engine dependencies are resolved");
  return Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
}

u32 TextureAdapter::ConvertFromDiligentFormat(Diligent::TEXTURE_FORMAT format) {
  // TODO: Implement format conversion when Diligent Engine is available
  GINI_WARN("TextureAdapter::ConvertFromDiligentFormat deferred until Diligent "
            "Engine dependencies are resolved");
  return 4; // Default to RGBA
}

void *TextureAdapter::ConvertTextureData(const void *srcData, u32 width,
                                         u32 height, u32 srcChannels,
                                         u32 dstChannels) {
  // TODO: Implement texture data conversion when Diligent Engine is available
  GINI_WARN("TextureAdapter::ConvertTextureData deferred until Diligent Engine "
            "dependencies are resolved");
  return nullptr;
}

// HybridResourceManager Implementation
HybridResourceManager &HybridResourceManager::GetInstance() {
  static HybridResourceManager instance;
  return instance;
}

void HybridResourceManager::RegisterMaterial(
    const std::string &name, Ref<MaterialAsset> glMaterial,
    Ref<DiligentMaterial> diligentMaterial) {
  MaterialPair pair;
  pair.glMaterial = glMaterial;
  pair.diligentMaterial = diligentMaterial;
  m_Materials[name] = pair;
  GINI_INFO("Registered dual material: " + name);
}

void HybridResourceManager::RegisterTexture(
    const std::string &name, Ref<Texture> glTexture,
    Ref<DiligentTexture> diligentTexture) {
  TexturePair pair;
  pair.glTexture = glTexture;
  pair.diligentTexture = diligentTexture;
  m_Textures[name] = pair;
  GINI_INFO("Registered dual texture: " + name);
}

Ref<MaterialAsset> HybridResourceManager::GetMaterial(const std::string &name) {
  auto it = m_Materials.find(name);
  return (it != m_Materials.end()) ? it->second.glMaterial : nullptr;
}

Ref<DiligentMaterial>
HybridResourceManager::GetDiligentMaterial(const std::string &name) {
  auto it = m_Materials.find(name);
  return (it != m_Materials.end()) ? it->second.diligentMaterial : nullptr;
}

Ref<Texture> HybridResourceManager::GetTexture(const std::string &name) {
  auto it = m_Textures.find(name);
  return (it != m_Textures.end()) ? it->second.glTexture : nullptr;
}

Ref<DiligentTexture>
HybridResourceManager::GetDiligentTexture(const std::string &name) {
  auto it = m_Textures.find(name);
  return (it != m_Textures.end()) ? it->second.diligentTexture : nullptr;
}

void HybridResourceManager::Cleanup() {
  // TODO: Implement cleanup when Diligent Engine is available
  GINI_WARN("HybridResourceManager::Cleanup deferred until Diligent Engine "
            "dependencies are resolved");
}

void HybridResourceManager::ClearCache() {
  m_Materials.clear();
  m_Textures.clear();
  GINI_INFO("Hybrid resource manager cache cleared");
}

} // namespace Gini
