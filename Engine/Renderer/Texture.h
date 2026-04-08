#pragma once

#include "Core/Types.h"
#include <string>
#include <vector>

namespace Gini {

class Texture {
public:
  virtual ~Texture() = default;

  virtual u32 GetWidth() const = 0;
  virtual u32 GetHeight() const = 0;
  virtual u32 GetID() const = 0;

  virtual void Bind(u32 slot = 0) const = 0;
  virtual void Unbind() const = 0;

  virtual bool operator==(const Texture &other) const = 0;
};

class Texture2D : public Texture {
public:
  Texture2D(u32 width, u32 height);
  Texture2D(const std::string &path);
  ~Texture2D() override;

  u32 GetWidth() const override { return m_Width; }
  u32 GetHeight() const override { return m_Height; }
  u32 GetID() const override { return m_RendererID; }
  const std::string &GetPath() const { return m_Path; }

  void SetData(void *data, u32 size);

  void Bind(u32 slot = 0) const override;
  void Unbind() const override;

  bool operator==(const Texture &other) const override {
    return m_RendererID == static_cast<const Texture2D &>(other).m_RendererID;
  }

  static Ref<Texture2D> Create(u32 width, u32 height);
  static Ref<Texture2D> Create(const std::string &path);
  /// Embedded images (e.g. GLB): PNG/JPEG bytes in memory (stb_image).
  static Ref<Texture2D> CreateFromMemory(const unsigned char *data, size_t length);
  /// Uncompressed ARGB8888 from Assimp embedded texture (aiTexel BGRA layout).
  static Ref<Texture2D> CreateFromBGRA(const unsigned char *bgra, u32 width,
                                      u32 height);

private:
  Texture2D(unsigned char *stbiData, int width, int height, int channels);
  Texture2D(const unsigned char *bgra, u32 width, u32 height);
  u32 m_RendererID = 0;
  u32 m_Width = 0;
  u32 m_Height = 0;
  std::string m_Path;
  u32 m_InternalFormat = 0;
  u32 m_DataFormat = 0;
};

// Texture atlas for sprite sheets and tile maps
class TextureAtlas {
public:
  TextureAtlas(Ref<Texture2D> texture, u32 tileWidth, u32 tileHeight);

  Ref<Texture2D> GetTexture() const { return m_Texture; }
  u32 GetTileWidth() const { return m_TileWidth; }
  u32 GetTileHeight() const { return m_TileHeight; }
  u32 GetTilesPerRow() const { return m_TilesPerRow; }
  u32 GetTilesPerColumn() const { return m_TilesPerColumn; }

  // Get UV coordinates for a specific tile
  Rect GetTileUV(u32 tileIndex) const;
  Rect GetTileUV(u32 col, u32 row) const;

private:
  Ref<Texture2D> m_Texture;
  u32 m_TileWidth;
  u32 m_TileHeight;
  u32 m_TilesPerRow;
  u32 m_TilesPerColumn;
};

// Cubemap texture for skyboxes and environment maps
class TextureCube : public Texture {
public:
  // Create from 6 face images (right, left, top, bottom, front, back)
  TextureCube(const std::vector<std::string> &facePaths);
  // Create from single HDR equirectangular image
  TextureCube(const std::string &hdrPath, bool isHDR = true);
  // Create empty cubemap for rendering
  TextureCube(u32 size, bool hdr = false);
  ~TextureCube() override;

  u32 GetWidth() const override { return m_Size; }
  u32 GetHeight() const override { return m_Size; }
  u32 GetID() const override { return m_RendererID; }
  u32 GetSize() const { return m_Size; }
  bool IsHDR() const { return m_IsHDR; }

  void Bind(u32 slot = 0) const override;
  void Unbind() const override;

  bool operator==(const Texture &other) const override {
    return m_RendererID == static_cast<const TextureCube &>(other).m_RendererID;
  }

  static Ref<TextureCube> Create(const std::vector<std::string> &facePaths);
  static Ref<TextureCube> CreateFromHDR(const std::string &hdrPath);
  static Ref<TextureCube> Create(u32 size, bool hdr = false);

private:
  void CreateFromEquirectangular(const std::string &hdrPath);

  u32 m_RendererID = 0;
  u32 m_Size = 0;
  bool m_IsHDR = false;
};

// HDR Texture for environment maps
class TextureHDR {
public:
  TextureHDR(const std::string &path);
  ~TextureHDR();

  u32 GetWidth() const { return m_Width; }
  u32 GetHeight() const { return m_Height; }
  u32 GetID() const { return m_RendererID; }

  void Bind(u32 slot = 0) const;
  void Unbind() const;

  static Ref<TextureHDR> Create(const std::string &path);

private:
  u32 m_RendererID = 0;
  u32 m_Width = 0;
  u32 m_Height = 0;
  std::string m_Path;
};

} // namespace Gini
