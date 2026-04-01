#include "SpriteBatch.h"
#include "Core/Logger.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Gini {

SpriteBatch::SpriteBatch() {
    m_VertexBufferBase = new SpriteVertex[MaxVertices];
    
    // Create VAO
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);
    
    // Create VBO
    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, MaxVertices * sizeof(SpriteVertex), nullptr, GL_DYNAMIC_DRAW);
    
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), 
                          (const void*)offsetof(SpriteVertex, position));
    // Color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex),
                          (const void*)offsetof(SpriteVertex, color));
    // TexCoord
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex),
                          (const void*)offsetof(SpriteVertex, texCoord));
    // TexIndex
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex),
                          (const void*)offsetof(SpriteVertex, texIndex));
    
    // Create IBO
    u32* indices = new u32[MaxIndices];
    u32 offset = 0;
    for (u32 i = 0; i < MaxIndices; i += 6) {
        indices[i + 0] = offset + 0;
        indices[i + 1] = offset + 1;
        indices[i + 2] = offset + 2;
        indices[i + 3] = offset + 2;
        indices[i + 4] = offset + 3;
        indices[i + 5] = offset + 0;
        offset += 4;
    }
    
    glGenBuffers(1, &m_IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MaxIndices * sizeof(u32), indices, GL_STATIC_DRAW);
    delete[] indices;
    
    // Create white texture
    m_WhiteTexture = Texture2D::Create(1, 1);
    u32 whiteData = 0xFFFFFFFF;
    m_WhiteTexture->SetData(&whiteData, sizeof(u32));
    m_TextureSlots[0] = m_WhiteTexture;
    
    // Create shader
    m_Shader = Shader::Create(Shaders::GetSpriteVertexShader(), Shaders::GetSpriteFragmentShader());
    m_Shader->Bind();
    
    i32 samplers[MaxTextureSlots];
    for (u32 i = 0; i < MaxTextureSlots; i++)
        samplers[i] = i;
    m_Shader->SetIntArray("u_Textures", samplers, MaxTextureSlots);
}

SpriteBatch::~SpriteBatch() {
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_IBO);
    delete[] m_VertexBufferBase;
}

void SpriteBatch::Begin(const Camera& camera) {
    m_ViewProjection = camera.GetViewProjection();
    StartBatch();
}

void SpriteBatch::End() {
    Flush();
}

void SpriteBatch::StartBatch() {
    m_IndexCount = 0;
    m_VertexBufferPtr = m_VertexBufferBase;
    m_TextureSlotIndex = 1;
}

void SpriteBatch::NextBatch() {
    Flush();
    StartBatch();
}

void SpriteBatch::Flush() {
    if (m_IndexCount == 0) return;
    
    u32 dataSize = static_cast<u32>((u8*)m_VertexBufferPtr - (u8*)m_VertexBufferBase);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, m_VertexBufferBase);
    
    // Bind textures
    for (u32 i = 0; i < m_TextureSlotIndex; i++) {
        m_TextureSlots[i]->Bind(i);
    }
    
    m_Shader->Bind();
    m_Shader->SetMat4("u_ViewProjection", m_ViewProjection);
    
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, nullptr);
    
    m_Stats.drawCalls++;
}

f32 SpriteBatch::GetTextureIndex(const Ref<Texture2D>& texture) {
    for (u32 i = 0; i < m_TextureSlotIndex; i++) {
        if (*m_TextureSlots[i] == *texture) {
            return static_cast<f32>(i);
        }
    }
    
    if (m_TextureSlotIndex >= MaxTextureSlots) {
        NextBatch();
    }
    
    m_TextureSlots[m_TextureSlotIndex] = texture;
    return static_cast<f32>(m_TextureSlotIndex++);
}

void SpriteBatch::DrawQuad(const Vec2& position, const Vec2& size, const Color& color) {
    DrawQuad(Vec3(position, 0.0f), size, color);
}

void SpriteBatch::DrawQuad(const Vec3& position, const Vec2& size, const Color& color) {
    if (m_IndexCount >= MaxIndices) {
        NextBatch();
    }
    
    constexpr f32 texIndex = 0.0f; // White texture
    
    Vec4 col = color.ToVec4();
    
    m_VertexBufferPtr->position = position;
    m_VertexBufferPtr->color = col;
    m_VertexBufferPtr->texCoord = {0.0f, 0.0f};
    m_VertexBufferPtr->texIndex = texIndex;
    m_VertexBufferPtr++;
    
    m_VertexBufferPtr->position = {position.x + size.x, position.y, position.z};
    m_VertexBufferPtr->color = col;
    m_VertexBufferPtr->texCoord = {1.0f, 0.0f};
    m_VertexBufferPtr->texIndex = texIndex;
    m_VertexBufferPtr++;
    
    m_VertexBufferPtr->position = {position.x + size.x, position.y + size.y, position.z};
    m_VertexBufferPtr->color = col;
    m_VertexBufferPtr->texCoord = {1.0f, 1.0f};
    m_VertexBufferPtr->texIndex = texIndex;
    m_VertexBufferPtr++;
    
    m_VertexBufferPtr->position = {position.x, position.y + size.y, position.z};
    m_VertexBufferPtr->color = col;
    m_VertexBufferPtr->texCoord = {0.0f, 1.0f};
    m_VertexBufferPtr->texIndex = texIndex;
    m_VertexBufferPtr++;
    
    m_IndexCount += 6;
    m_Stats.quadCount++;
}

void SpriteBatch::DrawQuad(const Vec2& position, const Vec2& size, 
                           const Ref<Texture2D>& texture, const Color& tint) {
    DrawQuad(Vec3(position, 0.0f), size, texture, tint);
}

