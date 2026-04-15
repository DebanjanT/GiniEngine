#include "FileDialog.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <codecvt>
#include <locale>

namespace Gini {

static std::string WideToUTF8(const std::wstring &wide) {
  if (wide.empty())
    return "";
  int sizeNeeded =
      WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), nullptr,
                          0, nullptr, nullptr);
  std::string result(sizeNeeded, 0);
  WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), &result[0],
                      sizeNeeded, nullptr, nullptr);
  return result;
}

static std::wstring UTF8ToWide(const std::string &str) {
  if (str.empty())
    return L"";
  int sizeNeeded =
      MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
  std::wstring result(sizeNeeded, 0);
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0],
                      sizeNeeded);
  return result;
}

// Build a Win32 filter string: "Name\0*.ext;*.ext2\0Name2\0*.ext3\0\0"
static std::wstring BuildFilterString(
    const std::vector<FileDialogFilter> &filters) {
  std::wstring result;
  for (const auto &f : filters) {
    result += UTF8ToWide(f.name);
    result += L'\0';
    
    // Parse comma-separated extensions and format as "*.ext1;*.ext2;..."
    std::wstring extPattern;
    std::string ext;
    for (size_t i = 0; i <= f.extensions.size(); ++i) {
      if (i == f.extensions.size() || f.extensions[i] == ',') {
        if (!ext.empty()) {
          if (!extPattern.empty()) {
            extPattern += L';';
          }
          extPattern += L"*.";
          extPattern += UTF8ToWide(ext);
          ext.clear();
        }
      } else {
        ext += f.extensions[i];
      }
    }
    result += extPattern;
    result += L'\0';
  }
  if (result.empty()) {
    result = L"All Files\0*.*\0";
  }
  result += L'\0';
  return result;
}

std::string
FileDialog::OpenFile(const std::vector<FileDialogFilter> &filters) {
  std::wstring filterStr = BuildFilterString(filters);

  wchar_t fileName[MAX_PATH] = {0};
  OPENFILENAMEW ofn = {};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = nullptr;
  ofn.lpstrFilter = filterStr.c_str();
  ofn.lpstrFile = fileName;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

  if (GetOpenFileNameW(&ofn)) {
    return WideToUTF8(fileName);
  }
  return "";
}

std::vector<std::string>
FileDialog::OpenFiles(const std::vector<FileDialogFilter> &filters) {
  std::vector<std::string> result;
  std::wstring filterStr = BuildFilterString(filters);

  wchar_t fileName[4096] = {0};
  OPENFILENAMEW ofn = {};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = nullptr;
  ofn.lpstrFilter = filterStr.c_str();
  ofn.lpstrFile = fileName;
  ofn.nMaxFile = sizeof(fileName) / sizeof(wchar_t);
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT |
              OFN_EXPLORER | OFN_NOCHANGEDIR;

  if (GetOpenFileNameW(&ofn)) {
    std::wstring dir = fileName;
    wchar_t *p = fileName + dir.size() + 1;
    if (*p == L'\0') {
      result.push_back(WideToUTF8(dir));
    } else {
      while (*p) {
        std::wstring file = p;
        result.push_back(WideToUTF8(dir + L"\\" + file));
        p += file.size() + 1;
      }
    }
  }
  return result;
}

std::string FileDialog::OpenFolder() {
  std::string result;

  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  IFileDialog *pfd = nullptr;
  HRESULT hr =
      CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
                       IID_IFileDialog, reinterpret_cast<void **>(&pfd));
  if (SUCCEEDED(hr)) {
    DWORD options;
    pfd->GetOptions(&options);
    pfd->SetOptions(options | FOS_PICKFOLDERS);

    if (SUCCEEDED(pfd->Show(nullptr))) {
      IShellItem *psi = nullptr;
      if (SUCCEEDED(pfd->GetResult(&psi))) {
        PWSTR pszPath = nullptr;
        if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
          result = WideToUTF8(pszPath);
          CoTaskMemFree(pszPath);
        }
        psi->Release();
      }
    }
    pfd->Release();
  }
  CoUninitialize();

  return result;
}

std::string FileDialog::SaveFile(const std::vector<FileDialogFilter> &filters,
                                 const std::string &defaultName) {
  std::wstring filterStr = BuildFilterString(filters);

  wchar_t fileName[MAX_PATH] = {0};
  if (!defaultName.empty()) {
    std::wstring wideName = UTF8ToWide(defaultName);
    wcsncpy(fileName, wideName.c_str(), MAX_PATH - 1);
  }

  OPENFILENAMEW ofn = {};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = nullptr;
  ofn.lpstrFilter = filterStr.c_str();
  ofn.lpstrFile = fileName;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

  if (GetSaveFileNameW(&ofn)) {
    return WideToUTF8(fileName);
  }
  return "";
}

} // namespace Gini

#endif // _WIN32
