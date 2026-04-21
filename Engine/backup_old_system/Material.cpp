#include "Material.h"
#include "../Core/Logger.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <random>

namespace Gini {

static u64 GenerateUUID() {
  static std::random_device rd;
  static std::mt19937_64 gen(rd());
  static std::uniform_int_distribution<u64> dis;
  return dis(gen);
}

Material::Material(const std::string& name)
    : m_Name(name), m_UUID(GenerateUUID()) {
}

Ref<Material> Material::Create(const std::string& name) {
  return CreateRef<Material>(name);
}

Ref<Material> Material::LoadFromDirectory(const std::filesystem::path& directory) {
  if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
    GINI_ERROR("Material directory not found: ", directory.string());
    return nullptr;
  }
  
  std::string materialName = directory.filename().string();
  Ref<Material> material = Create(materialName);
  material->SetPath(directory);
  
  // Search for texture files with common naming conventions
  for (const auto& entry : std::filesystem::directory_iterator(directory)) {
    if (!entry.is_regular_file()) continue;
    
    std::string filename = entry.path().filename().string();
    std::string lowerFilename = filename;
    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);
    
    // Check for albedo/diffuse/color
    if (lowerFilename.find("albedo") != std::string::npos ||
        lowerFilename.find("diffuse") != std::string::npos ||
        lowerFilename.find("color") != std::string::npos ||
        lowerFilename.find("basecolor") != std::string::npos) {
      material->SetAlbedoTexture(Texture2D::Create(entry.path().string()));
      GINI_INFO("Loaded albedo texture: ", filename);
    }
    // Check for normal
    else if (lowerFilename.find("normal") != std::string::npos) {
      material->SetNormalTexture(Texture2D::Create(entry.path().string()));
      GINI_INFO("Loaded normal texture: ", filename);
    }
    // Check for roughness
    else if (lowerFilename.find("roughness") != std::string::npos) {
      material->SetRoughnessTexture(Texture2D::Create(entry.path().string()));
      GINI_INFO("Loaded roughness texture: ", filename);
    }
    // Check for metallic/metalness
    else if (lowerFilename.find("metallic") != std::string::npos ||
             lowerFilename.find("metalness") != std::string::npos) {
      material->SetMetallicTexture(Texture2D::Create(entry.path().string()));
      GINI_INFO("Loaded metallic texture: ", filename);
    }
    // Check for AO/ambient occlusion
    else if (lowerFilename.find("ao") != std::string::npos ||
             lowerFilename.find("ambient") != std::string::npos ||
             lowerFilename.find("occlusion") != std::string::npos) {
      material->SetAOTexture(Texture2D::Create(entry.path().string()));
      GINI_INFO("Loaded AO texture: ", filename);
    }
    // Check for height/displacement
    else if (lowerFilename.find("height") != std::string::npos ||
             lowerFilename.find("displacement") != std::string::npos ||
             lowerFilename.find("disp") != std::string::npos) {
      material->SetHeightTexture(Texture2D::Create(entry.path().string()));
      GINI_INFO("Loaded height texture: ", filename);
    }
  }
  
  GINI_INFO("Loaded material from directory: ", materialName);
  return material;
}

