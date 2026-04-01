#pragma once

#include "Core/Types.h"
#include "SpriteBatch.h"
#include "Camera.h"

namespace Gini {

class Renderer {
public:
    static void Init();
    static void Shutdown();
    
    static void BeginScene(const Camera& camera);
    static void EndScene();
    
    static void SetClearColor(const Color& color);
    static void Clear();
    static void SetViewport(u32 x, u32 y, u32 width, u32 height);
    
    // 2D Rendering
    static void DrawQuad(const Vec2& position, const Vec2& size, const Color& color);
    static void DrawQuad(const Vec3& position, const Vec2& size, const Color& color);
    static void DrawQuad(const Vec2& position, const Vec2& size, const Ref<Texture2D>& texture,
                         const Color& tint = Color::White());
    static void DrawQuad(const Vec3& position, const Vec2& size, const Ref<Texture2D>& texture,
                         const Color& tint = Color::White());
    
    // Rotated
    static void DrawRotatedQuad(const Vec2& position, const Vec2& size, f32 rotation, const Color& color);
    static void DrawRotatedQuad(const Vec3& position, const Vec2& size, f32 rotation, const Color& color);
    static void DrawRotatedQuad(const Vec2& position, const Vec2& size, f32 rotation,
                                const Ref<Texture2D>& texture, const Color& tint = Color::White());
    
    // Lines and shapes (useful for debug/selection)
    static void DrawLine(const Vec2& start, const Vec2& end, const Color& color, f32 thickness = 1.0f);
    static void DrawRect(const Rect& rect, const Color& color, f32 thickness = 1.0f);
    static void DrawFilledRect(const Rect& rect, const Color& color);
    static void DrawCircle(const Vec2& center, f32 radius, const Color& color, f32 thickness = 1.0f);
    
    // Stats
    static SpriteBatch::Stats GetStats();
    static void ResetStats();
    
private:
    static Scope<SpriteBatch> s_SpriteBatch;
};

} // namespace Gini
