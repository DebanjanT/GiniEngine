#pragma once

#include "Core/Types.h"
#include "Renderer/Shader.h"

namespace Gini {

class SSAO {
public:
    static void Init(u32 width, u32 height);
    static void Shutdown();
    static void Resize(u32 width, u32 height);

    static void Render(u32 depthTexture, u32 normalTexture,
                       const Mat4& projection, const Mat4& view);

    static u32 GetSSAOTexture();
    static bool IsInitialized();
};

} // namespace Gini
