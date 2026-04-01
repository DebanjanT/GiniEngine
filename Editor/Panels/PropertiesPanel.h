#pragma once

#include "EditorPanel.h"
#include "Scene/Scene.h"

namespace Gini {

class PropertiesPanel : public EditorPanel {
public:
    PropertiesPanel();
    
    void OnImGuiRender() override;
    
    void SetScene(Ref<Scene> scene) { m_Scene = scene; }
    void SetSelectedEntity(Entity entity) { m_SelectedEntity = entity; }
    
private:
    void DrawComponents(Entity entity);
    
    template<typename T>
    void DrawComponent(const std::string& name, Entity entity, std::function<void(T&)> uiFunction);
    
    void DrawVec3Control(const std::string& label, Vec3& values, f32 resetValue = 0.0f, f32 columnWidth = 100.0f);
    
    Ref<Scene> m_Scene;
    Entity m_SelectedEntity = NullEntity;
};

} // namespace Gini
