#pragma once

#include "Core/Types.h"
#include <vector>

namespace Gini {

enum class FramebufferTextureFormat {
    RGBA8,
    RGBA16F,
    RGB16F,
    RED,
    Depth24Stencil8,
    DepthComponent
};

struct FramebufferTextureSpec {
    FramebufferTextureFormat format = FramebufferTextureFormat::RGBA8;
    FramebufferTextureSpec() = default;
    FramebufferTextureSpec(FramebufferTextureFormat fmt) : format(fmt) {}
};

struct FramebufferSpec {
    u32 width = 1280;
    u32 height = 720;
    u32 samples = 1;
    bool swapChainTarget = false;
    std::vector<FramebufferTextureSpec> colorAttachments = {
        {FramebufferTextureFormat::RGBA8}
    };
    bool hasDepth = true;
    FramebufferTextureFormat depthFormat = FramebufferTextureFormat::Depth24Stencil8;
};

class Framebuffer {
public:
    Framebuffer(const FramebufferSpec& spec);
    ~Framebuffer();
    
    void Bind();
    void Unbind();
    void Resize(u32 width, u32 height);
    
    u32 GetColorAttachment(u32 index = 0) const;
    u32 GetDepthAttachment() const { return m_DepthAttachment; }
    u32 GetColorAttachmentCount() const { return static_cast<u32>(m_ColorAttachments.size()); }
    const FramebufferSpec& GetSpec() const { return m_Spec; }
    
    static Ref<Framebuffer> Create(const FramebufferSpec& spec);
    
private:
    void Invalidate();
    
    u32 m_RendererID = 0;
    std::vector<u32> m_ColorAttachments;
    u32 m_DepthAttachment = 0;
    FramebufferSpec m_Spec;
};

} // namespace Gini
