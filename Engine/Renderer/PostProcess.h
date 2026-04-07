#pragma once

#include "Core/Types.h"
#include "Renderer/Shader.h"

namespace Gini {

class PostProcess {
public:
    static void Init();
    static void Shutdown();

    static void Resolve(u32 hdrColorTexture, u32 ssaoTexture = 0,
                        f32 exposure = 1.0f, f32 gamma = 2.2f);

private:
    static void InitQuad();
};

} // namespace Gini
