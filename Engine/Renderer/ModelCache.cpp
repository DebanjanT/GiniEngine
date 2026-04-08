#include "ModelCache.h"
#include "Core/Logger.h"

namespace Gini {

Ref<Model> ModelCache::Load(const std::string &filepath) {
  if (filepath.empty())
    return nullptr;

  std::lock_guard<std::mutex> lock(m_Mutex);

  auto it = m_Cache.find(filepath);
  if (it != m_Cache.end()) {
    return it->second;
  }

  auto model = Model::Create(filepath);
  if (model) {
    m_Cache[filepath] = model;
    GINI_INFO("ModelCache: loaded '", filepath, "' (", m_Cache.size(),
              " cached)");
  }
  return model;
}

void ModelCache::Evict(const std::string &filepath) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Cache.erase(filepath);
}

void ModelCache::Clear() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Cache.clear();
  GINI_INFO("ModelCache: cleared");
}

bool ModelCache::Contains(const std::string &filepath) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_Cache.count(filepath) > 0;
}

size_t ModelCache::Size() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_Cache.size();
}

} // namespace Gini
