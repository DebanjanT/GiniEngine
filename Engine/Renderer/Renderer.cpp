#include "Renderer.h"
#include "Core/Logger.h"

#include <glad/gl.h>

namespace Gini {

Scope<SpriteBatch> Renderer::s_SpriteBatch = nullptr;

void Renderer::Init() {
    GINI_INFO("Initializing Renderer");
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    s_SpriteBatch = CreateScope<SpriteBatch>();
}

void Renderer::Shutdown() {
    s_SpriteBatch.reset();
}

void Renderer::BeginScene(const Camera& camera) {
    s_SpriteBatch->Begin(camera);
}

void Renderer::EndScene() {
    s_SpriteBatch->End();
}

void Renderer::SetClearColor(const Color& color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void Renderer::Clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetViewport(u32 x, u32 y, u32 width, u32 height) {
    glViewport(x, y, width, height);
}

void Renderer::DrawQuad(const Vec2& position, const Vec2& size, const Color& color) {
    s_SpriteBatch->DrawQuad(position, size, color);
}

void Renderer::DrawQuad(const Vec3& position, const Vec2& size, const Color& color) {
    s_SpriteBatch->DrawQuad(position, size, color);
}

void Renderer::DrawQuad(const Vec2& position, const Vec2& size, 
                        const Ref<Texture2D>& texture, const Color& tint) {
    s_SpriteBatch->DrawQuad(position, size, texture, tint);
}

void Renderer::DrawQuad(const Vec3& position, const Vec2& size,
                        const Ref<Texture2D>& texture, const Color& tint) {
    s_SpriteBatch->DrawQuad(position, size, texture, tint);
}

void Renderer::DrawRotatedQuad(const Vec2& position, const Vec2& size, f32 rotation, const Color& color) {
    s_SpriteBatch->DrawRotatedQuad(position, size, rotation, color);
}

void Renderer::DrawRotatedQuad(const Vec3& position, const Vec2& size, f32 rotation, const Color& color) {
    s_SpriteBatch->DrawRotatedQuad(position, size, rotation, color);
}

void Renderer::DrawRotatedQuad(const Vec2& position, const Vec2& size, f32 rotation,
                               const Ref<Texture2D>& texture, const Color& tint) {
    s_SpriteBatch->DrawRotatedQuad(position, size, rotation, texture, tint);
}

void Renderer::DrawLine(const Vec2& start, const Vec2& end, const Color& color, f32 thickness) {
    Vec2 dir = end - start;
    f32 length = glm::length(dir);
    f32 angle = std::atan2(dir.y, dir.x);
    Vec2 center = (start + end) * 0.5f;
    s_SpriteBatch->DrawRotatedQuad(center, Vec2(length, thickness), angle, color);
}

void Renderer::DrawRect(const Rect& rect, const Color& color, f32 thickness) {
    Vec2 topLeft(rect.x, rect.y + rect.height);
    Vec2 topRight(rect.x + rect.width, rect.y + rect.height);
    Vec2 bottomLeft(rect.x, rect.y);
    Vec2 bottomRight(rect.x + rect.width, rect.y);
    
    DrawLine(topLeft, topRight, color, thickness);
    DrawLine(topRight, bottomRight, color, thickness);
    DrawLine(bottomRight, bottomLeft, color, thickness);
    DrawLine(bottomLeft, topLeft, color, thickness);
}

void Renderer::DrawFilledRect(const Rect& rect, const Color& color) {
    s_SpriteBatch->DrawQuad(Vec2(rect.x, rect.y), Vec2(rect.width, rect.height), color);
}

void Renderer::DrawCircle(const Vec2& center, f32 radius, const Color& color, f32 thickness) {
    constexpr int segments = 32;
    f32 angleStep = 2.0f * 3.14159265f / segments;
    
    for (int i = 0; i < segments; i++) {
        f32 angle1 = i * angleStep;
        f32 angle2 = (i + 1) * angleStep;
        
        Vec2 p1 = center + Vec2(std::cos(angle1), std::sin(angle1)) * radius;
        Vec2 p2 = center + Vec2(std::cos(angle2), std::sin(angle2)) * radius;
        
        DrawLine(p1, p2, color, thickness);
    }
}

SpriteBatch::Stats Renderer::GetStats() {
    return s_SpriteBatch->GetStats();
}

void Renderer::ResetStats() {
    s_SpriteBatch->ResetStats();
}

} // namespace Gini
