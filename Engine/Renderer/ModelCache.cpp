#include "ModelCache.h"
#include "Core/Logger.h"
#include <filesystem>

namespace Gini {

Ref<Model3D> ModelCache::LoadModel3D(const std::string &filepath) {
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

  auto it = m_Model3DCache.find(filepath);
  if (it != m_Model3DCache.end()) {
    auto wtIt = m_FileWriteTimes.find(filepath);
    if (!hasWriteTime || wtIt == m_FileWriteTimes.end() ||
        wtIt->second == currentWriteTime) {
      GINI_DEBUG("ModelCache hit: reusing cached Model3D '", filepath, "'");
      return it->second;
    }

    GINI_INFO("ModelCache: source changed, reloading Model3D '", filepath, "'");
    m_Model3DCache.erase(it);
    m_FileWriteTimes.erase(filepath);
  }

  auto model = Model3D::Create(filepath);
  if (model && model->IsValid()) {
    m_Model3DCache[filepath] = model;
    if (hasWriteTime) {
      m_FileWriteTimes[filepath] = currentWriteTime;
    }
    GINI_INFO("ModelCache: loaded Model3D '", filepath, "' (",
              m_Model3DCache.size(), " cached)");
  }
  return model;
}

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

  auto it = m_LegacyCache.find(filepath);
  if (it != m_LegacyCache.end()) {
    auto wtIt = m_FileWriteTimes.find(filepath);
    if (!hasWriteTime || wtIt == m_FileWriteTimes.end() ||
        wtIt->second == currentWriteTime) {
      GINI_DEBUG("ModelCache hit: reusing cached legacy model '", filepath,
                 "'");
      return it->second;
    }

    GINI_INFO("ModelCache: source changed, reloading legacy model '", filepath,
              "'");
    m_LegacyCache.erase(it);
    m_FileWriteTimes.erase(filepath);
  }

  auto model = Model::Create(filepath);
  if (model) {
    m_LegacyCache[filepath] = model;
    if (hasWriteTime) {
      m_FileWriteTimes[filepath] = currentWriteTime;
    }
    GINI_INFO("ModelCache: loaded legacy model '", filepath, "' (",
              m_LegacyCache.size(), " cached)");
  }
  return model;
}

void ModelCache::Evict(const std::string &filepath) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Model3DCache.erase(filepath);
  m_LegacyCache.erase(filepath);
  m_FileWriteTimes.erase(filepath);
}

void ModelCache::Clear() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Model3DCache.clear();
  m_LegacyCache.clear();
  m_FileWriteTimes.clear();
  GINI_INFO("ModelCache: cleared");
}

bool ModelCache::Contains(const std::string &filepath) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_Model3DCache.count(filepath) > 0 ||
         m_LegacyCache.count(filepath) > 0;
}

size_t ModelCache::Size() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_Model3DCache.size() + m_LegacyCache.size();
}

} // namespace Gini
