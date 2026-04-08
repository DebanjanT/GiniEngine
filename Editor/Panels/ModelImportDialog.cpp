#include "ModelImportDialog.h"
#include "Asset/AssetRegistry.h"
#include "Core/Logger.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cstdlib>
#include <fstream>
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace Gini {

namespace {

bool WriteAiTextureToPng(const aiTexture *t, const std::string &outPath) {
  if (!t || !t->pcData)
    return false;

  if (t->mHeight == 0) {
    int w = 0, h = 0, ch = 0;
    unsigned char *data = stbi_load_from_memory(
        reinterpret_cast<const stbi_uc *>(t->pcData),
        static_cast<int>(t->mWidth), &w, &h, &ch, 0);
    if (!data)
      return false;
    int stride = w * ch;
    int ok = stbi_write_png(outPath.c_str(), w, h, ch, data, stride);
    stbi_image_free(data);
    return ok != 0;
  }

  const unsigned w = t->mWidth;
  const unsigned h = t->mHeight;
  std::vector<unsigned char> rgba(static_cast<size_t>(w) * h * 4);
  const unsigned char *bgra =
      reinterpret_cast<const unsigned char *>(t->pcData);
  for (unsigned i = 0; i < w * h; i++) {
    rgba[i * 4 + 0] = bgra[i * 4 + 2];
    rgba[i * 4 + 1] = bgra[i * 4 + 1];
    rgba[i * 4 + 2] = bgra[i * 4 + 0];
    rgba[i * 4 + 3] = bgra[i * 4 + 3];
  }
  return stbi_write_png(outPath.c_str(), static_cast<int>(w), static_cast<int>(h),
                        4, rgba.data(), static_cast<int>(w * 4)) != 0;
}

void ExtractEmbeddedTexturesToFolder(const std::filesystem::path &modelFilePath,
                                     const std::filesystem::path &texDir) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      modelFilePath.string(),
      aiProcess_Triangulate | aiProcess_GenSmoothNormals |
          aiProcess_FlipUVs | aiProcess_CalcTangentSpace |
          aiProcess_JoinIdenticalVertices);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    GINI_ERROR("ExtractEmbeddedTextures: Assimp failed: ",
               importer.GetErrorString());
    return;
  }

  for (unsigned i = 0; i < scene->mNumTextures; i++) {
    std::string out =
        (texDir / ("embedded_" + std::to_string(i) + ".png")).string();
    if (WriteAiTextureToPng(scene->mTextures[i], out)) {
      GINI_INFO("Extracted embedded texture: ", out);
    } else {
      GINI_ERROR("Failed to extract embedded texture index ", i);
    }
  }
}

} // namespace

static std::string StemFromPath(const std::string &path) {
  std::filesystem::path p(path);
  return p.stem().string();
}

