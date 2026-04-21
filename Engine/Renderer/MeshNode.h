#pragma once

#include "Core/Types.h"
#include <string>
#include <vector>

namespace Gini {

struct MeshNode {
  u32 Parent = 0xFFFFFFFF;
  std::vector<u32> Submeshes;
  std::vector<u32> Children;
  
  std::string Name;
  Mat4 LocalTransform{1.0f};
};

} // namespace Gini
