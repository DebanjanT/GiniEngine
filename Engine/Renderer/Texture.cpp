#include "Texture.h"
#include "Core/Logger.h"

#include <glad/gl.h>
#include <stb_image.h>

namespace Gini {

Texture2D::Texture2D(u32 width, u32 height) : m_Width(width), m_Height(height) {
    m_InternalFormat = GL_RGBA8;
    m_DataFormat = GL_RGBA;
    
    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, width, height, 0, m_DataFormat, GL_UNSIGNED_BYTE, nullptr);
}

Texture2D::Texture2D(const std::string& path) : m_Path(path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        GINI_ERROR("Failed to load texture: ", path);
        return;
    }
    
    m_Width = width;
    m_Height = height;
    
    if (channels == 4) {
        m_InternalFormat = GL_RGBA8;
        m_DataFormat = GL_RGBA;
    } else if (channels == 3) {
        m_InternalFormat = GL_RGB8;
        m_DataFormat = GL_RGB;
    } else if (channels == 1) {
        m_InternalFormat = GL_R8;
        m_DataFormat = GL_RED;
    }
    
    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, width, height, 0, m_DataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    stbi_image_free(data);
    
    GINI_DEBUG("Loaded texture: ", path, " (", width, "x", height, ", ", channels, " channels)");
}

Texture2D::~Texture2D() {
    glDeleteTextures(1, &m_RendererID);
}

void Texture2D::SetData(void* data, u32 size) {
    u32 bpp = m_DataFormat == GL_RGBA ? 4 : 3;
    GINI_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
}

void Texture2D::Bind(u32 slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
}

void Texture2D::Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

Ref<Texture2D> Texture2D::Create(u32 width, u32 height) {
    return CreateRef<Texture2D>(width, height);
}

Ref<Texture2D> Texture2D::Create(const std::string& path) {
    return CreateRef<Texture2D>(path);
}

// TextureAtlas
TextureAtlas::TextureAtlas(Ref<Texture2D> texture, u32 tileWidth, u32 tileHeight)
    : m_Texture(texture), m_TileWidth(tileWidth), m_TileHeight(tileHeight) {
    m_TilesPerRow = texture->GetWidth() / tileWidth;
    m_TilesPerColumn = texture->GetHeight() / tileHeight;
}

Rect TextureAtlas::GetTileUV(u32 tileIndex) const {
    u32 col = tileIndex % m_TilesPerRow;
    u32 row = tileIndex / m_TilesPerRow;
    return GetTileUV(col, row);
}

Rect TextureAtlas::GetTileUV(u32 col, u32 row) const {
    f32 texWidth = static_cast<f32>(m_Texture->GetWidth());
    f32 texHeight = static_cast<f32>(m_Texture->GetHeight());
    
    f32 u = (col * m_TileWidth) / texWidth;
    f32 v = (row * m_TileHeight) / texHeight;
    f32 uWidth = m_TileWidth / texWidth;
    f32 vHeight = m_TileHeight / texHeight;
    
    return Rect(u, v, uWidth, vHeight);
}

} // namespace Gini
