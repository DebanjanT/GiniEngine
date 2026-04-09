#pragma once

// Define platform for Diligent Engine
#define PLATFORM_MACOS 1

#include "../Core/Types.h"
#include <memory>
#include <string>
#include <vector>

// Diligent Engine includes
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Common/interface/RefCntAutoPtr.hpp"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Shader.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h"
#include "../../ThirdParty/DiligentEngine_v2.5.6/DiligentCore/Graphics/GraphicsEngine/interface/Texture.h"

// Forward declarations
namespace Gini {
class DiligentTexture;
class DiligentBuffer;
} // namespace Gini

namespace Gini {

// PBR material structure for Diligent Engine
struct DiligentPBRMaterial {
  // Base properties
  glm::vec3 Albedo = glm::vec3(1.0f);
  float Metallic = 0.0f;
  float Roughness = 0.5f;
  float AO = 1.0f;
  glm::vec3 Emissive = glm::vec3(0.0f);

  // Texture maps
  Ref<Gini::DiligentTexture> AlbedoMap;
  Ref<Gini::DiligentTexture> NormalMap;
  Ref<Gini::DiligentTexture> MetallicMap;
  Ref<Gini::DiligentTexture> RoughnessMap;
  Ref<Gini::DiligentTexture> AOMap;
  Ref<Gini::DiligentTexture> EmissiveMap;

  // Material flags
  bool UseAlbedoMap = false;
  bool UseNormalMap = false;
  bool UseMetallicMap = false;
  bool UseRoughnessMap = false;
  bool UseAOMap = false;
  bool UseEmissiveMap = false;

  // Material properties
  float Alpha = 1.0f;
  float AlphaCutoff = 0.5f;
  bool Transparent = false;
  bool DoubleSided = false;
};

// Diligent Engine material wrapper
class DiligentMaterial {
public:
  DiligentMaterial();
  DiligentMaterial(const std::string &name);
  ~DiligentMaterial() = default;

  // Material creation and management
  bool Create(const DiligentPBRMaterial &material);
  bool CreatePBR(const glm::vec3 &albedo = glm::vec3(1.0f),
                 float metallic = 0.0f, float roughness = 0.5f,
                 float ao = 1.0f);

  // Texture management
  void SetAlbedoMap(Ref<Gini::DiligentTexture> texture);
  void SetNormalMap(Ref<Gini::DiligentTexture> texture);
  void SetMetallicMap(Ref<Gini::DiligentTexture> texture);
  void SetRoughnessMap(Ref<Gini::DiligentTexture> texture);
  void SetAOMap(Ref<Gini::DiligentTexture> texture);
  void SetEmissiveMap(Ref<Gini::DiligentTexture> texture);

  // Material properties
  void SetAlbedoColor(const glm::vec3 &color);
  void SetMetallic(float metallic);
  void SetRoughness(float roughness);
  void SetAO(float ao);
  void SetEmissive(const glm::vec3 &emissive);
  void SetAlpha(float alpha);
  void SetAlphaCutoff(float cutoff);
  void SetTransparent(bool transparent);
  void SetDoubleSided(bool doubleSided);

  // Resource binding
  void Bind(Diligent::IShaderResourceBinding *resourceBinding,
            u32 materialSlot = 0);
  void UpdateConstantBuffer();

  // Material utilities
  void ResetToDefaults();
  bool IsValid() const;

  // Getters
  const DiligentPBRMaterial &GetMaterialData() const { return m_Material; }
  const std::string &GetName() const { return m_Name; }
  Diligent::IBuffer *GetConstantBuffer() const { return m_ConstantBuffer; }

  // Material properties accessors
  const glm::vec3 &GetAlbedoColor() const { return m_Material.Albedo; }
  float GetMetallic() const { return m_Material.Metallic; }
  float GetRoughness() const { return m_Material.Roughness; }
  float GetAO() const { return m_Material.AO; }
  const glm::vec3 &GetEmissive() const { return m_Material.Emissive; }
  float GetAlpha() const { return m_Material.Alpha; }
  bool IsTransparent() const { return m_Material.Transparent; }
  bool IsDoubleSided() const { return m_Material.DoubleSided; }

  // Texture accessors
  Ref<Gini::DiligentTexture> GetAlbedoMap() const {
    return m_Material.AlbedoMap;
  }
  Ref<Gini::DiligentTexture> GetNormalMap() const {
    return m_Material.NormalMap;
  }
  Ref<Gini::DiligentTexture> GetMetallicMap() const {
    return m_Material.MetallicMap;
  }
  Ref<Gini::DiligentTexture> GetRoughnessMap() const {
    return m_Material.RoughnessMap;
  }
  Ref<Gini::DiligentTexture> GetAOMap() const { return m_Material.AOMap; }
  Ref<Gini::DiligentTexture> GetEmissiveMap() const {
    return m_Material.EmissiveMap;
  }

private:
  std::string m_Name;
  DiligentPBRMaterial m_Material;

