#pragma once

#include "Core/Types.h"
#include "Renderer/Model.h"
#include <mutex>
#include <string>
#include <unordered_map>

namespace Gini {

class ModelCache {
public:
  static ModelCache &Get() {
    static ModelCache instance;
    return instance;
  }

  Ref<Model> Load(const std::string &filepath);
  void Evict(const std::string &filepath);
  void Clear();
  bool Contains(const std::string &filepath) const;
  size_t Size() const;

private:
  ModelCache() = default;

  mutable std::mutex m_Mutex;
  std::unordered_map<std::string, Ref<Model>> m_Cache;
};

} // namespace Gini
