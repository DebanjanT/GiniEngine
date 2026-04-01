#pragma once

#include "Core/Types.h"
#include <string>

namespace Gini {

class Scene;

class EditorPanel {
public:
  EditorPanel(const std::string &name) : m_Name(name) {}
  virtual ~EditorPanel() = default;

  virtual void OnImGuiRender() = 0;
  virtual void OnUpdate(f32 deltaTime) {}

  const std::string &GetName() const { return m_Name; }
  bool IsVisible() const { return m_Visible; }
  void SetVisible(bool visible) { m_Visible = visible; }
  void ToggleVisible() { m_Visible = !m_Visible; }

public:
  std::string m_Name;
  bool m_Visible = true;
};

} // namespace Gini
