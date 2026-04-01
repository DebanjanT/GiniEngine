#pragma once

#include "EditorPanel.h"
#include "Scene/Scene.h"

namespace Gini {

class SceneHierarchyPanel : public EditorPanel {
public:
    SceneHierarchyPanel();
    
    void OnImGuiRender() override;
    
    void SetScene(Ref<Scene> scene) { m_Scene = scene; }
    void SetSelectedEntity(Entity entity) { m_SelectedEntity = entity; }
    Entity GetSelectedEntity() const { return m_SelectedEntity; }
    
    using SelectionCallback = std::function<void(Entity)>;
    void SetSelectionCallback(SelectionCallback callback) { m_SelectionCallback = callback; }
    
private:
    void DrawEntityNode(Entity entity);
    void DrawContextMenu();
    
    Ref<Scene> m_Scene;
    Entity m_SelectedEntity = NullEntity;
    SelectionCallback m_SelectionCallback;
};

} // namespace Gini
