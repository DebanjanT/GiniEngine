#include "ImGuiLayer.h"
#include "Core/Logger.h"
#include "ECS/Components.h"
#include "ECS/World.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_opengl3_loader.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstring>

#ifdef _WIN32
const char *fontPath = "C:/Windows/Fonts/Arial.ttf";
#elif __APPLE__
const char *fontPath = "/System/Library/Fonts/SFNS.ttf"; // fallback
#else
const char *fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#endif

namespace Gini {
float ImGuiLayer::m_fontSize = 16.0f;
float ImGuiLayer::m_pendingFontSize = -1.0f;
bool ImGuiLayer::s_Initialized = false;
bool ImGuiLayer::s_BlockEvents = true;

void ImGuiLayer::Init() {
  if (s_Initialized)
    return;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  // When viewports are enabled, tweak WindowRounding/WindowBg
  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  ImFont *regular = io.Fonts->AddFontFromFileTTF(
      "assets/fonts/SpaceGrotesk-Regular.ttf", m_fontSize);

  ImFont *semibold = io.Fonts->AddFontFromFileTTF(
      "assets/fonts/SpaceGrotesk-SemiBold.ttf", m_fontSize);

  if (!regular) {
    GINI_ERROR("Regular Font load failed!");
    regular = io.Fonts->AddFontDefault();
  }
  GINI_INFO("Font Loaded Correctly");
  io.FontDefault = regular;
  io.FontGlobalScale = 1.0f;

  SetDarkTheme();

  GLFWwindow *window = glfwGetCurrentContext();
  if (!window) {
    GINI_ERROR("ImGuiLayer::Init - No GLFW context available!");
    return;
  }

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 410");

  s_Initialized = true;
  GINI_INFO("ImGui initialized successfully");
}

void ImGuiLayer::ReloadFonts(float size) {
  ImGuiIO &io = ImGui::GetIO();

  io.Fonts->Clear();

  ImFont *regular = io.Fonts->AddFontFromFileTTF(
      "assets/fonts/SpaceGrotesk-Regular.ttf", size);

  ImFont *semibold = io.Fonts->AddFontFromFileTTF(
      "assets/fonts/SpaceGrotesk-SemiBold.ttf", size);

  if (!regular) {
    GINI_ERROR("Font load failed!");
    regular = io.Fonts->AddFontDefault();
  }
  GINI_INFO("Font Loaded Correctly");
  io.FontDefault = regular;

  // Font texture will be rebuilt on next NewFrame
  io.Fonts->Build();
}

void ImGuiLayer::SetFontSize(float size) { m_pendingFontSize = size; }

void ImGuiLayer::Shutdown() {
  if (!s_Initialized)
    return;

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  s_Initialized = false;
  GINI_INFO("ImGui shutdown");
}

void ImGuiLayer::Begin() {
  if (!s_Initialized)
    return;
  if (m_pendingFontSize > 0.0f) {

    m_fontSize = m_pendingFontSize;
    m_pendingFontSize = -1.0f;

    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();

    ImFont *font = io.Fonts->AddFontFromFileTTF(
        "assets/fonts/SpaceGrotesk-Regular.ttf", m_fontSize);

    if (!font)
      font = io.Fonts->AddFontDefault();

    io.FontDefault = font;

    // Font texture will be rebuilt on next NewFrame
    io.Fonts->Build();
  }
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::End() {
  if (!s_Initialized)
    return;

  ImGuiIO &io = ImGui::GetIO();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  // Update and render additional platform windows
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    GLFWwindow *backup_current_context = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup_current_context);
  }
}

void ImGuiLayer::OnEvent(Event &event) {
  if (!s_Initialized || !s_BlockEvents)
    return;

  ImGuiIO &io = ImGui::GetIO();

  if (event.GetType() == EventType::MouseButtonPressed ||
      event.GetType() == EventType::MouseButtonReleased ||
      event.GetType() == EventType::MouseMoved ||
      event.GetType() == EventType::MouseScrolled) {
    if (io.WantCaptureMouse) {
      event.handled = true;
    }
  }

  if (event.GetType() == EventType::KeyPressed ||
      event.GetType() == EventType::KeyReleased ||
      event.GetType() == EventType::KeyTyped) {
    if (io.WantCaptureKeyboard) {
      event.handled = true;
    }
  }
}

