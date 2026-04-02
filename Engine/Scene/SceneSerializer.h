#pragma once

#include "Scene.h"
#include "Core/Types.h"
#include <string>

namespace Gini {

class SceneSerializer {
public:
    SceneSerializer(const Ref<Scene>& scene);
    
    // Serialize scene to YAML file
    void Serialize(const std::string& filepath);
    
    // Serialize to YAML string
    std::string SerializeToString();
    
    // Deserialize scene from YAML file
    bool Deserialize(const std::string& filepath);
    
    // Deserialize from YAML string
    bool DeserializeFromString(const std::string& yamlString);
    
private:
    void SerializeEntity(void* emitter, Entity entity);
    
    Ref<Scene> m_Scene;
};

} // namespace Gini
