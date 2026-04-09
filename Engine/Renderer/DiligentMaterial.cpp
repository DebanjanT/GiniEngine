#include "DiligentMaterial.h"
#include "../Core/Logger.h"
#include "DiligentBuffer.h"
#include "DiligentTexture.h"

namespace Gini {

DiligentMaterial::DiligentMaterial() : m_Name("Unnamed Material") {
  ResetToDefaults();
}

DiligentMaterial::DiligentMaterial(const std::string &name) : m_Name(name) {
  ResetToDefaults();
}

bool DiligentMaterial::Create(const DiligentPBRMaterial &material) {
  m_Material = material;
  UpdateMaterialFlags();

  if (!CreateConstantBuffer()) {
    GINI_ERROR("Failed to create constant buffer for material: " + m_Name);
    return false;
  }

  UpdateConstantBuffer();
  return true;
}

bool DiligentMaterial::CreatePBR(const glm::vec3 &albedo, float metallic,
                                 float roughness, float ao) {
  m_Material.Albedo = albedo;
  m_Material.Metallic = metallic;
  m_Material.Roughness = roughness;
  m_Material.AO = ao;

  UpdateMaterialFlags();

  if (!CreateConstantBuffer()) {
    GINI_ERROR("Failed to create constant buffer for PBR material: " + m_Name);
    return false;
  }

  UpdateConstantBuffer();
  return true;
}

void DiligentMaterial::SetAlbedoMap(Ref<Gini::DiligentTexture> texture) {
  m_Material.AlbedoMap = texture;
  m_Material.UseAlbedoMap = (texture != nullptr);
}

void DiligentMaterial::SetNormalMap(Ref<Gini::DiligentTexture> texture) {
  m_Material.NormalMap = texture;
  m_Material.UseNormalMap = (texture != nullptr);
}

void DiligentMaterial::SetMetallicMap(Ref<Gini::DiligentTexture> texture) {
  m_Material.MetallicMap = texture;
  m_Material.UseMetallicMap = (texture != nullptr);
}

void DiligentMaterial::SetRoughnessMap(Ref<Gini::DiligentTexture> texture) {
  m_Material.RoughnessMap = texture;
  m_Material.UseRoughnessMap = (texture != nullptr);
}

void DiligentMaterial::SetAOMap(Ref<Gini::DiligentTexture> texture) {
  m_Material.AOMap = texture;
  m_Material.UseAOMap = (texture != nullptr);
}

void DiligentMaterial::SetEmissiveMap(Ref<Gini::DiligentTexture> texture) {
  m_Material.EmissiveMap = texture;
  m_Material.UseEmissiveMap = (texture != nullptr);
}

void DiligentMaterial::SetAlbedoColor(const glm::vec3 &color) {
  m_Material.Albedo = color;
  UpdateConstantBuffer();
}

void DiligentMaterial::SetMetallic(float metallic) {
  m_Material.Metallic = glm::clamp(metallic, 0.0f, 1.0f);
  UpdateConstantBuffer();
}

void DiligentMaterial::SetRoughness(float roughness) {
  m_Material.Roughness = glm::clamp(roughness, 0.0f, 1.0f);
  UpdateConstantBuffer();
}

void DiligentMaterial::SetAO(float ao) {
  m_Material.AO = glm::clamp(ao, 0.0f, 1.0f);
  UpdateConstantBuffer();
}

void DiligentMaterial::SetEmissive(const glm::vec3 &emissive) {
  m_Material.Emissive = emissive;
  UpdateConstantBuffer();
}

void DiligentMaterial::SetAlpha(float alpha) {
  m_Material.Alpha = glm::clamp(alpha, 0.0f, 1.0f);
  m_Material.Transparent = (alpha < 1.0f);
  UpdateConstantBuffer();
}

void DiligentMaterial::SetAlphaCutoff(float cutoff) {
  m_Material.AlphaCutoff = glm::clamp(cutoff, 0.0f, 1.0f);
  UpdateConstantBuffer();
}

void DiligentMaterial::SetTransparent(bool transparent) {
  m_Material.Transparent = transparent;
  UpdateConstantBuffer();
}

void DiligentMaterial::SetDoubleSided(bool doubleSided) {
  m_Material.DoubleSided = doubleSided;
  UpdateConstantBuffer();
}

void DiligentMaterial::Bind(Diligent::IShaderResourceBinding *resourceBinding,
                            u32 materialSlot) {
  // TODO: Bind material resources when Diligent Engine is available
  GINI_WARN("DiligentMaterial::Bind deferred until Diligent Engine "
            "dependencies are resolved");
}

void DiligentMaterial::UpdateConstantBuffer() {
  // TODO: Update constant buffer data when Diligent Engine is available
  GINI_WARN("DiligentMaterial::UpdateConstantBuffer deferred until Diligent "
            "Engine dependencies are resolved");
}

void DiligentMaterial::ResetToDefaults() {
  m_Material = DiligentPBRMaterial();
  UpdateMaterialFlags();
}

bool DiligentMaterial::IsValid() const { return m_ConstantBuffer != nullptr; }

bool DiligentMaterial::CreateConstantBuffer() {
  // TODO: Create constant buffer when Diligent Engine is available
  GINI_WARN("DiligentMaterial::CreateConstantBuffer deferred until Diligent "
            "Engine dependencies are resolved");
  return false;
}

void DiligentMaterial::UpdateMaterialFlags() {
  // Update flags based on texture availability
  m_Material.UseAlbedoMap = (m_Material.AlbedoMap != nullptr);
  m_Material.UseNormalMap = (m_Material.NormalMap != nullptr);
  m_Material.UseMetallicMap = (m_Material.MetallicMap != nullptr);
  m_Material.UseRoughnessMap = (m_Material.RoughnessMap != nullptr);
  m_Material.UseAOMap = (m_Material.AOMap != nullptr);
  m_Material.UseEmissiveMap = (m_Material.EmissiveMap != nullptr);
}

// Material Factory Implementation
Ref<DiligentMaterial> DiligentMaterialFactory::CreateDefaultMaterial() {
  auto material = CreateRef<DiligentMaterial>("Default");
  material->CreatePBR(glm::vec3(1.0f), 0.0f, 0.5f, 1.0f);
  return material;
}

Ref<DiligentMaterial>
DiligentMaterialFactory::CreateMetalMaterial(const glm::vec3 &albedo) {
  auto material = CreateRef<DiligentMaterial>("Metal");
  material->CreatePBR(albedo, 1.0f, 0.2f, 1.0f);
  return material;
}

Ref<DiligentMaterial>
DiligentMaterialFactory::CreatePlasticMaterial(const glm::vec3 &albedo) {
  auto material = CreateRef<DiligentMaterial>("Plastic");
  material->CreatePBR(albedo, 0.0f, 0.8f, 1.0f);
  return material;
}

Ref<DiligentMaterial>
DiligentMaterialFactory::CreateGlassMaterial(const glm::vec3 &albedo) {
  auto material = CreateRef<DiligentMaterial>("Glass");
  material->CreatePBR(albedo, 0.0f, 0.0f, 1.0f);
  material->SetAlpha(0.1f);
  material->SetTransparent(true);
  return material;
}

Ref<DiligentMaterial>
DiligentMaterialFactory::CreateEmissiveMaterial(const glm::vec3 &emissive) {
  auto material = CreateRef<DiligentMaterial>("Emissive");
  material->CreatePBR(glm::vec3(0.0f), 0.0f, 0.8f, 1.0f);
  material->SetEmissive(emissive);
  return material;
}

Ref<DiligentMaterial>
DiligentMaterialFactory::CreateDebugMaterial(const glm::vec3 &color) {
  auto material = CreateRef<DiligentMaterial>("Debug");
  material->CreatePBR(color, 0.0f, 1.0f, 1.0f);
  return material;
}

Ref<DiligentMaterial>
DiligentMaterialFactory::CreateWireframeMaterial(const glm::vec3 &color) {
  auto material = CreateRef<DiligentMaterial>("Wireframe");
  material->CreatePBR(color, 0.0f, 1.0f, 1.0f);
  return material;
}

Ref<DiligentMaterial> DiligentMaterialFactory::CreateSkyboxMaterial() {
  auto material = CreateRef<DiligentMaterial>("Skybox");
  material->CreatePBR(glm::vec3(1.0f), 0.0f, 1.0f, 1.0f);
  return material;
}

Ref<DiligentMaterial> DiligentMaterialFactory::CreateTerrainMaterial() {
  auto material = CreateRef<DiligentMaterial>("Terrain");
  material->CreatePBR(glm::vec3(0.5f, 0.4f, 0.3f), 0.0f, 0.8f, 1.0f);
  return material;
}

Ref<DiligentMaterial> DiligentMaterialFactory::CreateWaterMaterial() {
  auto material = CreateRef<DiligentMaterial>("Water");
  material->CreatePBR(glm::vec3(0.1f, 0.3f, 0.5f), 0.0f, 0.02f, 1.0f);
  material->SetAlpha(0.8f);
  material->SetTransparent(true);
  return material;
}

Ref<DiligentMaterial> DiligentMaterialFactory::CreateFromTexture(
    Ref<Gini::DiligentTexture> albedoTexture) {
  auto material = CreateRef<DiligentMaterial>("FromTexture");
  material->CreatePBR(glm::vec3(1.0f), 0.0f, 0.5f, 1.0f);
  material->SetAlbedoMap(albedoTexture);
  return material;
}

Ref<DiligentMaterial> DiligentMaterialFactory::CreateFromTextures(
    Ref<Gini::DiligentTexture> albedo, Ref<Gini::DiligentTexture> normal,
    Ref<Gini::DiligentTexture> metallic, Ref<Gini::DiligentTexture> roughness,
    Ref<Gini::DiligentTexture> ao) {
  auto material = CreateRef<DiligentMaterial>("FromTextures");
  material->CreatePBR(glm::vec3(1.0f), 0.0f, 0.5f, 1.0f);

  if (albedo)
    material->SetAlbedoMap(albedo);
  if (normal)
    material->SetNormalMap(normal);
  if (metallic)
    material->SetMetallicMap(metallic);
  if (roughness)
    material->SetRoughnessMap(roughness);
  if (ao)
    material->SetAOMap(ao);

  return material;
}

// Material Library Implementation
DiligentMaterialLibrary &DiligentMaterialLibrary::GetInstance() {
  static DiligentMaterialLibrary instance;
  return instance;
}

DiligentMaterialLibrary::DiligentMaterialLibrary() {
  InitializeBuiltInMaterials();
}

Ref<DiligentMaterial>
DiligentMaterialLibrary::GetMaterial(const std::string &name) {
  auto it = m_Materials.find(name);
  return (it != m_Materials.end()) ? it->second : nullptr;
}

void DiligentMaterialLibrary::AddMaterial(const std::string &name,
                                          Ref<DiligentMaterial> material) {
  m_Materials[name] = material;
}

void DiligentMaterialLibrary::RemoveMaterial(const std::string &name) {
  m_Materials.erase(name);
}

void DiligentMaterialLibrary::ClearLibrary() { m_Materials.clear(); }

Ref<DiligentMaterial> DiligentMaterialLibrary::GetDefaultMaterial() {
  return m_DefaultMaterial;
}

Ref<DiligentMaterial> DiligentMaterialLibrary::GetDebugMaterial() {
  return m_DebugMaterial;
}

Ref<DiligentMaterial> DiligentMaterialLibrary::GetWireframeMaterial() {
  return m_WireframeMaterial;
}

void DiligentMaterialLibrary::InitializeBuiltInMaterials() {
  m_DefaultMaterial = DiligentMaterialFactory::CreateDefaultMaterial();
  m_DebugMaterial = DiligentMaterialFactory::CreateDebugMaterial();
  m_WireframeMaterial = DiligentMaterialFactory::CreateWireframeMaterial();

  AddMaterial("Default", m_DefaultMaterial);
  AddMaterial("Debug", m_DebugMaterial);
  AddMaterial("Wireframe", m_WireframeMaterial);
}

// Material Utilities Implementation
namespace DiligentMaterialUtils {

glm::vec3 CalculateFresnelReflectance(float cosTheta, const glm::vec3 &f0) {
  // TODO: Implement Fresnel reflectance calculation when Diligent Engine is
  // available
  GINI_WARN("DiligentMaterialUtils::CalculateFresnelReflectance deferred until "
            "Diligent Engine dependencies are resolved");
  return f0;
}

float CalculateDistributionGGX(const glm::vec3 &N, const glm::vec3 &H,
                               float roughness) {
  // TODO: Implement GGX distribution calculation when Diligent Engine is
  // available
  GINI_WARN("DiligentMaterialUtils::CalculateDistributionGGX deferred until "
            "Diligent Engine dependencies are resolved");
  return 0.0f;
}

float CalculateGeometrySmith(const glm::vec3 &N, const glm::vec3 &V,
                             const glm::vec3 &L, float roughness) {
  // TODO: Implement Smith geometry calculation when Diligent Engine is
  // available
  GINI_WARN("DiligentMaterialUtils::CalculateGeometrySmith deferred until "
            "Diligent Engine dependencies are resolved");
  return 0.0f;
}

glm::vec3 CalculatePBRBRDF(const glm::vec3 &N, const glm::vec3 &V,
                           const glm::vec3 &L, const glm::vec3 &albedo,
                           float metallic, float roughness) {
  // TODO: Implement PBR BRDF calculation when Diligent Engine is available
  GINI_WARN("DiligentMaterialUtils::CalculatePBRBRDF deferred until Diligent "
            "Engine dependencies are resolved");
  return albedo;
}

DiligentPBRMaterial
ConvertFromLegacyMaterial(const class Material3D &legacyMaterial) {
  // TODO: Implement legacy material conversion when Diligent Engine is
  // available
  GINI_WARN("DiligentMaterialUtils::ConvertFromLegacyMaterial deferred until "
            "Diligent Engine dependencies are resolved");
  return DiligentPBRMaterial();
}

DiligentPBRMaterial CreateMaterialFromPBRValues(const glm::vec3 &albedo,
                                                float metallic,
                                                float roughness) {
  DiligentPBRMaterial material;
  material.Albedo = albedo;
  material.Metallic = metallic;
  material.Roughness = roughness;
  material.AO = 1.0f;
  return material;
}

bool ValidateMaterial(const DiligentPBRMaterial &material) {
  // TODO: Implement material validation when Diligent Engine is available
  GINI_WARN("DiligentMaterialUtils::ValidateMaterial deferred until Diligent "
            "Engine dependencies are resolved");
  return true;
}

std::vector<std::string>
GetMaterialWarnings(const DiligentPBRMaterial &material) {
  // TODO: Implement material warnings when Diligent Engine is available
  GINI_WARN("DiligentMaterialUtils::GetMaterialWarnings deferred until "
            "Diligent Engine dependencies are resolved");
  return {};
}

std::string SerializeMaterial(const DiligentPBRMaterial &material) {
  // TODO: Implement material serialization when Diligent Engine is available
  GINI_WARN("DiligentMaterialUtils::SerializeMaterial deferred until Diligent "
            "Engine dependencies are resolved");
  return "";
}

DiligentPBRMaterial DeserializeMaterial(const std::string &data) {
  // TODO: Implement material deserialization when Diligent Engine is available
  GINI_WARN("DiligentMaterialUtils::DeserializeMaterial deferred until "
            "Diligent Engine dependencies are resolved");
  return DiligentPBRMaterial();
}

} // namespace DiligentMaterialUtils

} // namespace Gini
