#pragma once

#include "Core/Types.h"
#include "Texture.h"
#include "Shader.h"
#include "Camera.h"
#include <array>

namespace Gini {

struct SpriteVertex {
    Vec3 position;
    Vec4 color;
    Vec2 texCoord;
    f32 texIndex;
};

class SpriteBatch {
public:
    static constexpr u32 MaxQuads = 10000;
    static constexpr u32 MaxVertices = MaxQuads * 4;
    static constexpr u32 MaxIndices = MaxQuads * 6;
    static constexpr u32 MaxTextureSlots = 16;
    
    SpriteBatch();
    ~SpriteBatch();
    
    void Begin(const Camera& camera);
    void End();
    void Flush();
    
    // Draw sprites
    void DrawQuad(const Vec2& position, const Vec2& size, const Color& color);
    void DrawQuad(const Vec3& position, const Vec2& size, const Color& color);
    void DrawQuad(const Vec2& position, const Vec2& size, const Ref<Texture2D>& texture, 
                  const Color& tint = Color::White());
    void DrawQuad(const Vec3& position, const Vec2& size, const Ref<Texture2D>& texture,
                  const Color& tint = Color::White());
    void DrawQuad(const Vec3& position, const Vec2& size, const Ref<Texture2D>& texture,
                  const Rect& uvRect, const Color& tint = Color::White());
    
    // Rotated sprites
    void DrawRotatedQuad(const Vec2& position, const Vec2& size, f32 rotation, const Color& color);
    void DrawRotatedQuad(const Vec3& position, const Vec2& size, f32 rotation, const Color& color);
    void DrawRotatedQuad(const Vec2& position, const Vec2& size, f32 rotation,
                         const Ref<Texture2D>& texture, const Color& tint = Color::White());
    void DrawRotatedQuad(const Vec3& position, const Vec2& size, f32 rotation,
                         const Ref<Texture2D>& texture, const Color& tint = Color::White());
    
    // Statistics
    struct Stats {
        u32 drawCalls = 0;
        u32 quadCount = 0;
        u32 GetVertexCount() const { return quadCount * 4; }
        u32 GetIndexCount() const { return quadCount * 6; }
    };
    
    void ResetStats();
    Stats GetStats() const { return m_Stats; }
    
private:
    void StartBatch();
    void NextBatch();
    f32 GetTextureIndex(const Ref<Texture2D>& texture);
    
    u32 m_VAO = 0;
    u32 m_VBO = 0;
    u32 m_IBO = 0;
    
    Ref<Shader> m_Shader;
    Ref<Texture2D> m_WhiteTexture;
    
    std::array<Ref<Texture2D>, MaxTextureSlots> m_TextureSlots;
    u32 m_TextureSlotIndex = 1; // 0 = white texture
    
    SpriteVertex* m_VertexBufferBase = nullptr;
    SpriteVertex* m_VertexBufferPtr = nullptr;
    u32 m_IndexCount = 0;
    
    Mat4 m_ViewProjection{1.0f};
    Stats m_Stats;
};

} // namespace Gini
