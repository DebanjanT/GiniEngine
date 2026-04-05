#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace Gini {

template <typename T> using Ref = std::shared_ptr<T>;

struct ProjectConfig {
  std::string name = "Untitled Project";
  std::string version = "1.0.0";
  std::string engineVersion = "0.1.0";
  std::filesystem::path projectPath;
  std::filesystem::path assetsPath;
  std::filesystem::path materialsPath;
  std::filesystem::path scenesPath;
  std::filesystem::path scriptsPath;
};

class Project {
public:
  static Ref<Project> New(const std::string &name,
                          const std::filesystem::path &path);
  static Ref<Project> Load(const std::filesystem::path &projectFile);
  static void Save();

  static Ref<Project> GetActive() { return s_ActiveProject; }
  static void SetActive(Ref<Project> project) { s_ActiveProject = project; }

  const ProjectConfig &GetConfig() const { return m_Config; }
  ProjectConfig &GetConfig() { return m_Config; }

  const std::filesystem::path &GetProjectPath() const {
    return m_Config.projectPath;
  }
  const std::filesystem::path &GetAssetsPath() const {
    return m_Config.assetsPath;
  }
  const std::filesystem::path &GetMaterialsPath() const {
    return m_Config.materialsPath;
  }

  std::filesystem::path
  GetAssetAbsolutePath(const std::filesystem::path &relativePath) const;
  std::filesystem::path
  GetAssetRelativePath(const std::filesystem::path &absolutePath) const;

  bool IsValid() const { return !m_Config.projectPath.empty(); }

private:
  Project() = default;
  void CreateDirectoryStructure();

  ProjectConfig m_Config;

  inline static Ref<Project> s_ActiveProject = nullptr;
};

} // namespace Gini