Ref<Material> Material::Load(const std::filesystem::path& materialFile) {
  if (!std::filesystem::exists(materialFile)) {
    GINI_ERROR("Material file not found: ", materialFile.string());
    return nullptr;
  }
  
  try {
    YAML::Node data = YAML::LoadFile(materialFile.string());
    
    std::string name = data["Material"]["Name"].as<std::string>("Unnamed");
    Ref<Material> material = Create(name);
    material->SetPath(materialFile);
    
    if (data["Material"]["UUID"]) {
      material->SetUUID(data["Material"]["UUID"].as<u64>());
    }
    
    // Load fallback values
    if (data["Material"]["AlbedoColor"]) {
      auto color = data["Material"]["AlbedoColor"];
      material->SetAlbedoColor(Vec3(color[0].as<f32>(), color[1].as<f32>(), color[2].as<f32>()));
    }
    material->SetRoughness(data["Material"]["Roughness"].as<f32>(0.5f));
    material->SetMetallic(data["Material"]["Metallic"].as<f32>(0.0f));
    material->SetAO(data["Material"]["AO"].as<f32>(1.0f));
    material->SetHeightScale(data["Material"]["HeightScale"].as<f32>(0.05f));
    
    if (data["Material"]["Tiling"]) {
      auto tiling = data["Material"]["Tiling"];
      material->SetTiling(Vec2(tiling[0].as<f32>(), tiling[1].as<f32>()));
    }
    
    // Load textures (paths relative to material file)
    std::filesystem::path basePath = materialFile.parent_path();
    
    if (data["Material"]["AlbedoTexture"]) {
      std::string texPath = data["Material"]["AlbedoTexture"].as<std::string>();
      material->SetAlbedoTexture(Texture2D::Create((basePath / texPath).string()));
    }
    if (data["Material"]["NormalTexture"]) {
      std::string texPath = data["Material"]["NormalTexture"].as<std::string>();
      material->SetNormalTexture(Texture2D::Create((basePath / texPath).string()));
    }
    if (data["Material"]["RoughnessTexture"]) {
      std::string texPath = data["Material"]["RoughnessTexture"].as<std::string>();
      material->SetRoughnessTexture(Texture2D::Create((basePath / texPath).string()));
    }
    if (data["Material"]["MetallicTexture"]) {
      std::string texPath = data["Material"]["MetallicTexture"].as<std::string>();
      material->SetMetallicTexture(Texture2D::Create((basePath / texPath).string()));
    }
    if (data["Material"]["AOTexture"]) {
      std::string texPath = data["Material"]["AOTexture"].as<std::string>();
      material->SetAOTexture(Texture2D::Create((basePath / texPath).string()));
    }
    if (data["Material"]["HeightTexture"]) {
      std::string texPath = data["Material"]["HeightTexture"].as<std::string>();
      material->SetHeightTexture(Texture2D::Create((basePath / texPath).string()));
    }
    
    GINI_INFO("Loaded material: ", name);
    return material;
  } catch (const YAML::Exception& e) {
    GINI_ERROR("Failed to load material: ", e.what());
    return nullptr;
  }
}

void Material::Save(const std::filesystem::path& path) {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;
  
  out << YAML::Key << "Name" << YAML::Value << m_Name;
  out << YAML::Key << "UUID" << YAML::Value << m_UUID;
  
  // Fallback values
  out << YAML::Key << "AlbedoColor" << YAML::Value << YAML::Flow 
      << YAML::BeginSeq << m_AlbedoColor.x << m_AlbedoColor.y << m_AlbedoColor.z << YAML::EndSeq;
  out << YAML::Key << "Roughness" << YAML::Value << m_Roughness;
  out << YAML::Key << "Metallic" << YAML::Value << m_Metallic;
  out << YAML::Key << "AO" << YAML::Value << m_AO;
  out << YAML::Key << "HeightScale" << YAML::Value << m_HeightScale;
  out << YAML::Key << "Tiling" << YAML::Value << YAML::Flow 
      << YAML::BeginSeq << m_Tiling.x << m_Tiling.y << YAML::EndSeq;
  
  // Texture paths (relative to material file)
  // Note: In a full implementation, we'd store relative paths
  // For now, we'll just note if textures are present
  
  out << YAML::EndMap;
  out << YAML::EndMap;
  
  std::ofstream fout(path);
  fout << out.c_str();
  fout.close();
  
  m_Path = path;
  GINI_INFO("Saved material: ", m_Name, " to ", path.string());
}

