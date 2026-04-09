#pragma once

#include "Core/Types.h"
#include "Renderer/Shader.h"

namespace Gini {

// Forward declarations for Diligent Engine integration
class DiligentPostProcess;

class PostProcess {
public:
  static void Init();
  static void Shutdown();

  static void Resolve(u32 hdrColorTexture, u32 ssaoTexture = 0,
                      f32 exposure = 1.0f, f32 gamma = 2.2f);

  // Diligent Engine integration methods
  static void CreateDiligentPostProcess();
  static void UpdateDiligentPostProcess();
  static void RegisterWithHybridManager();
  static bool HasDiligentResources() { return s_HasDiligentResources; }

private:
  static void InitQuad();

  // Diligent Engine resources
  static bool s_HasDiligentResources;
};

} // namespace Gini
