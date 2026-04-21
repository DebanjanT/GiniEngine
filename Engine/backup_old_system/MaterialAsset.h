#pragma once

#include "Core/Types.h"
#include "Renderer/Texture.h"
#include "Renderer/Shader.h"
#include "Renderer/Material.h"
#include <filesystem>
#include <string>
#include <unordered_map>

namespace Gini {

class MaterialAsset {
public:
  MaterialAsset(const std::string& name = "Default Material");
  ~MaterialAsset() = default;
  
  static Ref<MaterialAsset> Create(const std::string& name = "Default Material");
  
  Ref<Material> GetMaterial() const { return m_Material; }
  void SetMaterial(Ref<Material> material) { m_Material = material; }
  
  void SetAlbedoColor(const Vec3& color);
  void SetMetalness(f32 metalness);
  void SetRoughness(f32 roughness);
  void SetEmission(f32 emission);
  
  Vec3 GetAlbedoColor() const { return m_AlbedoColor; }
  f32 GetMetalness() const { return m_Metalness; }
  f32 GetRoughness() const { return m_Roughness; }
  f32 GetEmission() const { return m_Emission; }
  
  void SetAlbedoMap(Ref<Texture2D> texture);
  void SetNormalMap(Ref<Texture2D> texture);
  void SetMetalnessMap(Ref<Texture2D> texture);
  void SetRoughnessMap(Ref<Texture2D> texture);
  
  Ref<Texture2D> GetAlbedoMap() const { return m_AlbedoMap; }
  Ref<Texture2D> GetNormalMap() const { return m_NormalMap; }
  Ref<Texture2D> GetMetalnessMap() const { return m_MetalnessMap; }
  Ref<Texture2D> GetRoughnessMap() const { return m_RoughnessMap; }
  
  bool HasAlbedoMap() const { return m_AlbedoMap != nullptr; }
  bool HasNormalMap() const { return m_NormalMap != nullptr; }
  bool HasMetalnessMap() const { return m_MetalnessMap != nullptr; }
  bool HasRoughnessMap() const { return m_RoughnessMap != nullptr; }
  
  void SetAlbedoMapPath(const std::string& path) { m_AlbedoMapPath = path; }
  void SetNormalMapPath(const std::string& path) { m_NormalMapPath = path; }
  void SetMetalnessMapPath(const std::string& path) { m_MetalnessMapPath = path; }
  void SetRoughnessMapPath(const std::string& path) { m_RoughnessMapPath = path; }
  
  const std::string& GetAlbedoMapPath() const { return m_AlbedoMapPath; }
  const std::string& GetNormalMapPath() const { return m_NormalMapPath; }
  const std::string& GetMetalnessMapPath() const { return m_MetalnessMapPath; }
  const std::string& GetRoughnessMapPath() const { return m_RoughnessMapPath; }
  
  const std::string& GetName() const { return m_Name; }
  void SetName(const std::string& name) { m_Name = name; }
  
  u64 GetHandle() const { return m_Handle; }
  void SetHandle(u64 handle) { m_Handle = handle; }
  
  void Bind(Ref<Shader> shader, u32 textureSlotStart = 0);
  
  void Save(const std::filesystem::path& path);
  static Ref<MaterialAsset> Load(const std::filesystem::path& path);

private:
  void SyncToMaterial();
  
  std::string m_Name;
  u64 m_Handle = 0;
  
  Ref<Material> m_Material;
  
  Vec3 m_AlbedoColor{0.8f, 0.8f, 0.8f};
  f32 m_Metalness = 0.0f;
  f32 m_Roughness = 0.5f;
  f32 m_Emission = 0.0f;
  
  Ref<Texture2D> m_AlbedoMap;
  Ref<Texture2D> m_NormalMap;
  Ref<Texture2D> m_MetalnessMap;
  Ref<Texture2D> m_RoughnessMap;
  
  std::string m_AlbedoMapPath;
  std::string m_NormalMapPath;
  std::string m_MetalnessMapPath;
  std::string m_RoughnessMapPath;
};

class MaterialAssetLibrary {
public:
  static MaterialAssetLibrary& Get() {
    static MaterialAssetLibrary instance;
    return instance;
  }
  
  void Add(u64 handle, Ref<MaterialAsset> material);
  Ref<MaterialAsset> Get(u64 handle);
  bool Exists(u64 handle) const;
  void Remove(u64 handle);
  void Clear();

private:
  MaterialAssetLibrary() = default;
  std::unordered_map<u64, Ref<MaterialAsset>> m_Materials;
};

} // namespace Gini
