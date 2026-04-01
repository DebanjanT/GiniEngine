#include "Light.h"
#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>

namespace Gini {

void LightManager::Clear() {
    m_HasDirectionalLight = false;
    m_PointLights.clear();
    m_SpotLights.clear();
    m_AmbientLight = AmbientLight();
}

void LightManager::SetDirectionalLight(const DirectionalLight& light) {
    m_DirectionalLight = light;
    m_HasDirectionalLight = true;
}

void LightManager::AddPointLight(const PointLight& light) {
    if (m_PointLights.size() < MAX_POINT_LIGHTS) {
        m_PointLights.push_back(light);
    }
}

void LightManager::RemovePointLight(u32 index) {
    if (index < m_PointLights.size()) {
        m_PointLights.erase(m_PointLights.begin() + index);
    }
}

void LightManager::AddSpotLight(const SpotLight& light) {
    if (m_SpotLights.size() < MAX_SPOT_LIGHTS) {
        m_SpotLights.push_back(light);
    }
}

void LightManager::RemoveSpotLight(u32 index) {
    if (index < m_SpotLights.size()) {
        m_SpotLights.erase(m_SpotLights.begin() + index);
    }
}

void LightManager::UploadToShader(Shader* shader) const {
    if (!shader) return;
    
    // Ambient light
    shader->SetVec3("u_AmbientLight.color", m_AmbientLight.color);
    shader->SetFloat("u_AmbientLight.intensity", m_AmbientLight.intensity);
    
    // Directional light
    shader->SetInt("u_HasDirectionalLight", m_HasDirectionalLight ? 1 : 0);
    if (m_HasDirectionalLight) {
        shader->SetVec3("u_DirectionalLight.direction", glm::normalize(m_DirectionalLight.direction));
        shader->SetVec3("u_DirectionalLight.color", m_DirectionalLight.color);
        shader->SetFloat("u_DirectionalLight.intensity", m_DirectionalLight.intensity);
    }
    
    // Point lights
    shader->SetInt("u_PointLightCount", static_cast<i32>(m_PointLights.size()));
    for (u32 i = 0; i < m_PointLights.size(); i++) {
        std::string prefix = "u_PointLights[" + std::to_string(i) + "].";
        const auto& light = m_PointLights[i];
        
        shader->SetVec3(prefix + "position", light.position);
        shader->SetVec3(prefix + "color", light.color);
        shader->SetFloat(prefix + "intensity", light.intensity);
        shader->SetFloat(prefix + "constant", light.constant);
        shader->SetFloat(prefix + "linear", light.linear);
        shader->SetFloat(prefix + "quadratic", light.quadratic);
        shader->SetFloat(prefix + "radius", light.radius);
    }
    
    // Spot lights
    shader->SetInt("u_SpotLightCount", static_cast<i32>(m_SpotLights.size()));
    for (u32 i = 0; i < m_SpotLights.size(); i++) {
        std::string prefix = "u_SpotLights[" + std::to_string(i) + "].";
        const auto& light = m_SpotLights[i];
        
        shader->SetVec3(prefix + "position", light.position);
        shader->SetVec3(prefix + "direction", glm::normalize(light.direction));
        shader->SetVec3(prefix + "color", light.color);
        shader->SetFloat(prefix + "intensity", light.intensity);
        shader->SetFloat(prefix + "innerCutoff", glm::cos(glm::radians(light.innerCutoff)));
        shader->SetFloat(prefix + "outerCutoff", glm::cos(glm::radians(light.outerCutoff)));
        shader->SetFloat(prefix + "constant", light.constant);
        shader->SetFloat(prefix + "linear", light.linear);
        shader->SetFloat(prefix + "quadratic", light.quadratic);
    }
}

} // namespace Gini
