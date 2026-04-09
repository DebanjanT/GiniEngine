#pragma once

// Define platform for Diligent Engine
#define PLATFORM_MACOS 1

#include "../Core/Types.h"
#include <memory>
#include <string>

// Diligent Engine includes
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/TextureView.h"

namespace Gini {

// Diligent Engine texture wrapper
class DiligentTexture {
public:
  DiligentTexture();
  DiligentTexture(Diligent::ITexture *texture);
  ~DiligentTexture() = default;

  // Texture creation
  bool CreateFromFile(const std::string &filepath);
  bool CreateFromMemory(
      u32 width, u32 height, void *data,
      Diligent::TEXTURE_FORMAT format = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB);
  bool CreateCubeFromFiles(const std::string &right, const std::string &left,
                           const std::string &top, const std::string &bottom,
                           const std::string &front, const std::string &back);
  bool CreateRenderTarget(
      u32 width, u32 height,
      Diligent::TEXTURE_FORMAT format = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB);
  bool CreateDepthBuffer(
      u32 width, u32 height,
      Diligent::TEXTURE_FORMAT format = Diligent::TEX_FORMAT_D32_FLOAT);

  // Texture operations
  void UpdateData(const void *data, u32 size, u32 mipLevel = 0);
  void GenerateMipmaps();
  void Bind(u32 slot = 0) const;
  void Unbind() const;

  // Getters
  Diligent::ITexture *GetTexture() const { return m_Texture; }
  Diligent::ITextureView *GetSRV() const { return m_SRV; }
  Diligent::ITextureView *GetRTV() const { return m_RTV; }
  Diligent::ITextureView *GetDSV() const { return m_DSV; }

  u32 GetWidth() const;
  u32 GetHeight() const;
  u32 GetDepth() const;
  u32 GetMipLevels() const;
  Diligent::TEXTURE_FORMAT GetFormat() const;
  Diligent::RESOURCE_DIMENSION GetType() const;

  bool IsValid() const { return m_Texture != nullptr; }
  bool IsRenderTarget() const { return m_RTV != nullptr; }
  bool IsDepthBuffer() const { return m_DSV != nullptr; }

private:
  Diligent::RefCntAutoPtr<Diligent::ITexture> m_Texture;
  Diligent::RefCntAutoPtr<Diligent::ITextureView> m_SRV; // Shader Resource View
  Diligent::RefCntAutoPtr<Diligent::ITextureView> m_RTV; // Render Target View
  Diligent::RefCntAutoPtr<Diligent::ITextureView> m_DSV; // Depth Stencil View

  bool CreateSRV();
  bool CreateRTV();
  bool CreateDSV();
};

// Texture factory for creating common texture types
class DiligentTextureFactory {
public:
  static Ref<DiligentTexture> CreateFromFile(const std::string &filepath);
  static Ref<DiligentTexture> CreateFromMemory(
      u32 width, u32 height, void *data,
      Diligent::TEXTURE_FORMAT format = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB);
  static Ref<DiligentTexture>
  CreateCubeFromFiles(const std::string &right, const std::string &left,
                      const std::string &top, const std::string &bottom,
                      const std::string &front, const std::string &back);
  static Ref<DiligentTexture> CreateRenderTarget(
      u32 width, u32 height,
      Diligent::TEXTURE_FORMAT format = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB);
  static Ref<DiligentTexture> CreateDepthBuffer(
      u32 width, u32 height,
      Diligent::TEXTURE_FORMAT format = Diligent::TEX_FORMAT_D32_FLOAT);
  static Ref<DiligentTexture> CreateWhiteTexture();
  static Ref<DiligentTexture> CreateBlackTexture();
  static Ref<DiligentTexture> CreateNormalTexture();
};

// Texture utilities
namespace DiligentTextureUtils {
Diligent::TEXTURE_FORMAT GetDiligentFormat(u32 channels, bool hdr = false);
u32 GetChannelsFromFormat(Diligent::TEXTURE_FORMAT format);
bool IsHDRFormat(Diligent::TEXTURE_FORMAT format);
bool IsCompressedFormat(Diligent::TEXTURE_FORMAT format);
u32 CalculateMipLevels(u32 width, u32 height);
std::string GetFormatString(Diligent::TEXTURE_FORMAT format);
} // namespace DiligentTextureUtils

} // namespace Gini