void SpriteBatch::DrawQuad(const Vec3& position, const Vec2& size,
                           const Ref<Texture2D>& texture, const Color& tint) {
    DrawQuad(position, size, texture, Rect(0, 0, 1, 1), tint);
}

void SpriteBatch::DrawQuad(const Vec3& position, const Vec2& size,
                           const Ref<Texture2D>& texture, const Rect& uvRect, const Color& tint) {
    if (m_IndexCount >= MaxIndices) {
        NextBatch();
    }
    
    f32 texIndex = GetTextureIndex(texture);
    Vec4 col = tint.ToVec4();
    
    m_VertexBufferPtr->position = position;
    m_VertexBufferPtr->color = col;
    m_VertexBufferPtr->texCoord = {uvRect.x, uvRect.y};
    m_VertexBufferPtr->texIndex = texIndex;
    m_VertexBufferPtr++;
    
    m_VertexBufferPtr->position = {position.x + size.x, position.y, position.z};
    m_VertexBufferPtr->color = col;
    m_VertexBufferPtr->texCoord = {uvRect.x + uvRect.width, uvRect.y};
    m_VertexBufferPtr->texIndex = texIndex;
    m_VertexBufferPtr++;
    
    m_VertexBufferPtr->position = {position.x + size.x, position.y + size.y, position.z};
    m_VertexBufferPtr->color = col;
    m_VertexBufferPtr->texCoord = {uvRect.x + uvRect.width, uvRect.y + uvRect.height};
    m_VertexBufferPtr->texIndex = texIndex;
    m_VertexBufferPtr++;
    
    m_VertexBufferPtr->position = {position.x, position.y + size.y, position.z};
    m_VertexBufferPtr->color = col;
    m_VertexBufferPtr->texCoord = {uvRect.x, uvRect.y + uvRect.height};
    m_VertexBufferPtr->texIndex = texIndex;
    m_VertexBufferPtr++;
    
    m_IndexCount += 6;
    m_Stats.quadCount++;
}

void SpriteBatch::DrawRotatedQuad(const Vec2& position, const Vec2& size, f32 rotation, const Color& color) {
    DrawRotatedQuad(Vec3(position, 0.0f), size, rotation, color);
}

void SpriteBatch::DrawRotatedQuad(const Vec3& position, const Vec2& size, f32 rotation, const Color& color) {
    if (m_IndexCount >= MaxIndices) {
        NextBatch();
    }
    
    constexpr f32 texIndex = 0.0f;
    Vec4 col = color.ToVec4();
    
    Mat4 transform = glm::translate(Mat4(1.0f), position)
        * glm::rotate(Mat4(1.0f), rotation, Vec3(0, 0, 1))
        * glm::scale(Mat4(1.0f), Vec3(size, 1.0f));
    
    Vec4 positions[4] = {
        transform * Vec4(-0.5f, -0.5f, 0.0f, 1.0f),
        transform * Vec4( 0.5f, -0.5f, 0.0f, 1.0f),
        transform * Vec4( 0.5f,  0.5f, 0.0f, 1.0f),
        transform * Vec4(-0.5f,  0.5f, 0.0f, 1.0f)
    };
    
    Vec2 texCoords[4] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    
    for (int i = 0; i < 4; i++) {
        m_VertexBufferPtr->position = Vec3(positions[i]);
        m_VertexBufferPtr->color = col;
        m_VertexBufferPtr->texCoord = texCoords[i];
        m_VertexBufferPtr->texIndex = texIndex;
        m_VertexBufferPtr++;
    }
    
    m_IndexCount += 6;
    m_Stats.quadCount++;
}

void SpriteBatch::DrawRotatedQuad(const Vec2& position, const Vec2& size, f32 rotation,
                                  const Ref<Texture2D>& texture, const Color& tint) {
    DrawRotatedQuad(Vec3(position, 0.0f), size, rotation, texture, tint);
}

void SpriteBatch::DrawRotatedQuad(const Vec3& position, const Vec2& size, f32 rotation,
                                  const Ref<Texture2D>& texture, const Color& tint) {
    if (m_IndexCount >= MaxIndices) {
        NextBatch();
    }
    
    f32 texIndex = GetTextureIndex(texture);
    Vec4 col = tint.ToVec4();
    
    Mat4 transform = glm::translate(Mat4(1.0f), position)
        * glm::rotate(Mat4(1.0f), rotation, Vec3(0, 0, 1))
        * glm::scale(Mat4(1.0f), Vec3(size, 1.0f));
    
    Vec4 positions[4] = {
        transform * Vec4(-0.5f, -0.5f, 0.0f, 1.0f),
        transform * Vec4( 0.5f, -0.5f, 0.0f, 1.0f),
        transform * Vec4( 0.5f,  0.5f, 0.0f, 1.0f),
        transform * Vec4(-0.5f,  0.5f, 0.0f, 1.0f)
    };
    
    Vec2 texCoords[4] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    
    for (int i = 0; i < 4; i++) {
        m_VertexBufferPtr->position = Vec3(positions[i]);
        m_VertexBufferPtr->color = col;
        m_VertexBufferPtr->texCoord = texCoords[i];
        m_VertexBufferPtr->texIndex = texIndex;
        m_VertexBufferPtr++;
    }
    
    m_IndexCount += 6;
    m_Stats.quadCount++;
}

void SpriteBatch::ResetStats() {
    m_Stats.drawCalls = 0;
    m_Stats.quadCount = 0;
}

} // namespace Gini