void Material::Bind(Ref<Shader> shader, u32 textureSlotStart) {
  u32 slot = textureSlotStart;

  shader->SetVec3("u_Material_albedo", m_AlbedoColor);
  shader->SetFloat("u_Material_metallic", m_Metallic);
  shader->SetFloat("u_Material_roughness", m_Roughness);
  shader->SetFloat("u_Material_ao", m_AO);
  shader->SetVec3("u_Material_emissive", Vec3(0.0f));

  if (m_HasAlbedoTexture && m_AlbedoTexture) {
    m_AlbedoTexture->Bind(slot);
    shader->SetInt("u_AlbedoMap", slot);
    shader->SetInt("u_HasAlbedoMap", 1);
    slot++;
  } else {
    shader->SetInt("u_HasAlbedoMap", 0);
  }

  if (m_HasNormalTexture && m_NormalTexture) {
    m_NormalTexture->Bind(slot);
    shader->SetInt("u_NormalMap", slot);
    shader->SetInt("u_HasNormalMap", 1);
    slot++;
  } else {
    shader->SetInt("u_HasNormalMap", 0);
  }

  if (m_HasRoughnessTexture && m_RoughnessTexture) {
    m_RoughnessTexture->Bind(slot);
    shader->SetInt("u_RoughnessMap", slot);
    shader->SetInt("u_HasRoughnessMap", 1);
    slot++;
  } else {
    shader->SetInt("u_HasRoughnessMap", 0);
  }

  if (m_HasMetallicTexture && m_MetallicTexture) {
    m_MetallicTexture->Bind(slot);
    shader->SetInt("u_MetallicMap", slot);
    shader->SetInt("u_HasMetallicMap", 1);
    slot++;
  } else {
    shader->SetInt("u_HasMetallicMap", 0);
  }

  if (m_HasAOTexture && m_AOTexture) {
    m_AOTexture->Bind(slot);
    shader->SetInt("u_AOMap", slot);
    shader->SetInt("u_HasAOMap", 1);
    slot++;
  } else {
    shader->SetInt("u_HasAOMap", 0);
  }

  if (m_HasHeightTexture && m_HeightTexture) {
    m_HeightTexture->Bind(slot);
    shader->SetInt("u_HeightMap", slot);
    shader->SetInt("u_HasHeightMap", 1);
    shader->SetFloat("u_HeightScale", m_HeightScale);
    slot++;
  } else {
    shader->SetInt("u_HasHeightMap", 0);
  }

  shader->SetVec2("u_Tiling", m_Tiling);
}

void Material::Unbind() {
  // Unbind all textures
  if (m_AlbedoTexture) m_AlbedoTexture->Unbind();
  if (m_NormalTexture) m_NormalTexture->Unbind();
  if (m_RoughnessTexture) m_RoughnessTexture->Unbind();
  if (m_MetallicTexture) m_MetallicTexture->Unbind();
  if (m_AOTexture) m_AOTexture->Unbind();
  if (m_HeightTexture) m_HeightTexture->Unbind();
}

// MaterialLibrary implementation
Ref<Material> MaterialLibrary::Load(const std::string& name, const std::filesystem::path& path) {
  if (Exists(name)) {
    return Get(name);
  }
  
  Ref<Material> material;
  if (std::filesystem::is_directory(path)) {
    material = Material::LoadFromDirectory(path);
  } else {
    material = Material::Load(path);
  }
  
  if (material) {
    m_Materials[name] = material;
  }
  return material;
}

Ref<Material> MaterialLibrary::Get(const std::string& name) {
  auto it = m_Materials.find(name);
  if (it != m_Materials.end()) {
    return it->second;
  }
  return nullptr;
}

bool MaterialLibrary::Exists(const std::string& name) const {
  return m_Materials.find(name) != m_Materials.end();
}

void MaterialLibrary::Add(const std::string& name, Ref<Material> material) {
  m_Materials[name] = material;
}

void MaterialLibrary::Remove(const std::string& name) {
  m_Materials.erase(name);
}

void MaterialLibrary::Clear() {
  m_Materials.clear();
}

} // namespace Gini
