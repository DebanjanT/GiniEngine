#include "FileDialog.h"

#if defined(__linux__)

#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace Gini {

// Uses zenity (GTK dialog tool) which is available on most Linux desktops.
// Falls back to empty string if zenity is not installed.

static std::string ExecCommand(const std::string &cmd) {
  std::string result;
  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe)
    return "";
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe)) {
    result += buffer;
  }
  int status = pclose(pipe);
  if (status != 0)
    return "";
  // Trim trailing newline
  while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
    result.pop_back();
  }
  return result;
}

static std::string BuildZenityFilter(
    const std::vector<FileDialogFilter> &filters) {
  if (filters.empty())
    return "";
  std::string result;
  for (const auto &f : filters) {
    result += " --file-filter=\"" + f.name + " | *." + f.extensions + "\"";
  }
  return result;
}

std::string
FileDialog::OpenFile(const std::vector<FileDialogFilter> &filters) {
  std::string cmd = "zenity --file-selection --title=\"Open File\"";
  cmd += BuildZenityFilter(filters);
  return ExecCommand(cmd);
}

std::vector<std::string>
FileDialog::OpenFiles(const std::vector<FileDialogFilter> &filters) {
  std::vector<std::string> result;
  std::string cmd =
      "zenity --file-selection --multiple --separator=\"|\" --title=\"Open Files\"";
  cmd += BuildZenityFilter(filters);
  std::string output = ExecCommand(cmd);
  if (output.empty())
    return result;

  std::istringstream stream(output);
  std::string path;
  while (std::getline(stream, path, '|')) {
    if (!path.empty()) {
      result.push_back(path);
    }
  }
  return result;
}

std::string FileDialog::OpenFolder() {
  return ExecCommand(
      "zenity --file-selection --directory --title=\"Select Folder\"");
}

std::string FileDialog::SaveFile(const std::vector<FileDialogFilter> &filters,
                                 const std::string &defaultName) {
  std::string cmd =
      "zenity --file-selection --save --confirm-overwrite --title=\"Save File\"";
  if (!defaultName.empty()) {
    cmd += " --filename=\"" + defaultName + "\"";
  }
  cmd += BuildZenityFilter(filters);
  return ExecCommand(cmd);
}

} // namespace Gini

#endif // __linux__
