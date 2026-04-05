#include "Project.h"
#include "../Core/Logger.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Gini {

Ref<Project> Project::New(const std::string &name,
                          const std::filesystem::path &path) {
  Ref<Project> project = Ref<Project>(new Project());

  project->m_Config.name = name;
  project->m_Config.projectPath = path;
  project->m_Config.assetsPath = path / std::filesystem::path("Assets");
  project->m_Config.materialsPath =
      project->m_Config.assetsPath / std::filesystem::path("Materials");
  project->m_Config.scenesPath =
      project->m_Config.assetsPath / std::filesystem::path("Scenes");
  project->m_Config.scriptsPath =
      project->m_Config.assetsPath / std::filesystem::path("Scripts");

  project->CreateDirectoryStructure();

  s_ActiveProject = project;
  Save();

  GINI_INFO("Created new project: ", name, " at ", path.string());
  return project;
}

Ref<Project> Project::Load(const std::filesystem::path &projectFile) {
  if (!std::filesystem::exists(projectFile)) {
    GINI_ERROR("Project file not found: ", projectFile.string());
    return nullptr;
  }

  try {
    YAML::Node data = YAML::LoadFile(projectFile.string());

    Ref<Project> project = Ref<Project>(new Project());

    project->m_Config.name =
        data["Project"]["Name"].as<std::string>("Untitled");
    project->m_Config.version =
        data["Project"]["Version"].as<std::string>("1.0.0");
    project->m_Config.engineVersion =
        data["Project"]["EngineVersion"].as<std::string>("0.1.0");
    project->m_Config.projectPath = projectFile.parent_path();
    project->m_Config.assetsPath = project->m_Config.projectPath / "Assets";
    project->m_Config.materialsPath =
        project->m_Config.projectPath / "Assets" / "Materials";
    project->m_Config.scenesPath =
        project->m_Config.projectPath / "Assets" / "Scenes";
    project->m_Config.scriptsPath =
        project->m_Config.projectPath / "Assets" / "Scripts";

    s_ActiveProject = project;

    GINI_INFO("Loaded project: ", project->m_Config.name, " from ",
              projectFile.string());
    return project;
  } catch (const YAML::Exception &e) {
    GINI_ERROR("Failed to load project: ", e.what());
    return nullptr;
  }
}

void Project::Save() {
  if (!s_ActiveProject) {
    GINI_WARN("No active project to save");
    return;
  }

  auto &config = s_ActiveProject->m_Config;
  std::filesystem::path projectFile =
      config.projectPath / (config.name + ".giniproject");

  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "Project" << YAML::Value << YAML::BeginMap;
  out << YAML::Key << "Name" << YAML::Value << config.name;
  out << YAML::Key << "Version" << YAML::Value << config.version;
  out << YAML::Key << "EngineVersion" << YAML::Value << config.engineVersion;
  out << YAML::EndMap;
  out << YAML::EndMap;

  std::ofstream fout(projectFile);
  fout << out.c_str();
  fout.close();

  GINI_INFO("Saved project to: ", projectFile.string());
}

void Project::CreateDirectoryStructure() {
  std::filesystem::create_directories(m_Config.assetsPath);
  std::filesystem::create_directories(m_Config.materialsPath);
  std::filesystem::create_directories(m_Config.scenesPath);
  std::filesystem::create_directories(m_Config.scriptsPath);
  std::filesystem::create_directories(m_Config.assetsPath / "Textures");
  std::filesystem::create_directories(m_Config.assetsPath / "Models");
  std::filesystem::create_directories(m_Config.assetsPath / "Audio");
}

std::filesystem::path
Project::GetAssetAbsolutePath(const std::filesystem::path &relativePath) const {
  return m_Config.assetsPath / relativePath;
}

std::filesystem::path
Project::GetAssetRelativePath(const std::filesystem::path &absolutePath) const {
  return std::filesystem::relative(absolutePath, m_Config.assetsPath);
}

} // namespace Gini
