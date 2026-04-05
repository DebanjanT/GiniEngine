#pragma once

#include "EditorPanel.h"
#include "Project/Project.h"
#include <filesystem>
#include <vector>
#include <string>

namespace Gini {

struct RecentProject {
  std::string name;
  std::filesystem::path path;
  std::string lastOpened;
};

class ProjectLauncher {
public:
  ProjectLauncher();
  ~ProjectLauncher() = default;
  
  void OnImGuiRender();
  
  bool IsOpen() const { return m_IsOpen; }
  void Open() { m_IsOpen = true; }
  void Close() { m_IsOpen = false; }
  
  bool HasProjectLoaded() const { return m_ProjectLoaded; }
  Ref<Project> GetLoadedProject() const { return m_LoadedProject; }
  
  // Load/save recent projects list
  void LoadRecentProjects();
  void SaveRecentProjects();
  void AddToRecentProjects(const std::string& name, const std::filesystem::path& path);

private:
  void DrawHeader();
  void DrawRecentProjects();
  void DrawNewProjectSection();
  void DrawOpenProjectSection();
  
  void CreateNewProject();
  void OpenProject(const std::filesystem::path& path);
  void OpenProjectDialog();
  
  bool m_IsOpen = true;
  bool m_ProjectLoaded = false;
  Ref<Project> m_LoadedProject;
  
  // Recent projects
  std::vector<RecentProject> m_RecentProjects;
  std::filesystem::path m_RecentProjectsFile;
  
  // New project form
  char m_NewProjectName[256] = "MyProject";
  char m_NewProjectPath[512] = "";
  bool m_ShowNewProjectForm = false;
  
  // Error message
  std::string m_ErrorMessage;
  float m_ErrorTimer = 0.0f;
};

} // namespace Gini
