#pragma once

#include "EditorPanel.h"
#include "Core/Types.h"
#include <filesystem>
#include <string>
#include <vector>

namespace Gini {

struct ModelImportSettings {
  std::string assetName;
  std::string destinationFolder = "Models";

  Vec3 positionOffset{0.0f};
  Vec3 rotationOffset{0.0f};
  Vec3 scaleMultiplier{1.0f};

  bool importMeshes = true;
  bool combineMeshes = false;

  bool importMaterials = true;
  bool convertToPBR = true;

  bool importTextures = true;
  u32 maxTextureResolution = 4096;

  bool importAnimations = true;
};

class ModelImportDialog {
public:
  ModelImportDialog() = default;

  void Open(const std::string &sourceFilePath);
  void OnImGuiRender();
  bool IsOpen() const { return m_Open; }

  using ImportCallback =
      std::function<void(const std::string &gmeshPath)>;
  void SetOnImportComplete(ImportCallback cb) { m_OnImportComplete = cb; }

private:
  void DrawGeneralTab();
  void DrawMeshesTab();
  void DrawMaterialsTab();
  void DrawTexturesTab();

  bool PerformImport();
  bool GenerateGMeshManifest(const std::filesystem::path &destDir);

  bool m_Open = false;
  std::string m_SourceFilePath;
  ModelImportSettings m_Settings;

  struct PreviewInfo {
    u32 meshCount = 0;
    u32 materialCount = 0;
    u32 animationCount = 0;
    u32 vertexCount = 0;
    u32 triangleCount = 0;
    std::vector<std::string> meshNames;
    std::vector<std::string> materialNames;
    std::vector<std::string> textureFiles;
  };
  PreviewInfo m_Preview;
  bool m_PreviewLoaded = false;

  ImportCallback m_OnImportComplete;
};

} // namespace Gini
