#include "ProjectLauncher.h"
#include "Core/Logger.h"
#include "Utils/FileDialog.h"
#include <cstring>
#include <ctime>
#include <fstream>
#include <imgui.h>
#include <yaml-cpp/yaml.h>

#ifdef __APPLE__
#include <limits.h>
#include <mach-o/dyld.h>
#endif

namespace Gini {

static std::filesystem::path GetExecutablePath() {
#ifdef __APPLE__
  char path[PATH_MAX];
  uint32_t size = sizeof(path);
  if (_NSGetExecutablePath(path, &size) == 0) {
    return std::filesystem::path(path).parent_path();
  }
#endif
  return std::filesystem::current_path();
}

static std::filesystem::path GetAppDataPath() {
#ifdef __APPLE__
  const char *home = getenv("HOME");
  if (home) {
    return std::filesystem::path(home) / "Library" / "Application Support" /
           "GiniEngine";
  }
#elif defined(_WIN32)
  const char *appdata = getenv("APPDATA");
  if (appdata) {
    return std::filesystem::path(appdata) / "GiniEngine";
  }
#else
  const char *home = getenv("HOME");
  if (home) {
    return std::filesystem::path(home) / ".config" / "GiniEngine";
  }
#endif
  return GetExecutablePath();
}

ProjectLauncher::ProjectLauncher() {
  // Set default project path to user's Documents folder
#ifdef __APPLE__
  const char *home = getenv("HOME");
  if (home) {
    std::string defaultPath = std::string(home) + "/Documents/GiniProjects";
    strncpy(m_NewProjectPath, defaultPath.c_str(),
            sizeof(m_NewProjectPath) - 1);
  }
#elif defined(_WIN32)
  const char *userprofile = getenv("USERPROFILE");
  if (userprofile) {
    std::string defaultPath =
        std::string(userprofile) + "\\Documents\\GiniProjects";
    strncpy(m_NewProjectPath, defaultPath.c_str(),
            sizeof(m_NewProjectPath) - 1);
  }
#endif

  // Setup recent projects file path
  m_RecentProjectsFile = GetAppDataPath() / "recent_projects.yaml";

  // Create app data directory if it doesn't exist
  std::filesystem::path appDataPath = GetAppDataPath();
  if (!std::filesystem::exists(appDataPath)) {
    std::filesystem::create_directories(appDataPath);
  }

  LoadRecentProjects();
}

void ProjectLauncher::LoadRecentProjects() {
  m_RecentProjects.clear();

  if (!std::filesystem::exists(m_RecentProjectsFile)) {
    return;
  }

  try {
    YAML::Node data = YAML::LoadFile(m_RecentProjectsFile.string());

    if (data["RecentProjects"]) {
      for (const auto &project : data["RecentProjects"]) {
        RecentProject rp;
        rp.name = project["Name"].as<std::string>();
        rp.path = project["Path"].as<std::string>();
        rp.lastOpened = project["LastOpened"].as<std::string>();

        // Only add if project still exists
        if (std::filesystem::exists(rp.path)) {
          m_RecentProjects.push_back(rp);
        }
      }
    }
  } catch (const std::exception &e) {
    GINI_ERROR("Failed to load recent projects: ", e.what());
  }
}

void ProjectLauncher::SaveRecentProjects() {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "RecentProjects" << YAML::Value << YAML::BeginSeq;

  for (const auto &project : m_RecentProjects) {
    out << YAML::BeginMap;
    out << YAML::Key << "Name" << YAML::Value << project.name;
    out << YAML::Key << "Path" << YAML::Value << project.path.string();
    out << YAML::Key << "LastOpened" << YAML::Value << project.lastOpened;
    out << YAML::EndMap;
  }

  out << YAML::EndSeq;
  out << YAML::EndMap;

  std::ofstream fout(m_RecentProjectsFile);
  if (fout.is_open()) {
    fout << out.c_str();
    fout.close();
  }
}

void ProjectLauncher::AddToRecentProjects(const std::string &name,
                                          const std::filesystem::path &path) {
  // Remove if already exists
  m_RecentProjects.erase(std::remove_if(m_RecentProjects.begin(),
                                        m_RecentProjects.end(),
                                        [&path](const RecentProject &rp) {
                                          return rp.path == path;
                                        }),
                         m_RecentProjects.end());

  // Get current time
  time_t now = time(nullptr);
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", localtime(&now));

  // Add to front
  RecentProject rp;
  rp.name = name;
  rp.path = path;
  rp.lastOpened = timeStr;
  m_RecentProjects.insert(m_RecentProjects.begin(), rp);

  // Keep only last 10 projects
  if (m_RecentProjects.size() > 10) {
    m_RecentProjects.resize(10);
  }

  SaveRecentProjects();
}

void ProjectLauncher::OnImGuiRender() {
  if (!m_IsOpen)
    return;

  // Center the launcher window
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImVec2 center = viewport->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_Always);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoDocking;

  if (ImGui::Begin("Gini Engine - Project Launcher", nullptr, flags)) {
    DrawHeader();
    ImGui::Separator();

    // Two column layout
    ImGui::Columns(2, "LauncherColumns", true);
    ImGui::SetColumnWidth(0, 450);

    // Left side: Recent projects
    DrawRecentProjects();

    ImGui::NextColumn();

    // Right side: New/Open project
    DrawNewProjectSection();
    ImGui::Spacing();
    ImGui::Spacing();
    DrawOpenProjectSection();

    ImGui::Columns(1);

    // Error message
    if (!m_ErrorMessage.empty()) {
      ImGui::Separator();
      ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s",
                         m_ErrorMessage.c_str());

      m_ErrorTimer -= ImGui::GetIO().DeltaTime;
      if (m_ErrorTimer <= 0) {
        m_ErrorMessage.clear();
      }
    }
  }
  ImGui::End();
}

