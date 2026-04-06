#pragma once

#include "EditorPanel.h"
#include "Scene/Scene.h"
#include <string>

namespace Gini {

class ScenePropertiesPanel : public EditorPanel {
public:
  ScenePropertiesPanel();
  ~ScenePropertiesPanel() = default;

  void OnImGuiRender() override;

  void SetScene(Ref<Scene> scene) { m_Scene = scene; }
  Ref<Scene> GetScene() const { return m_Scene; }

private:
  Ref<Scene> m_Scene;
};

} // namespace Gini
