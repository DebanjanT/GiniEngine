#pragma once

#include "Core/Types.h"
#include <string>

namespace Gini {

struct AABB {
  Vec3 Min{0.0f};
  Vec3 Max{0.0f};
};

struct Submesh {
  u32 BaseVertex = 0;
  u32 BaseIndex = 0;
  u32 MaterialIndex = 0;
  u32 VertexCount = 0;
  u32 IndexCount = 0;
  
  Mat4 Transform{1.0f};
  Mat4 LocalTransform{1.0f};
  AABB BoundingBox;
  
  std::string MeshName;
  bool IsRigged = false;
};

} // namespace Gini
