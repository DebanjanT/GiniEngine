#include "FileDialog.h"

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#endif

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
        NSString* ext = [NSString stringWithUTF8String:filter.extensions.c_str()];
        [allowedTypes addObject:ext];
      }
      [panel setAllowedFileTypes:allowedTypes];
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
        NSString* ext = [NSString stringWithUTF8String:filter.extensions.c_str()];
        [allowedTypes addObject:ext];
      }
      [panel setAllowedFileTypes:allowedTypes];
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
        NSString* ext = [NSString stringWithUTF8String:filter.extensions.c_str()];
        [allowedTypes addObject:ext];
      }
      [panel setAllowedFileTypes:allowedTypes];
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