void ModelImportDialog::Open(const std::string &sourceFilePath) {
  m_SourceFilePath = sourceFilePath;
  m_Open = true;
  m_PreviewLoaded = false;

  m_Settings = ModelImportSettings();
  m_Settings.assetName = StemFromPath(sourceFilePath);

  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      sourceFilePath,
      aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
          aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);

  m_Preview = PreviewInfo();
  if (scene && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) &&
      scene->mRootNode) {
    m_Preview.meshCount = scene->mNumMeshes;
    m_Preview.materialCount = scene->mNumMaterials;
    m_Preview.animationCount = scene->mNumAnimations;

    for (u32 i = 0; i < scene->mNumMeshes; i++) {
      auto *mesh = scene->mMeshes[i];
      m_Preview.vertexCount += mesh->mNumVertices;
      m_Preview.triangleCount += mesh->mNumFaces;
      m_Preview.meshNames.push_back(
          mesh->mName.length ? mesh->mName.C_Str()
                             : ("Mesh_" + std::to_string(i)));
    }

    for (u32 i = 0; i < scene->mNumMaterials; i++) {
      aiString name;
      scene->mMaterials[i]->Get(AI_MATKEY_NAME, name);
      m_Preview.materialNames.push_back(
          name.length ? name.C_Str()
                      : ("Material_" + std::to_string(i)));

      auto collectTextures = [&](aiTextureType type) {
        auto *mat = scene->mMaterials[i];
        for (u32 t = 0; t < mat->GetTextureCount(type); t++) {
          aiString texPath;
          if (mat->GetTexture(type, t, &texPath) == AI_SUCCESS) {
            std::string ts = texPath.C_Str();
            bool found = false;
            for (auto &existing : m_Preview.textureFiles) {
              if (existing == ts) {
                found = true;
                break;
              }
            }
            if (!found)
              m_Preview.textureFiles.push_back(ts);
          }
        }
      };

      collectTextures(aiTextureType_DIFFUSE);
      collectTextures(aiTextureType_NORMALS);
      collectTextures(aiTextureType_HEIGHT);
      collectTextures(aiTextureType_METALNESS);
      collectTextures(aiTextureType_DIFFUSE_ROUGHNESS);
      collectTextures(aiTextureType_AMBIENT_OCCLUSION);
      collectTextures(aiTextureType_EMISSIVE);
      collectTextures(aiTextureType_SPECULAR);
    }

    m_PreviewLoaded = true;
  } else {
    GINI_ERROR("ModelImportDialog: failed to preview '", sourceFilePath,
               "': ", importer.GetErrorString());
  }
}

