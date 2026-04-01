#include "ImGuiLayer.h"
#include "Core/Logger.h"
#include "ECS/Components.h"
#include "ECS/World.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstring>

namespace Gini {

bool ImGuiLayer::s_Initialized = false;
bool ImGuiLayer::s_BlockEvents = true;

void ImGuiLayer::Init() {
  if (s_Initialized)
    return;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // Set default font size
  io.FontGlobalScale = 1.0f;

  SetDarkTheme();

  // Get GLFW window from current context
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

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::End() {
  if (!s_Initialized)
    return;

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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

  // Window
  colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.13f, 1.0f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.16f, 0.16f, 0.21f, 1.0f);

  // Border
  colors[ImGuiCol_Border] = ImVec4(0.44f, 0.37f, 0.61f, 0.29f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.24f);

  // Text
  colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

  // Headers
  colors[ImGuiCol_Header] = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.13f, 0.75f, 0.55f, 0.80f);

  // Buttons
  colors[ImGuiCol_Button] = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.13f, 0.75f, 0.55f, 0.80f);

  // Frame
  colors[ImGuiCol_FrameBg] = ImVec4(0.13f, 0.13f, 0.17f, 1.0f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.25f, 1.0f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.21f, 1.0f);

  // Tabs
  colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.21f, 1.0f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.24f, 0.32f, 1.0f);
  colors[ImGuiCol_TabActive] = ImVec4(0.2f, 0.2f, 0.28f, 1.0f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.21f, 1.0f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.16f, 0.21f, 1.0f);

  // Title
  colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.21f, 1.0f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.21f, 1.0f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.16f, 0.16f, 0.21f, 1.0f);

  // Scrollbar
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1f, 0.1f, 0.13f, 1.0f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.16f, 0.16f, 0.21f, 1.0f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.19f, 0.19f, 0.25f, 1.0f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.24f, 0.24f, 0.32f, 1.0f);

  // Separator
  colors[ImGuiCol_Separator] = ImVec4(0.44f, 0.37f, 0.61f, 0.29f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.74f, 0.58f, 0.98f, 0.29f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.84f, 0.58f, 1.0f, 0.29f);

  // Resize
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.44f, 0.37f, 0.61f, 0.29f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.74f, 0.58f, 0.98f, 0.29f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.84f, 0.58f, 1.0f, 0.29f);

  auto &style = ImGui::GetStyle();
  style.TabRounding = 4;
  style.ScrollbarRounding = 9;
  style.WindowRounding = 7;
  style.GrabRounding = 3;
  style.FrameRounding = 3;
  style.PopupRounding = 4;
  style.ChildRounding = 4;
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
