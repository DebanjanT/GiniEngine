#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace Gini {

struct FileDialogFilter {
  std::string name;        // e.g., "Project Files"
  std::string extensions;  // e.g., "giniproject"
};

class FileDialog {
public:
  // Open a file dialog to select a single file
  // Returns empty string if cancelled
  static std::string OpenFile(const std::vector<FileDialogFilter>& filters = {});
  
  // Open a file dialog to select multiple files
  static std::vector<std::string> OpenFiles(const std::vector<FileDialogFilter>& filters = {});
  
  // Open a folder selection dialog
  static std::string OpenFolder();
  
  // Save file dialog
  // Returns empty string if cancelled
  static std::string SaveFile(const std::vector<FileDialogFilter>& filters = {}, 
                              const std::string& defaultName = "");
};

} // namespace Gini
