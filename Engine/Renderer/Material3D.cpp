#include "Material3D.h"
#include "../Core/Logger.h"
#include "RendererAdapter.h"

namespace Gini {

// MaterialAsset Diligent Engine integration methods
Ref<DiligentMaterial> MaterialAsset::ToDiligentMaterial() const {
  // Diligent Engine not available, return null
  // OpenGL is used as fallback renderer
  return nullptr;
}

void MaterialAsset::FromDiligentMaterial(
    const Ref<DiligentMaterial> &diligentMaterial) {
  // Diligent Engine not available, this is a no-op
  // OpenGL is used as fallback renderer
  if (!diligentMaterial) {
    GINI_ERROR("FromDiligentMaterial called with null material");
    return;
  }

  // Cache the Diligent material for future use when dependencies are resolved
  m_DiligentMaterial = diligentMaterial;
}

void MaterialAsset::RegisterWithHybridManager() {
  // Diligent Engine not available, this is a no-op
  // OpenGL is used as fallback renderer
  // Note: Cannot use shared_from_this as MaterialAsset doesn't inherit from
  // enable_shared_from_this This method should be called from the owner that
  // has the Ref<MaterialAsset>
  GINI_INFO("MaterialAsset using OpenGL fallback renderer: " + m_Name);
}

Ref<DiligentMaterial> MaterialAsset::GetDiligentMaterial() const {
  // Diligent Engine not available, return null
  // OpenGL is used as fallback renderer
  // Return cached Diligent material if available for future use
  if (m_DiligentMaterial) {
    return m_DiligentMaterial;
  }

  return nullptr;
}

} // namespace Gini
