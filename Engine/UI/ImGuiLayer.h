#pragma once

#include "Core/Event.h"
#include "Core/Types.h"
#include "ECS/World.h"

namespace Gini {

class ImGuiLayer {
public:
  static void Init();
  static void Shutdown();

  static void Begin();
  static void End();

  static void OnEvent(Event &event);

  static void SetDarkTheme();
  static void SetLightTheme();
  static void SetCustomTheme(const Vec4 &primary, const Vec4 &secondary,
                             const Vec4 &accent);

  static bool WantCaptureMouse();
  static bool WantCaptureKeyboard();

  static void ShowDemoWindow(bool *open = nullptr);
  static void ShowMetricsWindow(bool *open = nullptr);

  // Engine debug panels
  static void ShowRendererStats();
  static void ShowSceneHierarchy(class World *world);
  static void ShowInspector(class World *world, Entity selectedEntity);
  static void ShowAssetBrowser();
  static void ShowConsole();
  static void ShowProfiler();

private:
  static bool s_Initialized;
  static bool s_BlockEvents;
};

// Helper macros for quick debug UI
#define IMGUI_BEGIN_WINDOW(name) if (ImGui::Begin(name))
#define IMGUI_END_WINDOW() ImGui::End()

} // namespace Gini