  // Diligent Engine resources
  Diligent::RefCntAutoPtr<Diligent::IBuffer> m_ConstantBuffer;

  // Resource binding slots
  static const u32 ALBEDO_TEXTURE_SLOT = 0;
  static const u32 NORMAL_TEXTURE_SLOT = 1;
  static const u32 METALLIC_TEXTURE_SLOT = 2;
  static const u32 ROUGHNESS_TEXTURE_SLOT = 3;
  static const u32 AO_TEXTURE_SLOT = 4;
  static const u32 EMISSIVE_TEXTURE_SLOT = 5;
  static const u32 MATERIAL_CB_SLOT = 0;

  // Internal methods
  bool CreateConstantBuffer();
  void UpdateMaterialFlags();
};

// Material factory for creating common material types
class DiligentMaterialFactory {
public:
  // Standard PBR materials
  static Ref<DiligentMaterial> CreateDefaultMaterial();
  static Ref<DiligentMaterial>
  CreateMetalMaterial(const glm::vec3 &albedo = glm::vec3(0.7f, 0.7f, 0.8f));
  static Ref<DiligentMaterial>
  CreatePlasticMaterial(const glm::vec3 &albedo = glm::vec3(0.8f, 0.8f, 0.8f));
  static Ref<DiligentMaterial>
  CreateGlassMaterial(const glm::vec3 &albedo = glm::vec3(0.1f, 0.1f, 0.1f));
  static Ref<DiligentMaterial> CreateEmissiveMaterial(
      const glm::vec3 &emissive = glm::vec3(1.0f, 1.0f, 1.0f));

  // Utility materials
  static Ref<DiligentMaterial>
  CreateDebugMaterial(const glm::vec3 &color = glm::vec3(1.0f, 0.0f, 0.0f));
  static Ref<DiligentMaterial>
  CreateWireframeMaterial(const glm::vec3 &color = glm::vec3(0.0f, 1.0f, 0.0f));

  // Environment materials
  static Ref<DiligentMaterial> CreateSkyboxMaterial();
  static Ref<DiligentMaterial> CreateTerrainMaterial();
  static Ref<DiligentMaterial> CreateWaterMaterial();

  // Material from texture
  static Ref<DiligentMaterial>
  CreateFromTexture(Ref<Gini::DiligentTexture> albedoTexture);
  static Ref<DiligentMaterial>
  CreateFromTextures(Ref<Gini::DiligentTexture> albedo,
                     Ref<Gini::DiligentTexture> normal = nullptr,
                     Ref<Gini::DiligentTexture> metallic = nullptr,
                     Ref<Gini::DiligentTexture> roughness = nullptr,
                     Ref<Gini::DiligentTexture> ao = nullptr);
};

// Material library for managing materials
class DiligentMaterialLibrary {
public:
  static DiligentMaterialLibrary &GetInstance();

  Ref<DiligentMaterial> GetMaterial(const std::string &name);
  void AddMaterial(const std::string &name, Ref<DiligentMaterial> material);
  void RemoveMaterial(const std::string &name);
  void ClearLibrary();

  // Built-in materials
  Ref<DiligentMaterial> GetDefaultMaterial();
  Ref<DiligentMaterial> GetDebugMaterial();
  Ref<DiligentMaterial> GetWireframeMaterial();

  u32 GetMaterialCount() const { return m_Materials.size(); }

private:
  DiligentMaterialLibrary();
  void InitializeBuiltInMaterials();

  std::unordered_map<std::string, Ref<DiligentMaterial>> m_Materials;
  Ref<DiligentMaterial> m_DefaultMaterial;
  Ref<DiligentMaterial> m_DebugMaterial;
  Ref<DiligentMaterial> m_WireframeMaterial;
};

// Material utilities
namespace DiligentMaterialUtils {
// Material property calculations
glm::vec3 CalculateFresnelReflectance(float cosTheta, const glm::vec3 &f0);
float CalculateDistributionGGX(const glm::vec3 &N, const glm::vec3 &H,
                               float roughness);
float CalculateGeometrySmith(const glm::vec3 &N, const glm::vec3 &V,
                             const glm::vec3 &L, float roughness);
glm::vec3 CalculatePBRBRDF(const glm::vec3 &N, const glm::vec3 &V,
                           const glm::vec3 &L, const glm::vec3 &albedo,
                           float metallic, float roughness);

// Material conversions
DiligentPBRMaterial
ConvertFromLegacyMaterial(const class Material3D &legacyMaterial);
DiligentPBRMaterial CreateMaterialFromPBRValues(const glm::vec3 &albedo,
                                                float metallic,
                                                float roughness);

// Material validation
bool ValidateMaterial(const DiligentPBRMaterial &material);
std::vector<std::string>
GetMaterialWarnings(const DiligentPBRMaterial &material);

// Material serialization
std::string SerializeMaterial(const DiligentPBRMaterial &material);
DiligentPBRMaterial DeserializeMaterial(const std::string &data);
} // namespace DiligentMaterialUtils

} // namespace Gini
