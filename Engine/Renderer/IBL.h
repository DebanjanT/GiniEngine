#pragma once

#include "Core/Types.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture.h"

namespace Gini {

class AtmosphericSky;

class IBL {
public:
    static void Init();
    static void Shutdown();

    static void GenerateBRDFLUT();
    static void GenerateDefaultIBL();
    static void GenerateFromCubemap(u32 envCubemap, u32 size = 512);
    static void CaptureAtmosphericSky(AtmosphericSky* sky, const class Camera3D& camera);

    static u32 GetIrradianceMap();
    static u32 GetPrefilterMap();
    static u32 GetBRDFLUT();
    static f32 GetMaxReflectionLod();

    static void BindIBLTextures(Shader* shader, u32 startTextureUnit);

    static bool IsReady();
};

} // namespace Gini
