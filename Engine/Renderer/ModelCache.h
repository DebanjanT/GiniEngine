#pragma once

#include "Core/Types.h"
#include "Renderer/Model.h"
#include "Renderer/Model3D.h"
#include <filesystem>
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

  // Load using the new Model3D system (improved Hazel-based approach)
  Ref<Model3D> LoadModel3D(const std::string &filepath);

  // Legacy support for old Model system
  Ref<Model> Load(const std::string &filepath);

  void Evict(const std::string &filepath);
  void Clear();
  bool Contains(const std::string &filepath) const;
  size_t Size() const;

private:
  ModelCache() = default;

  mutable std::mutex m_Mutex;
  std::unordered_map<std::string, Ref<Model3D>> m_Model3DCache;
  std::unordered_map<std::string, Ref<Model>> m_LegacyCache;
  std::unordered_map<std::string, std::filesystem::file_time_type>
      m_FileWriteTimes;
};

} // namespace Gini
