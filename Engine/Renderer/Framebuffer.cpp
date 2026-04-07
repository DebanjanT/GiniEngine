#include "Framebuffer.h"
#include "Core/Logger.h"

#include <glad/gl.h>

namespace Gini {

static GLenum ToGLInternalFormat(FramebufferTextureFormat fmt) {
    switch (fmt) {
        case FramebufferTextureFormat::RGBA8:    return GL_RGBA8;
        case FramebufferTextureFormat::RGBA16F:  return GL_RGBA16F;
        case FramebufferTextureFormat::RGB16F:   return GL_RGB16F;
        case FramebufferTextureFormat::RED:      return GL_R8;
        default: return GL_RGBA8;
    }
}

static GLenum ToGLFormat(FramebufferTextureFormat fmt) {
    switch (fmt) {
        case FramebufferTextureFormat::RGBA8:
        case FramebufferTextureFormat::RGBA16F:  return GL_RGBA;
        case FramebufferTextureFormat::RGB16F:   return GL_RGB;
        case FramebufferTextureFormat::RED:      return GL_RED;
        default: return GL_RGBA;
    }
}

static GLenum ToGLType(FramebufferTextureFormat fmt) {
    switch (fmt) {
        case FramebufferTextureFormat::RGBA16F:
        case FramebufferTextureFormat::RGB16F:   return GL_FLOAT;
        default: return GL_UNSIGNED_BYTE;
    }
}

Framebuffer::Framebuffer(const FramebufferSpec& spec) : m_Spec(spec) {
    Invalidate();
}

Framebuffer::~Framebuffer() {
    glDeleteFramebuffers(1, &m_RendererID);
    for (auto id : m_ColorAttachments)
        glDeleteTextures(1, &id);
    if (m_DepthAttachment)
        glDeleteTextures(1, &m_DepthAttachment);
}

void Framebuffer::Invalidate() {
    if (m_RendererID) {
        glDeleteFramebuffers(1, &m_RendererID);
        for (auto id : m_ColorAttachments)
            glDeleteTextures(1, &id);
        if (m_DepthAttachment)
            glDeleteTextures(1, &m_DepthAttachment);
        m_ColorAttachments.clear();
        m_DepthAttachment = 0;
    }
    
    glGenFramebuffers(1, &m_RendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    
    m_ColorAttachments.resize(m_Spec.colorAttachments.size());
    
    for (u32 i = 0; i < m_Spec.colorAttachments.size(); i++) {
        auto& attachSpec = m_Spec.colorAttachments[i];
        
        glGenTextures(1, &m_ColorAttachments[i]);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachments[i]);
        
        GLenum internalFormat = ToGLInternalFormat(attachSpec.format);
        GLenum format = ToGLFormat(attachSpec.format);
        GLenum type = ToGLType(attachSpec.format);
        
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Spec.width, m_Spec.height, 0,
                     format, type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
                               m_ColorAttachments[i], 0);
    }
    
    if (m_Spec.colorAttachments.size() > 1) {
        std::vector<GLenum> drawBuffers;
        for (u32 i = 0; i < m_Spec.colorAttachments.size(); i++)
            drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
        glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
    }
    
    if (m_Spec.hasDepth) {
        glGenTextures(1, &m_DepthAttachment);
        glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);
        
        if (m_Spec.depthFormat == FramebufferTextureFormat::DepthComponent) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_Spec.width, m_Spec.height, 0,
                         GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment, 0);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_Spec.width, m_Spec.height, 0,
                         GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment, 0);
        }
    }
    
    GINI_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    glViewport(0, 0, m_Spec.width, m_Spec.height);
}

void Framebuffer::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Resize(u32 width, u32 height) {
    if (width == 0 || height == 0 || width > 8192 || height > 8192) {
        GINI_WARN("Invalid framebuffer size: ", width, "x", height);
        return;
    }
    
    m_Spec.width = width;
    m_Spec.height = height;
    Invalidate();
}

u32 Framebuffer::GetColorAttachment(u32 index) const {
    if (index < m_ColorAttachments.size())
        return m_ColorAttachments[index];
    return 0;
}

Ref<Framebuffer> Framebuffer::Create(const FramebufferSpec& spec) {
    return CreateRef<Framebuffer>(spec);
}

} // namespace Gini