void ImGuiLayer::SetDarkTheme() {
  ImGui::StyleColorsDark();

  auto &colors = ImGui::GetStyle().Colors;

  // Base (bluish dark)
  colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.08f, 0.09f, 1.0f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.10f, 0.11f, 1.0f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.11f, 0.12f, 1.0f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.12f, 0.15f, 1.00f);

  // Borders (very subtle)
  colors[ImGuiCol_Border] = ImVec4(0.20f, 0.25f, 0.30f, 0.30f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  // Text
  colors[ImGuiCol_Text] = ImVec4(0.85f, 0.88f, 0.92f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.50f, 0.55f, 1.00f);

  // Primary accent (Frostbite blue)
  ImVec4 accent = ImVec4(0.20f, 0.55f, 0.85f, 1.00f);
  ImVec4 accentHover = ImVec4(0.30f, 0.65f, 0.95f, 1.00f);
  ImVec4 accentActive = ImVec4(0.15f, 0.45f, 0.75f, 1.00f);

  // Headers
  colors[ImGuiCol_Header] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
  colors[ImGuiCol_HeaderHovered] =
      ImVec4(accentHover.x, accentHover.y, accentHover.z, 0.55f);
  colors[ImGuiCol_HeaderActive] =
      ImVec4(accentActive.x, accentActive.y, accentActive.z, 0.75f);

  // Buttons
  colors[ImGuiCol_Button] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
  colors[ImGuiCol_ButtonHovered] =
      ImVec4(accentHover.x, accentHover.y, accentHover.z, 0.60f);
  colors[ImGuiCol_ButtonActive] =
      ImVec4(accentActive.x, accentActive.y, accentActive.z, 0.80f);

  // Frame
  colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.13f, 0.16f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.18f, 0.22f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.12f, 0.16f, 0.20f, 1.00f);

  // Tabs (flat, subtle)
  colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.09f, 0.10f, 1.0f);
  colors[ImGuiCol_TabActive] = ImVec4(0.12f, 0.14f, 0.16f, 1.0f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.18f, 0.22f, 0.27f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.10f, 0.13f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.10f, 0.12f, 0.15f, 1.00f);

  // Title
  colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.10f, 0.13f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.12f, 0.15f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.10f, 0.13f, 1.00f);

  // Scrollbar
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.08f, 0.10f, 1.00f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.12f, 0.15f, 0.18f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.27f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.22f, 0.27f, 0.32f, 1.00f);

  // Separator (very subtle blue hint)
  colors[ImGuiCol_Separator] = ImVec4(1, 1, 1, 0.06f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.50f, 0.70f, 0.50f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.35f, 0.60f, 0.85f, 0.70f);

  // Resize grip
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.30f, 0.40f, 0.30f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.30f, 0.50f, 0.70f, 0.60f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.35f, 0.60f, 0.85f, 0.80f);

  // Rounding (Frostbite is sharper)
  auto &style = ImGui::GetStyle();
  style.WindowRounding = 2;
  style.FrameRounding = 1;
  style.PopupRounding = 3;
  style.ScrollbarRounding = 6;
  style.GrabRounding = 2;
  style.TabRounding = 2;
  style.ChildRounding = 3;
}

void ImGuiLayer::SetLightTheme() { ImGui::StyleColorsLight(); }

