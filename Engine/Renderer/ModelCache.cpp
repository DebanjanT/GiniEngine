#include "ModelCache.h"
#include "Core/Logger.h"
#include <filesystem>

namespace Gini {

Ref<Model> ModelCache::Load(const std::string &filepath) {
  if (filepath.empty())
    return nullptr;

  std::lock_guard<std::mutex> lock(m_Mutex);

  std::filesystem::file_time_type currentWriteTime{};
  bool hasWriteTime = false;
  std::error_code ec;
  if (std::filesystem::exists(filepath, ec) && !ec) {
    currentWriteTime = std::filesystem::last_write_time(filepath, ec);
    hasWriteTime = !ec;
  }

  auto it = m_Cache.find(filepath);
  if (it != m_Cache.end()) {
    auto wtIt = m_FileWriteTimes.find(filepath);
    if (!hasWriteTime || wtIt == m_FileWriteTimes.end() ||
        wtIt->second == currentWriteTime) {
      GINI_DEBUG("ModelCache hit: reusing cached model '", filepath, "'");
      return it->second;
    }

    GINI_INFO("ModelCache: source changed, reloading '", filepath, "'");
    m_Cache.erase(it);
    m_FileWriteTimes.erase(filepath);
  }

  auto model = Model::Create(filepath);
  if (model) {
    m_Cache[filepath] = model;
    if (hasWriteTime) {
      m_FileWriteTimes[filepath] = currentWriteTime;
    }
    GINI_INFO("ModelCache: loaded '", filepath, "' (", m_Cache.size(),
              " cached)");
  }
  return model;
}

void ModelCache::Evict(const std::string &filepath) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Cache.erase(filepath);
  m_FileWriteTimes.erase(filepath);
}

void ModelCache::Clear() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Cache.clear();
  m_FileWriteTimes.clear();
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
