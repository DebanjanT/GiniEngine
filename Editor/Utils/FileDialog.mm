#include "FileDialog.h"

#include <cctype>
#include <sstream>
#include <string>

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#endif

namespace {

// macOS NSOpenPanel expects one extension per allowed type (e.g. "obj", "fbx"),
// not a comma-separated list in a single string.
static std::string TrimExtensionToken(const std::string &s) {
  size_t start = 0;
  while (start < s.size() &&
         std::isspace(static_cast<unsigned char>(s[start])))
    start++;
  size_t end = s.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(s[end - 1])))
    end--;
  std::string out = s.substr(start, end - start);
  if (!out.empty() && out[0] == '.')
    out = out.substr(1);
  return out;
}

static void AddAllowedExtensions(const std::string &extensions,
                                 NSMutableArray *allowedTypes) {
  std::stringstream ss(extensions);
  std::string token;
  while (std::getline(ss, token, ',')) {
    std::string ext = TrimExtensionToken(token);
    if (ext.empty())
      continue;
    [allowedTypes addObject:[NSString stringWithUTF8String:ext.c_str()]];
  }
}

} // namespace

namespace Gini {

std::string FileDialog::OpenFile(const std::vector<FileDialogFilter>& filters) {
#ifdef __APPLE__
  @autoreleasepool {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    [panel setAllowsMultipleSelection:NO];
    
    // Set file type filters
    if (!filters.empty()) {
      NSMutableArray* allowedTypes = [NSMutableArray array];
      for (const auto& filter : filters) {
        AddAllowedExtensions(filter.extensions, allowedTypes);
      }
      if ([allowedTypes count] > 0) {
        [panel setAllowedFileTypes:allowedTypes];
      }
    }
    
    if ([panel runModal] == NSModalResponseOK) {
      NSURL* url = [[panel URLs] objectAtIndex:0];
      return std::string([[url path] UTF8String]);
    }
  }
#endif
  return "";
}

std::vector<std::string> FileDialog::OpenFiles(const std::vector<FileDialogFilter>& filters) {
  std::vector<std::string> result;
#ifdef __APPLE__
  @autoreleasepool {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    [panel setAllowsMultipleSelection:YES];
    
    if (!filters.empty()) {
      NSMutableArray* allowedTypes = [NSMutableArray array];
      for (const auto& filter : filters) {
        AddAllowedExtensions(filter.extensions, allowedTypes);
      }
      if ([allowedTypes count] > 0) {
        [panel setAllowedFileTypes:allowedTypes];
      }
    }
    
    if ([panel runModal] == NSModalResponseOK) {
      for (NSURL* url in [panel URLs]) {
        result.push_back(std::string([[url path] UTF8String]));
      }
    }
  }
#endif
  return result;
}

std::string FileDialog::OpenFolder() {
#ifdef __APPLE__
  @autoreleasepool {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setCanChooseFiles:NO];
    [panel setCanChooseDirectories:YES];
    [panel setAllowsMultipleSelection:NO];
    
    if ([panel runModal] == NSModalResponseOK) {
      NSURL* url = [[panel URLs] objectAtIndex:0];
      return std::string([[url path] UTF8String]);
    }
  }
#endif
  return "";
}

std::string FileDialog::SaveFile(const std::vector<FileDialogFilter>& filters,
                                  const std::string& defaultName) {
#ifdef __APPLE__
  @autoreleasepool {
    NSSavePanel* panel = [NSSavePanel savePanel];
    
    if (!defaultName.empty()) {
      [panel setNameFieldStringValue:[NSString stringWithUTF8String:defaultName.c_str()]];
    }
    
    if (!filters.empty()) {
      NSMutableArray* allowedTypes = [NSMutableArray array];
      for (const auto& filter : filters) {
        AddAllowedExtensions(filter.extensions, allowedTypes);
      }
      if ([allowedTypes count] > 0) {
        [panel setAllowedFileTypes:allowedTypes];
      }
    }
    
    if ([panel runModal] == NSModalResponseOK) {
      NSURL* url = [panel URL];
      return std::string([[url path] UTF8String]);
    }
  }
#endif
  return "";
}

} // namespace Gini
