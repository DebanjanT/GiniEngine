#pragma once

#include "Texture.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace Gini {
// Forward declarations for Diligent Engine integration
class DiligentMaterial;

// Improved PBR material structure based on Hazel's design
struct PBRMaterial {
  // Base properties
  glm::vec3 Albedo = glm::vec3(1.0f);
  float Metallic = 0.0f;
  float Roughness = 0.5f;
  float AO = 1.0f;
  glm::vec3 Emissive = glm::vec3(0.0f);

  // Texture maps
  Ref<Texture> AlbedoMap = nullptr;
  Ref<Texture> NormalMap = nullptr;
  Ref<Texture> MetallicMap = nullptr;
  Ref<Texture> RoughnessMap = nullptr;
  Ref<Texture> AOMap = nullptr;
  Ref<Texture> EmissiveMap = nullptr;

  // Texture usage flags
  bool UseAlbedoMap = false;
  bool UseNormalMap = false;
  bool UseMetallicMap = false;
  bool UseRoughnessMap = false;
  bool UseAOMap = false;
  bool UseEmissiveMap = false;

  // PBR workflow settings
  bool InvertRoughness = false; // For specular workflow conversion

  PBRMaterial() = default;

  // Helper methods
  void SetAlbedoMap(Ref<Texture> texture) {
    AlbedoMap = texture;
    UseAlbedoMap = (texture != nullptr);
  }

  void SetNormalMap(Ref<Texture> texture) {
    NormalMap = texture;
    UseNormalMap = (texture != nullptr);
  }

  void SetMetallicMap(Ref<Texture> texture) {
    MetallicMap = texture;
    UseMetallicMap = (texture != nullptr);
  }

  void SetRoughnessMap(Ref<Texture> texture, bool invert = false) {
    RoughnessMap = texture;
    UseRoughnessMap = (texture != nullptr);
    InvertRoughness = invert;
  }

  void SetAOMap(Ref<Texture> texture) {
    AOMap = texture;
    UseAOMap = (texture != nullptr);
  }

  void SetEmissiveMap(Ref<Texture> texture) {
    EmissiveMap = texture;
    UseEmissiveMap = (texture != nullptr);
  }

  // Reset to default values
  void Reset() {
    Albedo = glm::vec3(1.0f);
    Metallic = 0.0f;
    Roughness = 0.5f;
    AO = 1.0f;
    Emissive = glm::vec3(0.0f);

    AlbedoMap = nullptr;
    NormalMap = nullptr;
    MetallicMap = nullptr;
    RoughnessMap = nullptr;
    AOMap = nullptr;
    EmissiveMap = nullptr;

    UseAlbedoMap = false;
    UseNormalMap = false;
    UseMetallicMap = false;
    UseRoughnessMap = false;
    UseAOMap = false;
    UseEmissiveMap = false;

    InvertRoughness = false;
  }
};

// Material asset wrapper for better asset management
class MaterialAsset {
public:
  static Ref<MaterialAsset> Create(const std::string &name = "") {
    return CreateRef<MaterialAsset>(name);
  }

  MaterialAsset(const std::string &name = "") : m_Name(name) {}

  // Getters
  const std::string &GetName() const { return m_Name; }
  PBRMaterial &GetMaterial() { return m_Material; }
  const PBRMaterial &GetMaterial() const { return m_Material; }

  // Convenience methods
  void SetAlbedoColor(const glm::vec3 &color) { m_Material.Albedo = color; }
  void SetMetallic(float metallic) { m_Material.Metallic = metallic; }
  void SetRoughness(float roughness) { m_Material.Roughness = roughness; }
  void SetEmission(float emission) {
    m_Material.Emissive = glm::vec3(emission);
  }

  void SetAlbedoMap(Ref<Texture> texture) { m_Material.SetAlbedoMap(texture); }
  void SetNormalMap(Ref<Texture> texture) { m_Material.SetNormalMap(texture); }
  void SetMetallicMap(Ref<Texture> texture) {
    m_Material.SetMetallicMap(texture);
  }
  void SetRoughnessMap(Ref<Texture> texture, bool invert = false) {
    m_Material.SetRoughnessMap(texture, invert);
  }
  void SetAOMap(Ref<Texture> texture) { m_Material.SetAOMap(texture); }
  void SetEmissiveMap(Ref<Texture> texture) {
    m_Material.SetEmissiveMap(texture);
  }

  // Diligent Engine integration methods
  Ref<DiligentMaterial> ToDiligentMaterial() const;
  void FromDiligentMaterial(const Ref<DiligentMaterial> &diligentMaterial);

  // Hybrid resource management
  void RegisterWithHybridManager();
  Ref<DiligentMaterial> GetDiligentMaterial() const;

private:
  std::string m_Name;
  PBRMaterial m_Material;
  mutable Ref<DiligentMaterial>
      m_DiligentMaterial; // Cached Diligent Engine material
};

} // namespace Gini
