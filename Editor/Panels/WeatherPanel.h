#pragma once

#include "EditorPanel.h"
#include "Particles/RainSystem.h"
#include <imgui.h>

namespace Gini {

class WeatherPanel {
public:
  WeatherPanel();
  ~WeatherPanel() = default;

  void OnImGuiRender();
  void OnUpdate(f32 deltaTime);

  bool IsVisible() const { return m_Visible; }
  void SetVisible(bool visible) { m_Visible = visible; }
  void ToggleVisible() { m_Visible = !m_Visible; }
  bool &GetVisibleRef() { return m_Visible; }

private:
  void DrawRainControls();
  void DrawThunderControls();
  void DrawWeatherPresets();

  bool m_Visible = false;

  // Weather state
  f32 m_WeatherIntensity = 0.0f;
  bool m_IsRaining = false;
  bool m_IsThunderActive = false;

  // UI state
  bool m_ShowAdvanced = false;
};

} // namespace Gini
