#pragma once

#include "Core/Types.h"
#include "Renderer/Shader.h"
#include "Renderer/Camera3D.h"
#include "Renderer/Light.h"
#include <vector>

namespace Gini {

class ShadowMap {
public:
    static constexpr u32 SHADOW_MAP_SIZE = 2048;
    static constexpr u32 NUM_CASCADES = 3;

    static void Init();
    static void Shutdown();

    static void BeginShadowPass(const Camera3D& camera, const DirectionalLight& light);
    static void EndShadowPass();

    static void BindShadowMaps(Shader* shader, u32 startTextureUnit);

    static u32 GetShadowMapTexture();
    static const std::vector<Mat4>& GetLightSpaceMatrices();
    static const std::vector<f32>& GetCascadeSplits();
    static bool IsInitialized();

    static Ref<Shader> GetDepthShader();
};

} // namespace Gini