void ProjectLauncher::DrawHeader() {
  // Title
  ImGui::PushFont(
      ImGui::GetIO().Fonts->Fonts[0]); // Use default font, could use larger
  ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "GINI ENGINE");
  ImGui::PopFont();

  ImGui::SameLine(ImGui::GetWindowWidth() - 150);
  ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Version 1.0");

  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                     "Select a project to open or create a new one");
}

void ProjectLauncher::DrawRecentProjects() {
  ImGui::Text("Recent Projects");
  ImGui::Separator();

  if (m_RecentProjects.empty()) {
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No recent projects");
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                       "Create a new project to get started!");
  } else {
    ImGui::BeginChild("RecentProjectsList", ImVec2(0, 350), true);

    for (size_t i = 0; i < m_RecentProjects.size(); i++) {
      const auto &project = m_RecentProjects[i];

      ImGui::PushID((int)i);

      // Use a simple layout instead of SetCursorScreenPos
      ImGui::BeginGroup();

      // Project name (bold/highlighted)
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s",
                         project.name.c_str());

      // Project path
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s",
                         project.path.string().c_str());

      // Last opened
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Last opened: %s",
                         project.lastOpened.c_str());

      ImGui::EndGroup();

      // Make the group clickable
      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        OpenProject(project.path);
      }

      ImGui::Separator();
      ImGui::PopID();
    }

    ImGui::EndChild();
  }
}

void ProjectLauncher::DrawNewProjectSection() {
  ImGui::Text("New Project");
  ImGui::Separator();

  if (!m_ShowNewProjectForm) {
    if (ImGui::Button("Create New Project", ImVec2(-1, 40))) {
      m_ShowNewProjectForm = true;
    }
  } else {
    ImGui::Text("Project Name:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##ProjectName", m_NewProjectName,
                     sizeof(m_NewProjectName));

    ImGui::Text("Location:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##ProjectPath", m_NewProjectPath,
                     sizeof(m_NewProjectPath));

    ImGui::Spacing();

    if (ImGui::Button("Create", ImVec2(100, 30))) {
      CreateNewProject();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 30))) {
      m_ShowNewProjectForm = false;
    }
  }
}

void ProjectLauncher::DrawOpenProjectSection() {
  ImGui::Text("Open Project");
  ImGui::Separator();

  if (ImGui::Button("Browse...", ImVec2(-1, 40))) {
    OpenProjectDialog();
  }

  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                     "Select a .giniproject file");
}

void ProjectLauncher::CreateNewProject() {
  std::string name = m_NewProjectName;
  std::filesystem::path basePath = m_NewProjectPath;

  if (name.empty()) {
    m_ErrorMessage = "Project name cannot be empty";
    m_ErrorTimer = 3.0f;
    return;
  }

  if (basePath.empty()) {
    m_ErrorMessage = "Project path cannot be empty";
    m_ErrorTimer = 3.0f;
    return;
  }

  // Create project directory
  std::filesystem::path projectPath = basePath / name;

  if (std::filesystem::exists(projectPath)) {
    m_ErrorMessage = "A project already exists at this location";
    m_ErrorTimer = 3.0f;
    return;
  }

  // Create the project
  m_LoadedProject = Project::New(name, projectPath);

  if (m_LoadedProject) {
    AddToRecentProjects(name, projectPath);
    m_ProjectLoaded = true;
    m_IsOpen = false;
    m_ShowNewProjectForm = false;
    GINI_INFO("Created new project: ", name, " at ", projectPath.string());
  } else {
    m_ErrorMessage = "Failed to create project";
    m_ErrorTimer = 3.0f;
  }
}

void ProjectLauncher::OpenProject(const std::filesystem::path &path) {
  std::filesystem::path projectFile = path;

  // If path is a directory, look for .giniproject file
  if (std::filesystem::is_directory(path)) {
    for (const auto &entry : std::filesystem::directory_iterator(path)) {
      if (entry.path().extension() == ".giniproject") {
        projectFile = entry.path();
        break;
      }
    }
  }

  if (!std::filesystem::exists(projectFile)) {
    m_ErrorMessage = "Project file not found";
    m_ErrorTimer = 3.0f;
    return;
  }

  m_LoadedProject = Project::Load(projectFile);

  if (m_LoadedProject) {
    AddToRecentProjects(m_LoadedProject->GetConfig().name, path);
    m_ProjectLoaded = true;
    m_IsOpen = false;
    GINI_INFO("Opened project: ", m_LoadedProject->GetConfig().name);
  } else {
    m_ErrorMessage = "Failed to load project";
    m_ErrorTimer = 3.0f;
  }
}

void ProjectLauncher::OpenProjectDialog() {
  // Use native file dialog to select .giniproject file
  std::vector<FileDialogFilter> filters = {{"Gini Project", "giniproject"}};

  std::string filepath = FileDialog::OpenFile(filters);

  if (!filepath.empty()) {
    OpenProject(filepath);
  }
}

} // namespace Gini
