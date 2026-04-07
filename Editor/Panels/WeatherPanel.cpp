#include "WeatherPanel.h"
#include "Core/Logger.h"
#include <imgui.h>

namespace Gini {

WeatherPanel::WeatherPanel() {
  // Weather system is initialized in EditorApp::OnInit()
}

void WeatherPanel::OnImGuiRender() {
  if (!m_Visible)
    return;

  ImGui::Begin("Weather System", &m_Visible);

  // Weather intensity slider
  ImGui::Text("Weather Control");
  ImGui::Separator();

  if (ImGui::SliderFloat("Intensity", &m_WeatherIntensity, 0.0f, 1.0f,
                         "%.2f")) {
    WeatherSystem::SetWeatherIntensity(m_WeatherIntensity);
    m_IsRaining = m_WeatherIntensity > 0.1f;
    m_IsThunderActive = m_WeatherIntensity > 0.5f;
  }

  ImGui::Spacing();

  // Weather presets
  DrawWeatherPresets();

  ImGui::Spacing();
  ImGui::Separator();

  // Rain controls
  DrawRainControls();

  ImGui::Spacing();

  // Thunder controls
  DrawThunderControls();

  ImGui::Spacing();
  ImGui::Separator();

  // Advanced settings
  if (ImGui::CollapsingHeader("Advanced Settings")) {
    DrawRainControls();
    DrawThunderControls();
  }

  // Status
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Text("Status:");
  ImGui::Text("  Rain: %s", WeatherSystem::IsRaining() ? "Active" : "Inactive");
  ImGui::Text("  Thunder: %s",
              WeatherSystem::IsThunderActive() ? "Active" : "Inactive");

  if (auto rain = WeatherSystem::GetRainEmitter()) {
    ImGui::Text("  Rain Particles: %u / %u", rain->GetActiveParticleCount(),
                rain->GetMaxParticles());
  }

  if (auto thunder = WeatherSystem::GetThunderSystem()) {
    ImGui::Text("  Active Bolts: %u", thunder->GetActiveBoltCount());
  }

  ImGui::End();
}

void WeatherPanel::DrawRainControls() {
  auto rain = WeatherSystem::GetRainEmitter();
  if (!rain)
    return;

  if (ImGui::CollapsingHeader("Rain Settings")) {
    auto &settings = rain->GetSettings();

    ImGui::Checkbox("Enable Rain", &settings.enabled);

    if (ImGui::DragFloat3("Spawn Position", &settings.position.x, 1.0f)) {
      rain->GetSettings().position = settings.position;
    }

    if (ImGui::DragFloat3("Area Size", &settings.areaSize.x, 1.0f)) {
      rain->GetSettings().areaSize = settings.areaSize;
    }

    if (ImGui::DragFloat("Fall Speed", &settings.fallSpeed, 1.0f, 10.0f,
                         100.0f)) {
      rain->GetSettings().fallSpeed = settings.fallSpeed;
    }

    if (ImGui::DragFloat("Wind Strength", &settings.windStrength, 0.1f, 0.0f,
                         20.0f)) {
      rain->GetSettings().windStrength = settings.windStrength;
    }

    if (ImGui::DragFloat("Emission Rate", &settings.emissionRate, 100.0f, 0.0f,
                         10000.0f)) {
      rain->GetSettings().emissionRate = settings.emissionRate;
    }

    if (ImGui::ColorEdit4("Rain Color", &settings.color.x)) {
      rain->GetSettings().color = settings.color;
    }

    if (ImGui::DragFloat("Drop Size", &settings.size, 0.001f, 0.005f, 0.1f)) {
      rain->GetSettings().size = settings.size;
    }
  }
}

void WeatherPanel::DrawThunderControls() {
  auto thunder = WeatherSystem::GetThunderSystem();
  if (!thunder)
    return;

  if (ImGui::CollapsingHeader("Thunder Settings")) {
    auto &settings = thunder->GetSettings();

    ImGui::Checkbox("Enable Thunder", &settings.enabled);

    if (ImGui::DragFloat("Strike Frequency", &settings.strikeFrequency, 0.5f,
                         1.0f, 20.0f)) {
      thunder->GetSettings().strikeFrequency = settings.strikeFrequency;
    }

    if (ImGui::DragFloat("Strike Variation", &settings.strikeVariation, 0.5f,
                         0.0f, 10.0f)) {
      thunder->GetSettings().strikeVariation = settings.strikeVariation;
    }

    if (ImGui::DragFloat("Bolt Intensity", &settings.boltIntensity, 0.1f, 0.1f,
                         2.0f)) {
      thunder->GetSettings().boltIntensity = settings.boltIntensity;
    }

    if (ImGui::ColorEdit4("Bolt Color", &settings.boltColor.x)) {
      thunder->GetSettings().boltColor = settings.boltColor;
    }

    if (ImGui::Checkbox("Enable Glow", &settings.enableGlow)) {
      thunder->GetSettings().enableGlow = settings.enableGlow;
    }

    if (ImGui::Checkbox("Enable Branches", &settings.enableBranches)) {
      thunder->GetSettings().enableBranches = settings.enableBranches;
    }

    // Manual strike button
    if (ImGui::Button("Strike Lightning")) {
      thunder->StrikeRandom();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear All")) {
      // This would need to be implemented in ThunderSystem
    }
  }
}

void WeatherPanel::DrawWeatherPresets() {
  ImGui::Text("Weather Presets:");

  if (ImGui::Button("Clear Sky")) {
    m_WeatherIntensity = 0.0f;
    WeatherSystem::SetWeatherIntensity(0.0f);
    WeatherSystem::StopRain();
    WeatherSystem::StopThunderstorm();
  }
  ImGui::SameLine();

  if (ImGui::Button("Light Rain")) {
    m_WeatherIntensity = 0.3f;
    WeatherSystem::SetWeatherIntensity(0.3f);
    WeatherSystem::StartRain();
    WeatherSystem::StopThunderstorm();
  }
  ImGui::SameLine();

  if (ImGui::Button("Heavy Rain")) {
    m_WeatherIntensity = 0.7f;
    WeatherSystem::SetWeatherIntensity(0.7f);
    WeatherSystem::StartRain();
    WeatherSystem::StopThunderstorm();
  }

  if (ImGui::Button("Thunderstorm")) {
    m_WeatherIntensity = 1.0f;
    WeatherSystem::SetWeatherIntensity(1.0f);
    WeatherSystem::StartThunderstorm();
  }
  ImGui::SameLine();

  if (ImGui::Button("Random Weather")) {
    m_WeatherIntensity = (float)rand() / RAND_MAX;
    WeatherSystem::SetWeatherIntensity(m_WeatherIntensity);

    if (m_WeatherIntensity > 0.5f) {
      WeatherSystem::StartThunderstorm();
    } else if (m_WeatherIntensity > 0.1f) {
      WeatherSystem::StartRain();
    } else {
      WeatherSystem::StopRain();
      WeatherSystem::StopThunderstorm();
    }
  }
}

void WeatherPanel::OnUpdate(f32 deltaTime) {
  // Update weather state
  m_IsRaining = WeatherSystem::IsRaining();
  m_IsThunderActive = WeatherSystem::IsThunderActive();
}

} // namespace Gini
