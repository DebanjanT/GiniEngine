#pragma once

#include "Core/Types.h"

namespace Gini {

struct FramebufferSpec {
    u32 width = 1280;
    u32 height = 720;
    u32 samples = 1;
    bool swapChainTarget = false;
};

class Framebuffer {
public:
    Framebuffer(const FramebufferSpec& spec);
    ~Framebuffer();
    
    void Bind();
    void Unbind();
    void Resize(u32 width, u32 height);
    
    u32 GetColorAttachment() const { return m_ColorAttachment; }
    u32 GetDepthAttachment() const { return m_DepthAttachment; }
    const FramebufferSpec& GetSpec() const { return m_Spec; }
    
    static Ref<Framebuffer> Create(const FramebufferSpec& spec);
    
private:
    void Invalidate();
    
    u32 m_RendererID = 0;
    u32 m_ColorAttachment = 0;
    u32 m_DepthAttachment = 0;
    FramebufferSpec m_Spec;
};

} // namespace Gini
