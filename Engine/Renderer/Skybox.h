#pragma once

#include "Core/Types.h"
#include "Texture.h"
#include "Shader.h"
#include "Camera3D.h"

namespace Gini {

class Skybox {
public:
    Skybox();
    ~Skybox();
    
    // Load from 6 face images
    void LoadFromFaces(const std::vector<std::string>& facePaths);
    
    // Load from HDR equirectangular image
    void LoadFromHDR(const std::string& hdrPath);
    
    // Set existing cubemap
    void SetCubemap(Ref<TextureCube> cubemap);
    
    // Render the skybox
    void Render(const Camera3D& camera);
    void Render(const Mat4& viewMatrix, const Mat4& projectionMatrix);
    
    // Settings
    void SetIntensity(f32 intensity) { m_Intensity = intensity; }
    void SetLod(f32 lod) { m_Lod = lod; }
    f32 GetIntensity() const { return m_Intensity; }
    f32 GetLod() const { return m_Lod; }
    
    Ref<TextureCube> GetCubemap() const { return m_Cubemap; }
    bool IsLoaded() const { return m_Cubemap != nullptr; }
    
    static Ref<Skybox> Create();
    static Ref<Skybox> CreateFromHDR(const std::string& hdrPath);
    static Ref<Skybox> CreateFromFaces(const std::vector<std::string>& facePaths);
    
private:
    void InitCube();
    void InitShader();
    
    Ref<TextureCube> m_Cubemap;
    Ref<Shader> m_Shader;
    
    u32 m_CubeVAO = 0;
    u32 m_CubeVBO = 0;
    
    f32 m_Intensity = 1.0f;
    f32 m_Lod = 0.0f;
};

} // namespace Gini