void ModelImportDialog::OnImGuiRender() {
  if (!m_Open)
    return;

  ImGui::SetNextWindowSize(ImVec2(560, 520), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Import Model", &m_Open,
                    ImGuiWindowFlags_NoDocking)) {
    ImGui::End();
    return;
  }

  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Source:");
  ImGui::SameLine();
  ImGui::TextWrapped("%s", m_SourceFilePath.c_str());
  ImGui::Separator();

  if (m_PreviewLoaded) {
    ImGui::Text("Meshes: %u  |  Materials: %u  |  Animations: %u",
                m_Preview.meshCount, m_Preview.materialCount,
                m_Preview.animationCount);
    ImGui::Text("Vertices: %u  |  Triangles: %u", m_Preview.vertexCount,
                m_Preview.triangleCount);
    ImGui::Separator();
  }

  if (ImGui::BeginTabBar("ImportTabs")) {
    if (ImGui::BeginTabItem("General")) {
      DrawGeneralTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Meshes")) {
      DrawMeshesTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Materials")) {
      DrawMaterialsTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Textures")) {
      DrawTexturesTab();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  ImGui::Separator();

  float buttonWidth = 120.0f;
  float spacing = 8.0f;
  float totalWidth = buttonWidth * 2 + spacing;
  ImGui::SetCursorPosX(ImGui::GetWindowWidth() - totalWidth - 12.0f);

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.56f, 0.72f, 0.85f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImVec4(0.22f, 0.62f, 0.80f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                        ImVec4(0.15f, 0.48f, 0.64f, 1.0f));
  if (ImGui::Button("Import", ImVec2(buttonWidth, 30))) {
    if (PerformImport()) {
      m_Open = false;
    }
  }
  ImGui::PopStyleColor(3);

  ImGui::SameLine(0, spacing);
  if (ImGui::Button("Cancel", ImVec2(buttonWidth, 30))) {
    m_Open = false;
  }

  ImGui::End();
}

void ModelImportDialog::DrawGeneralTab() {
  char nameBuf[256];
  std::strncpy(nameBuf, m_Settings.assetName.c_str(), sizeof(nameBuf));
  nameBuf[sizeof(nameBuf) - 1] = '\0';
  if (ImGui::InputText("Asset Name", nameBuf, sizeof(nameBuf))) {
    m_Settings.assetName = nameBuf;
  }

  char destBuf[256];
  std::strncpy(destBuf, m_Settings.destinationFolder.c_str(), sizeof(destBuf));
  destBuf[sizeof(destBuf) - 1] = '\0';
  if (ImGui::InputText("Destination Folder", destBuf, sizeof(destBuf))) {
    m_Settings.destinationFolder = destBuf;
  }

  ImGui::Spacing();
  ImGui::Text("Transform Offsets");
  ImGui::DragFloat3("Position", &m_Settings.positionOffset.x, 0.1f);
  ImGui::DragFloat3("Rotation", &m_Settings.rotationOffset.x, 1.0f);
  ImGui::DragFloat3("Scale", &m_Settings.scaleMultiplier.x, 0.01f, 0.001f,
                    100.0f);
}

void ModelImportDialog::DrawMeshesTab() {
  ImGui::Checkbox("Import Meshes", &m_Settings.importMeshes);
  ImGui::Checkbox("Combine Meshes", &m_Settings.combineMeshes);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Text("Meshes found:");
  for (u32 i = 0; i < m_Preview.meshNames.size(); i++) {
    ImGui::BulletText("%s", m_Preview.meshNames[i].c_str());
  }
}

void ModelImportDialog::DrawMaterialsTab() {
  ImGui::Checkbox("Import Materials", &m_Settings.importMaterials);
  ImGui::Checkbox("Convert to PBR", &m_Settings.convertToPBR);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Text("Materials found:");
  for (u32 i = 0; i < m_Preview.materialNames.size(); i++) {
    ImGui::BulletText("%s", m_Preview.materialNames[i].c_str());
  }
}

void ModelImportDialog::DrawTexturesTab() {
  ImGui::Checkbox("Import Textures", &m_Settings.importTextures);

  const char *resOptions[] = {"512", "1024", "2048", "4096", "8192"};
  u32 resValues[] = {512, 1024, 2048, 4096, 8192};
  int currentRes = 3;
  for (int i = 0; i < 5; i++) {
    if (resValues[i] == m_Settings.maxTextureResolution) {
      currentRes = i;
      break;
    }
  }
  if (ImGui::Combo("Max Resolution", &currentRes, resOptions, 5)) {
    m_Settings.maxTextureResolution = resValues[currentRes];
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Text("Referenced textures:");
  if (m_Preview.textureFiles.empty()) {
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "None found");
  } else {
    for (auto &tex : m_Preview.textureFiles) {
      if (!tex.empty() && tex[0] == '*') {
        int idx = std::atoi(tex.c_str() + 1);
        ImGui::BulletText("Embedded #%d (inside model file) -> embedded_%d.png",
                          idx, idx);
      } else {
        ImGui::BulletText("%s", tex.c_str());
      }
    }
  }
}

bool ModelImportDialog::PerformImport() {
  auto rootPath = AssetRegistry::Get().GetRootPath();
  if (rootPath.empty()) {
    GINI_ERROR("ModelImportDialog: no project root path set");
    return false;
  }

  std::filesystem::path destDir =
      rootPath / m_Settings.destinationFolder / m_Settings.assetName;
  std::error_code ec;
  std::filesystem::create_directories(destDir, ec);
  if (ec) {
    GINI_ERROR("ModelImportDialog: failed to create directory '",
               destDir.string(), "': ", ec.message());
    return false;
  }

  std::filesystem::path srcPath(m_SourceFilePath);
  std::filesystem::path destModelFile =
      destDir / srcPath.filename();
  std::filesystem::copy_file(srcPath, destModelFile,
                             std::filesystem::copy_options::overwrite_existing,
                             ec);
  if (ec) {
    GINI_ERROR("ModelImportDialog: failed to copy model file: ", ec.message());
    return false;
  }

  if (m_Settings.importTextures) {
    std::filesystem::path texDir = destDir / "Textures";
    std::filesystem::create_directories(texDir, ec);

    std::filesystem::path srcDir = srcPath.parent_path();
    for (auto &texRelPath : m_Preview.textureFiles) {
      if (!texRelPath.empty() && texRelPath[0] == '*')
        continue;
      std::filesystem::path texSrc = srcDir / texRelPath;
      if (std::filesystem::exists(texSrc, ec)) {
        std::filesystem::path texDest =
            texDir / std::filesystem::path(texRelPath).filename();
        std::filesystem::copy_file(
            texSrc, texDest,
            std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
          GINI_ERROR("ModelImportDialog: failed to copy texture '",
                     texRelPath, "': ", ec.message());
        }
      }
    }

    ExtractEmbeddedTexturesToFolder(destModelFile, texDir);
  }

  if (!GenerateGMeshManifest(destDir)) {
    GINI_ERROR("ModelImportDialog: failed to generate .gmesh manifest");
    return false;
  }

  AssetRegistry::Get().Refresh();

  GINI_INFO("Model imported to: ", destDir.string());

  if (m_OnImportComplete) {
    std::filesystem::path gmeshPath =
        destDir / (m_Settings.assetName + ".gmesh");
    m_OnImportComplete(gmeshPath.string());
  }

  return true;
}

bool ModelImportDialog::GenerateGMeshManifest(
    const std::filesystem::path &destDir) {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "Asset" << YAML::Value << m_Settings.assetName;
  out << YAML::Key << "SourceFile"
      << YAML::Value
      << (std::filesystem::path(m_SourceFilePath).filename().string());
  out << YAML::Key << "MeshCount" << YAML::Value << m_Preview.meshCount;
  out << YAML::Key << "MaterialCount" << YAML::Value
      << m_Preview.materialCount;
  out << YAML::Key << "AnimationCount" << YAML::Value
      << m_Preview.animationCount;
  out << YAML::Key << "VertexCount" << YAML::Value << m_Preview.vertexCount;
  out << YAML::Key << "TriangleCount" << YAML::Value
      << m_Preview.triangleCount;

  out << YAML::Key << "ImportSettings" << YAML::Value;
  out << YAML::BeginMap;
  out << YAML::Key << "CombineMeshes" << YAML::Value
      << m_Settings.combineMeshes;
  out << YAML::Key << "ConvertToPBR" << YAML::Value << m_Settings.convertToPBR;
  out << YAML::Key << "MaxTextureResolution" << YAML::Value
      << m_Settings.maxTextureResolution;
  out << YAML::Key << "PositionOffset" << YAML::Value << YAML::Flow
      << YAML::BeginSeq << m_Settings.positionOffset.x
      << m_Settings.positionOffset.y << m_Settings.positionOffset.z
      << YAML::EndSeq;
  out << YAML::Key << "RotationOffset" << YAML::Value << YAML::Flow
      << YAML::BeginSeq << m_Settings.rotationOffset.x
      << m_Settings.rotationOffset.y << m_Settings.rotationOffset.z
      << YAML::EndSeq;
  out << YAML::Key << "ScaleMultiplier" << YAML::Value << YAML::Flow
      << YAML::BeginSeq << m_Settings.scaleMultiplier.x
      << m_Settings.scaleMultiplier.y << m_Settings.scaleMultiplier.z
      << YAML::EndSeq;
  out << YAML::EndMap;

  out << YAML::Key << "Meshes" << YAML::Value << YAML::BeginSeq;
  for (auto &name : m_Preview.meshNames) {
    out << name;
  }
  out << YAML::EndSeq;

  out << YAML::Key << "Materials" << YAML::Value << YAML::BeginSeq;
  for (auto &name : m_Preview.materialNames) {
    out << name;
  }
  out << YAML::EndSeq;

  out << YAML::Key << "Textures" << YAML::Value << YAML::BeginSeq;
  for (auto &tex : m_Preview.textureFiles) {
    if (!tex.empty() && tex[0] == '*') {
      int idx = std::atoi(tex.c_str() + 1);
      out << ("embedded_" + std::to_string(idx) + ".png");
    } else {
      out << std::filesystem::path(tex).filename().string();
    }
  }
  out << YAML::EndSeq;

  out << YAML::EndMap;

  std::filesystem::path gmeshPath =
      destDir / (m_Settings.assetName + ".gmesh");
  std::ofstream fout(gmeshPath);
  if (!fout.is_open()) {
    return false;
  }
  fout << out.c_str();
  return true;
}

} // namespace Gini