void ImGuiLayer::SetCustomTheme(const Vec4 &primary, const Vec4 &secondary,
                                const Vec4 &accent) {
  auto &colors = ImGui::GetStyle().Colors;

  colors[ImGuiCol_WindowBg] =
      ImVec4(primary.x, primary.y, primary.z, primary.w);
  colors[ImGuiCol_Header] = ImVec4(accent.x, accent.y, accent.z, 0.4f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(accent.x, accent.y, accent.z, 0.6f);
  colors[ImGuiCol_HeaderActive] = ImVec4(accent.x, accent.y, accent.z, 0.8f);
  colors[ImGuiCol_Button] = ImVec4(secondary.x, secondary.y, secondary.z, 0.4f);
  colors[ImGuiCol_ButtonHovered] =
      ImVec4(secondary.x, secondary.y, secondary.z, 0.6f);
  colors[ImGuiCol_ButtonActive] =
      ImVec4(secondary.x, secondary.y, secondary.z, 0.8f);
}

bool ImGuiLayer::WantCaptureMouse() {
  return s_Initialized && ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiLayer::WantCaptureKeyboard() {
  return s_Initialized && ImGui::GetIO().WantCaptureKeyboard;
}

void ImGuiLayer::ShowDemoWindow(bool *open) { ImGui::ShowDemoWindow(open); }

void ImGuiLayer::ShowMetricsWindow(bool *open) {
  ImGui::ShowMetricsWindow(open);
}

void ImGuiLayer::ShowRendererStats() {
  ImGui::Begin("Renderer Stats");

  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

  ImGui::Separator();
  ImGui::Text("Draw Calls: --");
  ImGui::Text("Triangles: --");
  ImGui::Text("Vertices: --");

  ImGui::End();
}

void ImGuiLayer::ShowSceneHierarchy(World *world) {
  ImGui::Begin("Scene Hierarchy");

  if (!world) {
    ImGui::Text("No world loaded");
    ImGui::End();
    return;
  }

  ImGui::Text("Entity Count: %u", world->GetEntityCount());
  ImGui::Separator();

  auto view = world->GetRegistry().view<TagComponent>();
  for (auto entity : view) {
    auto &tag = view.get<TagComponent>(entity);
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    bool opened =
        ImGui::TreeNodeEx((void *)(u64)entity, flags, "%s", tag.tag.c_str());

    if (ImGui::IsItemClicked()) {
      // Select entity
    }

    if (opened) {
      ImGui::TreePop();
    }
  }

  ImGui::End();
}

void ImGuiLayer::ShowInspector(World *world, Entity selectedEntity) {
  ImGui::Begin("Inspector");

  if (!world || selectedEntity == NullEntity) {
    ImGui::Text("No entity selected");
    ImGui::End();
    return;
  }

  if (world->HasComponent<TagComponent>(selectedEntity)) {
    auto &tag = world->GetComponent<TagComponent>(selectedEntity);
    char buffer[256];
    std::strncpy(buffer, tag.tag.c_str(), sizeof(buffer));
    if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
      tag.tag = buffer;
    }
  }

  ImGui::Separator();

  if (world->HasComponent<TransformComponent>(selectedEntity)) {
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
      auto &transform = world->GetComponent<TransformComponent>(selectedEntity);
      ImGui::DragFloat3("Position", &transform.position.x, 0.1f);
      ImGui::DragFloat3("Rotation", &transform.rotation.x, 1.0f);
      ImGui::DragFloat3("Scale", &transform.scale.x, 0.1f);
    }
  }

  if (world->HasComponent<SpriteComponent>(selectedEntity)) {
    if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen)) {
      auto &sprite = world->GetComponent<SpriteComponent>(selectedEntity);
      ImGui::ColorEdit4("Color", &sprite.color.r);
    }
  }

  ImGui::End();
}

void ImGuiLayer::ShowAssetBrowser() {
  ImGui::Begin("Asset Browser");
  ImGui::Text("Asset browser coming soon...");
  ImGui::End();
}

void ImGuiLayer::ShowConsole() {
  ImGui::Begin("Console");
  ImGui::Text("Console coming soon...");
  ImGui::End();
}

void ImGuiLayer::ShowProfiler() {
  ImGui::Begin("Profiler");

  ImGui::Text("Frame Time Graph");
  static float frameTimes[100] = {};
  static int frameIndex = 0;
  frameTimes[frameIndex] = 1000.0f / ImGui::GetIO().Framerate;
  frameIndex = (frameIndex + 1) % 100;

  ImGui::PlotLines("##FrameTimes", frameTimes, 100, frameIndex, nullptr, 0.0f,
                   33.3f, ImVec2(0, 80));

  ImGui::Separator();
  ImGui::Text("Memory Usage: -- MB");
  ImGui::Text("GPU Memory: -- MB");

  ImGui::End();
}

} // namespace Gini
