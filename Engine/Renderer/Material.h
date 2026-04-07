#pragma once

#include "../Core/Types.h"
#include "Shader.h"
#include "Texture.h"
#include <filesystem>
#include <string>
#include <unordered_map>

namespace Gini {

// PBR Material with full texture support
class Material {
public:
  Material(const std::string &name = "Default Material");
  ~Material() = default;

  static Ref<Material> Create(const std::string &name = "Default Material");
  static Ref<Material>
  LoadFromDirectory(const std::filesystem::path &directory);
  static Ref<Material> Load(const std::filesystem::path &materialFile);
  void Save(const std::filesystem::path &path);

  // Texture setters
  void SetAlbedoTexture(Ref<Texture2D> texture) {
    m_AlbedoTexture = texture;
    m_HasAlbedoTexture = texture != nullptr;
  }
  void SetNormalTexture(Ref<Texture2D> texture) {
    m_NormalTexture = texture;
    m_HasNormalTexture = texture != nullptr;
  }
  void SetRoughnessTexture(Ref<Texture2D> texture) {
    m_RoughnessTexture = texture;
    m_HasRoughnessTexture = texture != nullptr;
  }
  void SetMetallicTexture(Ref<Texture2D> texture) {
    m_MetallicTexture = texture;
    m_HasMetallicTexture = texture != nullptr;
  }
  void SetAOTexture(Ref<Texture2D> texture) {
    m_AOTexture = texture;
    m_HasAOTexture = texture != nullptr;
  }
  void SetHeightTexture(Ref<Texture2D> texture) {
    m_HeightTexture = texture;
    m_HasHeightTexture = texture != nullptr;
  }

  // Texture getters
  Ref<Texture2D> GetAlbedoTexture() const { return m_AlbedoTexture; }
  Ref<Texture2D> GetNormalTexture() const { return m_NormalTexture; }
  Ref<Texture2D> GetRoughnessTexture() const { return m_RoughnessTexture; }
  Ref<Texture2D> GetMetallicTexture() const { return m_MetallicTexture; }
  Ref<Texture2D> GetAOTexture() const { return m_AOTexture; }
  Ref<Texture2D> GetHeightTexture() const { return m_HeightTexture; }

  // Has texture checks
  bool HasAlbedoTexture() const { return m_HasAlbedoTexture; }
  bool HasNormalTexture() const { return m_HasNormalTexture; }
  bool HasRoughnessTexture() const { return m_HasRoughnessTexture; }
  bool HasMetallicTexture() const { return m_HasMetallicTexture; }
  bool HasAOTexture() const { return m_HasAOTexture; }
  bool HasHeightTexture() const { return m_HasHeightTexture; }

  // Fallback values (used when textures are not present)
  void SetAlbedoColor(const Vec3 &color) { m_AlbedoColor = color; }
  void SetAlbedo(const Vec4 &color) {
    m_AlbedoColor = Vec3(color.x, color.y, color.z);
  }
  void SetRoughness(f32 roughness) { m_Roughness = roughness; }
  void SetMetallic(f32 metallic) { m_Metallic = metallic; }
  void SetAO(f32 ao) { m_AO = ao; }
  void SetHeightScale(f32 scale) { m_HeightScale = scale; }
  void SetTiling(const Vec2 &tiling) { m_Tiling = tiling; }

  Vec3 GetAlbedoColor() const { return m_AlbedoColor; }
  Vec4 GetAlbedo() const {
    return Vec4(m_AlbedoColor.x, m_AlbedoColor.y, m_AlbedoColor.z, 1.0f);
  }
  f32 GetRoughness() const { return m_Roughness; }
  f32 GetMetallic() const { return m_Metallic; }
  f32 GetAO() const { return m_AO; }
  f32 GetHeightScale() const { return m_HeightScale; }
  Vec2 GetTiling() const { return m_Tiling; }

  // Texture path getters/setters for serialization
  void SetAlbedoTexturePath(const std::string &path) {
    m_AlbedoTexturePath = path;
  }
  void SetNormalTexturePath(const std::string &path) {
    m_NormalTexturePath = path;
  }
  void SetHeightTexturePath(const std::string &path) {
    m_HeightTexturePath = path;
  }
  void SetRoughnessTexturePath(const std::string &path) {
    m_RoughnessTexturePath = path;
  }
  void SetMetallicTexturePath(const std::string &path) {
    m_MetallicTexturePath = path;
  }
  void SetAOTexturePath(const std::string &path) { m_AOTexturePath = path; }
  std::string GetAlbedoTexturePath() const { return m_AlbedoTexturePath; }
  std::string GetNormalTexturePath() const { return m_NormalTexturePath; }
  std::string GetHeightTexturePath() const { return m_HeightTexturePath; }
  std::string GetRoughnessTexturePath() const { return m_RoughnessTexturePath; }
  std::string GetMetallicTexturePath() const { return m_MetallicTexturePath; }
  std::string GetAOTexturePath() const { return m_AOTexturePath; }

  // Bind material to shader
  void Bind(Ref<Shader> shader, u32 textureSlotStart = 0);
  void Unbind();

  // Properties
  const std::string &GetName() const { return m_Name; }
  void SetName(const std::string &name) { m_Name = name; }

  const std::filesystem::path &GetPath() const { return m_Path; }
  void SetPath(const std::filesystem::path &path) { m_Path = path; }

  // UUID for asset tracking
  u64 GetUUID() const { return m_UUID; }
  void SetUUID(u64 uuid) { m_UUID = uuid; }

private:
  std::string m_Name;
  std::filesystem::path m_Path;
  u64 m_UUID = 0;

  // PBR Textures
  Ref<Texture2D> m_AlbedoTexture;
  Ref<Texture2D> m_NormalTexture;
  Ref<Texture2D> m_RoughnessTexture;
  Ref<Texture2D> m_MetallicTexture;
  Ref<Texture2D> m_AOTexture;
  Ref<Texture2D> m_HeightTexture;

  // Texture presence flags
  bool m_HasAlbedoTexture = false;
  bool m_HasNormalTexture = false;
  bool m_HasRoughnessTexture = false;
  bool m_HasMetallicTexture = false;
  bool m_HasAOTexture = false;
  bool m_HasHeightTexture = false;

  // Fallback values
  Vec3 m_AlbedoColor = Vec3(0.8f, 0.8f, 0.8f);
  f32 m_Roughness = 0.5f;
  f32 m_Metallic = 0.0f;
  f32 m_AO = 1.0f;
  f32 m_HeightScale = 0.05f;
  Vec2 m_Tiling = Vec2(1.0f, 1.0f);

  // Texture paths for serialization
  std::string m_AlbedoTexturePath;
  std::string m_NormalTexturePath;
  std::string m_HeightTexturePath;
  std::string m_RoughnessTexturePath;
  std::string m_MetallicTexturePath;
  std::string m_AOTexturePath;
};

// Material Library - manages all loaded materials
class MaterialLibrary {
public:
  static MaterialLibrary &Get() {
    static MaterialLibrary instance;
    return instance;
  }

  Ref<Material> Load(const std::string &name,
                     const std::filesystem::path &path);
  Ref<Material> Get(const std::string &name);
  bool Exists(const std::string &name) const;
  void Add(const std::string &name, Ref<Material> material);
  void Remove(const std::string &name);
  void Clear();

  const std::unordered_map<std::string, Ref<Material>> &GetAll() const {
    return m_Materials;
  }

private:
  MaterialLibrary() = default;
  std::unordered_map<std::string, Ref<Material>> m_Materials;
};

} // namespace Gini
