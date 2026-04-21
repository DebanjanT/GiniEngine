#include "MaterialAsset.h"
#include "Core/Logger.h"
#include <yaml-cpp/yaml.h>
#include <fstream>

namespace Gini {

MaterialAsset::MaterialAsset(const std::string& name)
    : m_Name(name) {
  m_Material = Material::Create(name);
}

Ref<MaterialAsset> MaterialAsset::Create(const std::string& name) {
  return CreateRef<MaterialAsset>(name);
}

void MaterialAsset::SyncToMaterial() {
  if (!m_Material) {
    m_Material = Material::Create(m_Name);
  }
  m_Material->SetAlbedoColor(m_AlbedoColor);
  m_Material->SetMetallic(m_Metalness);
  m_Material->SetRoughness(m_Roughness);
  m_Material->SetAlbedoTexture(m_AlbedoMap);
  m_Material->SetNormalTexture(m_NormalMap);
  m_Material->SetMetallicTexture(m_MetalnessMap);
  m_Material->SetRoughnessTexture(m_RoughnessMap);
}

void MaterialAsset::SetAlbedoColor(const Vec3& color) {
  m_AlbedoColor = color;
  if (m_Material) m_Material->SetAlbedoColor(color);
}

void MaterialAsset::SetMetalness(f32 metalness) {
  m_Metalness = metalness;
  if (m_Material) m_Material->SetMetallic(metalness);
}

void MaterialAsset::SetRoughness(f32 roughness) {
  m_Roughness = roughness;
  if (m_Material) m_Material->SetRoughness(roughness);
}

void MaterialAsset::SetEmission(f32 emission) {
  m_Emission = emission;
}

void MaterialAsset::SetAlbedoMap(Ref<Texture2D> texture) {
  m_AlbedoMap = texture;
  if (m_Material) m_Material->SetAlbedoTexture(texture);
}

void MaterialAsset::SetNormalMap(Ref<Texture2D> texture) {
  m_NormalMap = texture;
  if (m_Material) m_Material->SetNormalTexture(texture);
}

void MaterialAsset::SetMetalnessMap(Ref<Texture2D> texture) {
  m_MetalnessMap = texture;
  if (m_Material) m_Material->SetMetallicTexture(texture);
}

void MaterialAsset::SetRoughnessMap(Ref<Texture2D> texture) {
  m_RoughnessMap = texture;
  if (m_Material) m_Material->SetRoughnessTexture(texture);
}

void MaterialAsset::Bind(Ref<Shader> shader, u32 textureSlotStart) {
  if (m_Material) {
    m_Material->Bind(shader, textureSlotStart);
  }
}

void MaterialAsset::Save(const std::filesystem::path& path) {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "MaterialAsset" << YAML::Value << YAML::BeginMap;
  
  out << YAML::Key << "Name" << YAML::Value << m_Name;
  
  out << YAML::Key << "AlbedoColor" << YAML::Value << YAML::Flow 
      << YAML::BeginSeq << m_AlbedoColor.x << m_AlbedoColor.y << m_AlbedoColor.z << YAML::EndSeq;
  out << YAML::Key << "Metalness" << YAML::Value << m_Metalness;
  out << YAML::Key << "Roughness" << YAML::Value << m_Roughness;
  out << YAML::Key << "Emission" << YAML::Value << m_Emission;
  
  if (!m_AlbedoMapPath.empty())
    out << YAML::Key << "AlbedoMapPath" << YAML::Value << m_AlbedoMapPath;
  if (!m_NormalMapPath.empty())
    out << YAML::Key << "NormalMapPath" << YAML::Value << m_NormalMapPath;
  if (!m_MetalnessMapPath.empty())
    out << YAML::Key << "MetalnessMapPath" << YAML::Value << m_MetalnessMapPath;
  if (!m_RoughnessMapPath.empty())
    out << YAML::Key << "RoughnessMapPath" << YAML::Value << m_RoughnessMapPath;
  
  out << YAML::EndMap;
  out << YAML::EndMap;
  
  std::ofstream fout(path);
  fout << out.c_str();
}

Ref<MaterialAsset> MaterialAsset::Load(const std::filesystem::path& path) {
  std::ifstream fin(path);
  if (!fin.is_open()) {
    GINI_ERROR("MaterialAsset::Load - Failed to open file: ", path.string());
    return nullptr;
  }
  
  YAML::Node data = YAML::Load(fin);
  if (!data["MaterialAsset"]) {
    GINI_ERROR("MaterialAsset::Load - Invalid material file: ", path.string());
    return nullptr;
  }
  
  auto matNode = data["MaterialAsset"];
  auto material = CreateRef<MaterialAsset>();
  
  if (matNode["Name"])
    material->m_Name = matNode["Name"].as<std::string>();
  
  if (matNode["AlbedoColor"]) {
    auto color = matNode["AlbedoColor"];
    material->m_AlbedoColor = Vec3(color[0].as<f32>(), color[1].as<f32>(), color[2].as<f32>());
  }
  
  if (matNode["Metalness"])
    material->m_Metalness = matNode["Metalness"].as<f32>();
  if (matNode["Roughness"])
    material->m_Roughness = matNode["Roughness"].as<f32>();
  if (matNode["Emission"])
    material->m_Emission = matNode["Emission"].as<f32>();
  
  if (matNode["AlbedoMapPath"])
    material->m_AlbedoMapPath = matNode["AlbedoMapPath"].as<std::string>();
  if (matNode["NormalMapPath"])
    material->m_NormalMapPath = matNode["NormalMapPath"].as<std::string>();
  if (matNode["MetalnessMapPath"])
    material->m_MetalnessMapPath = matNode["MetalnessMapPath"].as<std::string>();
  if (matNode["RoughnessMapPath"])
    material->m_RoughnessMapPath = matNode["RoughnessMapPath"].as<std::string>();
  
  return material;
}

void MaterialAssetLibrary::Add(u64 handle, Ref<MaterialAsset> material) {
  m_Materials[handle] = material;
  if (material) {
    material->SetHandle(handle);
  }
}

Ref<MaterialAsset> MaterialAssetLibrary::Get(u64 handle) {
  auto it = m_Materials.find(handle);
  if (it != m_Materials.end()) {
    return it->second;
  }
  return nullptr;
}

bool MaterialAssetLibrary::Exists(u64 handle) const {
  return m_Materials.find(handle) != m_Materials.end();
}

void MaterialAssetLibrary::Remove(u64 handle) {
  m_Materials.erase(handle);
}

void MaterialAssetLibrary::Clear() {
  m_Materials.clear();
}

} // namespace Gini
