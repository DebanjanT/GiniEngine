#pragma once

// Define platform for Diligent Engine
#define PLATFORM_MACOS 1

#include "../Core/Types.h"
#include "../Renderer/Material3D.h"
#include "../Renderer/Texture.h"
#include "DiligentMaterial.h"
#include "DiligentTexture.h"

// Forward declarations
struct GLFWwindow;

// Diligent Engine includes
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/EngineFactory.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/SwapChain.h"

namespace Gini {

// Renderer backend type
enum class RendererBackend { OpenGL, DiligentEngine };

// Adapter class to bridge between OpenGL and Diligent Engine rendering systems
class RendererAdapter {
public:
  static RendererAdapter &GetInstance();

  // Backend management
  void SetBackend(RendererBackend backend);
  RendererBackend GetBackend() const { return m_Backend; }

  // Material conversion
  Ref<MaterialAsset>
  ConvertMaterial(const Ref<DiligentMaterial> &diligentMaterial);
  Ref<DiligentMaterial>
  ConvertMaterial(const Ref<MaterialAsset> &materialAsset);

  // Texture conversion
  Ref<Texture> ConvertTexture(const Ref<DiligentTexture> &diligentTexture);
  Ref<DiligentTexture> ConvertTexture(const Ref<Texture> &texture);

  // Resource synchronization
  void SyncMaterials();
  void SyncTextures();

  // Compatibility checks
  bool IsDiligentEngineAvailable() const;
  bool IsOpenGLAvailable() const;

  // Fallback management
  void EnableFallback(bool enable);
  bool IsFallbackEnabled() const { return m_FallbackEnabled; }

  // Diligent Engine initialization with window
  bool InitializeDiligentEngineWithWindow(GLFWwindow *glfwWindow, u32 width,
                                          u32 height);

private:
  RendererAdapter();

  // Diligent Engine initialization
  bool InitializeDiligentEngine();
  void ShutdownDiligentEngine();

  RendererBackend m_Backend = RendererBackend::OpenGL;
  bool m_FallbackEnabled = true;
  bool m_DiligentEngineInitialized = false;

  // Diligent Engine resources
  Diligent::RefCntAutoPtr<Diligent::IRenderDevice> m_DiligentDevice;
  Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_DiligentContext;
  Diligent::RefCntAutoPtr<Diligent::ISwapChain> m_DiligentSwapChain;

  // Conversion caches
  std::unordered_map<u64, Ref<DiligentMaterial>> m_MaterialToDiligentCache;
  std::unordered_map<u64, Ref<MaterialAsset>> m_DiligentToMaterialCache;
  std::unordered_map<u64, Ref<DiligentTexture>> m_TextureToDiligentCache;
  std::unordered_map<u64, Ref<Texture>> m_DiligentToTextureCache;
};

// Material adapter for seamless integration
class MaterialAdapter {
public:
  // Convert PBR material properties between systems
  static DiligentPBRMaterial
  ConvertToDiligentPBR(const PBRMaterial &pbrMaterial);
  static PBRMaterial
  ConvertFromDiligentPBR(const DiligentPBRMaterial &diligentMaterial);

  // Texture map conversion
  static Ref<DiligentTexture> ConvertTextureMap(Ref<Texture> texture);
  static Ref<Texture>
  ConvertDiligentTextureMap(Ref<DiligentTexture> diligentTexture);

  // Material property synchronization
  static void SyncMaterialProperties(Ref<MaterialAsset> material,
                                     Ref<DiligentMaterial> diligentMaterial);
};

// Texture adapter for seamless integration
class TextureAdapter {
public:
  // Texture format conversion
  static Diligent::TEXTURE_FORMAT ConvertToDiligentFormat(u32 channels,
                                                          bool hdr = false);
  static u32 ConvertFromDiligentFormat(Diligent::TEXTURE_FORMAT format);

  // Texture data conversion
  static void *ConvertTextureData(const void *srcData, u32 width, u32 height,
                                  u32 srcChannels, u32 dstChannels);
};

// Resource manager for dual-backend support
class HybridResourceManager {
public:
  static HybridResourceManager &GetInstance();

  // Dual resource management
  void RegisterMaterial(const std::string &name, Ref<MaterialAsset> glMaterial,
                        Ref<DiligentMaterial> diligentMaterial);
  void RegisterTexture(const std::string &name, Ref<Texture> glTexture,
                       Ref<DiligentTexture> diligentTexture);

  // Resource retrieval based on current backend
  Ref<MaterialAsset> GetMaterial(const std::string &name);
  Ref<DiligentMaterial> GetDiligentMaterial(const std::string &name);
  Ref<Texture> GetTexture(const std::string &name);
  Ref<DiligentTexture> GetDiligentTexture(const std::string &name);

  // Resource cleanup
  void Cleanup();
  void ClearCache();

private:
  HybridResourceManager() = default;

  struct MaterialPair {
    Ref<MaterialAsset> glMaterial;
    Ref<DiligentMaterial> diligentMaterial;
  };

  struct TexturePair {
    Ref<Texture> glTexture;
    Ref<DiligentTexture> diligentTexture;
  };

  std::unordered_map<std::string, MaterialPair> m_Materials;
  std::unordered_map<std::string, TexturePair> m_Textures;
};

} // namespace Gini
