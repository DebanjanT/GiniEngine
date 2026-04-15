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
#include <filesystem>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#elif _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace Gini {

static std::string GetExecutableDir() {
  std::string path;
#ifdef __APPLE__
  char buf[PATH_MAX];
  uint32_t size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) == 0) {
    path = std::filesystem::path(buf).parent_path().string();
  }
#elif _WIN32
  char buf[MAX_PATH];
  GetModuleFileNameA(NULL, buf, MAX_PATH);
  path = std::filesystem::path(buf).parent_path().string();
#else
  char buf[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (len != -1) {
    buf[len] = '\0';
    path = std::filesystem::path(buf).parent_path().string();
  }
#endif
  return path;
}

static std::string GetFontPath(const std::string &fontName) {
  std::string exeDir = GetExecutableDir();
  if (!exeDir.empty()) {
    std::string fontPath = exeDir + "/assets/fonts/" + fontName;
    if (std::filesystem::exists(fontPath)) {
      return fontPath;
    }
  }
  // Fallback to relative path
  return "assets/fonts/" + fontName;
}
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

  std::string regularFontPath = GetFontPath("MavenPro-Regular.ttf");
  std::string semiboldFontPath = GetFontPath("MavenPro-SemiBold.ttf");

  ImFont *regular =
      io.Fonts->AddFontFromFileTTF(regularFontPath.c_str(), m_fontSize);
  ImFont *semibold =
      io.Fonts->AddFontFromFileTTF(semiboldFontPath.c_str(), m_fontSize);

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

  std::string regularFontPath = GetFontPath("MavenPro-Regular.ttf");
  std::string semiboldFontPath = GetFontPath("MavenPro-SemiBold.ttf");

  ImFont *regular = io.Fonts->AddFontFromFileTTF(regularFontPath.c_str(), size);
  ImFont *semibold =
      io.Fonts->AddFontFromFileTTF(semiboldFontPath.c_str(), size);

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

    std::string fontPath = GetFontPath("MavenPro-Regular.ttf");
    ImFont *font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), m_fontSize);

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

  auto &style = ImGui::GetStyle();
  auto &colors = style.Colors;

  // -- Palette --
  // Background tiers (darkest to lightest)
  ImVec4 bg0(0.11f, 0.11f, 0.12f, 1.0f);   // deepest panels
  ImVec4 bg1(0.14f, 0.14f, 0.15f, 1.0f);   // windows
  ImVec4 bg2(0.17f, 0.17f, 0.19f, 1.0f);   // child / popup
  ImVec4 bg3(0.20f, 0.20f, 0.22f, 1.0f);   // frames / input fields

  // Accent (teal-blue)
  ImVec4 accent(0.18f, 0.56f, 0.72f, 1.0f);
  ImVec4 accentHover(0.24f, 0.65f, 0.82f, 1.0f);
  ImVec4 accentActive(0.14f, 0.46f, 0.62f, 1.0f);
  ImVec4 accentMuted(0.18f, 0.56f, 0.72f, 0.35f);

  // Text
  ImVec4 textPrimary(0.92f, 0.93f, 0.94f, 1.0f);
  ImVec4 textSecondary(0.55f, 0.56f, 0.58f, 1.0f);

  // Hover surface
  ImVec4 hoverSurface(0.24f, 0.24f, 0.27f, 1.0f);
  ImVec4 activeSurface(0.20f, 0.20f, 0.22f, 1.0f);

  // -- Backgrounds --
  colors[ImGuiCol_WindowBg]  = bg1;
  colors[ImGuiCol_ChildBg]   = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  colors[ImGuiCol_PopupBg]   = ImVec4(0.13f, 0.13f, 0.15f, 0.96f);
  colors[ImGuiCol_MenuBarBg] = bg0;

  // -- Borders --
  colors[ImGuiCol_Border]       = ImVec4(0.24f, 0.24f, 0.26f, 0.65f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

  // -- Text --
  colors[ImGuiCol_Text]         = textPrimary;
  colors[ImGuiCol_TextDisabled] = textSecondary;

  // -- Headers (tree nodes, collapsing headers) --
  colors[ImGuiCol_Header]        = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
  colors[ImGuiCol_HeaderHovered] = hoverSurface;
  colors[ImGuiCol_HeaderActive]  = activeSurface;

  // -- Buttons (muted, not overly bright) --
  colors[ImGuiCol_Button]        = ImVec4(0.22f, 0.23f, 0.25f, 1.0f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.29f, 0.32f, 1.0f);
  colors[ImGuiCol_ButtonActive]  = accent;

  // -- Input frames --
  colors[ImGuiCol_FrameBg]        = bg3;
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.27f, 1.0f);
  colors[ImGuiCol_FrameBgActive]  = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);

  // -- Tabs --
  colors[ImGuiCol_Tab]                = bg0;
  colors[ImGuiCol_TabHovered]         = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);
  colors[ImGuiCol_TabActive]          = bg1;
  colors[ImGuiCol_TabUnfocused]       = bg0;
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.16f, 1.0f);

  // -- Title bars --
  colors[ImGuiCol_TitleBg]          = bg0;
  colors[ImGuiCol_TitleBgActive]    = bg0;
  colors[ImGuiCol_TitleBgCollapsed] = bg0;

  // -- Scrollbar --
  colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.10f, 0.10f, 0.11f, 0.6f);
  colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.28f, 0.28f, 0.30f, 1.0f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.36f, 0.38f, 1.0f);
  colors[ImGuiCol_ScrollbarGrabActive]  = accent;

  // -- Separators --
  colors[ImGuiCol_Separator]        = ImVec4(0.24f, 0.24f, 0.26f, 0.50f);
  colors[ImGuiCol_SeparatorHovered] = accentMuted;
  colors[ImGuiCol_SeparatorActive]  = accent;

  // -- Resize grip --
  colors[ImGuiCol_ResizeGrip]        = ImVec4(0.24f, 0.24f, 0.26f, 0.25f);
  colors[ImGuiCol_ResizeGripHovered] = accentMuted;
  colors[ImGuiCol_ResizeGripActive]  = accent;

  // -- Selection / interaction accent --
  colors[ImGuiCol_CheckMark]       = accent;
  colors[ImGuiCol_SliderGrab]      = accent;
  colors[ImGuiCol_SliderGrabActive] = accentActive;

  // -- Docking --
  colors[ImGuiCol_DockingPreview] = ImVec4(0.18f, 0.56f, 0.72f, 0.50f);
  colors[ImGuiCol_DockingEmptyBg] = bg0;

  // -- Plot --
  colors[ImGuiCol_PlotLines]            = accent;
  colors[ImGuiCol_PlotLinesHovered]     = accentHover;
  colors[ImGuiCol_PlotHistogram]        = accent;
  colors[ImGuiCol_PlotHistogramHovered] = accentHover;

  // -- Nav --
  colors[ImGuiCol_NavHighlight] = accent;

  // -- Table --
  colors[ImGuiCol_TableHeaderBg]     = bg0;
  colors[ImGuiCol_TableBorderStrong] = ImVec4(0.24f, 0.24f, 0.26f, 0.60f);
  colors[ImGuiCol_TableBorderLight]  = ImVec4(0.20f, 0.20f, 0.22f, 0.40f);
  colors[ImGuiCol_TableRowBg]        = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  colors[ImGuiCol_TableRowBgAlt]     = ImVec4(1.0f, 1.0f, 1.0f, 0.015f);

  // ===== Geometry =====
  style.WindowRounding    = 2.0f;
  style.ChildRounding     = 2.0f;
  style.FrameRounding     = 3.0f;
  style.PopupRounding     = 3.0f;
  style.ScrollbarRounding = 6.0f;
  style.GrabRounding      = 2.0f;
  style.TabRounding       = 2.0f;

  // ===== Sizing =====
  style.WindowPadding     = ImVec2(8, 8);
  style.FramePadding      = ImVec2(6, 4);
  style.CellPadding       = ImVec2(4, 2);
  style.ItemSpacing       = ImVec2(8, 4);
  style.ItemInnerSpacing  = ImVec2(4, 4);
  style.IndentSpacing     = 16.0f;
  style.ScrollbarSize     = 12.0f;
  style.GrabMinSize       = 8.0f;

  // ===== Borders =====
  style.WindowBorderSize  = 1.0f;
  style.ChildBorderSize   = 0.0f;
  style.PopupBorderSize   = 1.0f;
  style.FrameBorderSize   = 0.0f;
  style.TabBorderSize     = 0.0f;

  // ===== Misc =====
  style.WindowMenuButtonPosition = ImGuiDir_None;
  style.ColorButtonPosition      = ImGuiDir_Right;
  style.WindowTitleAlign         = ImVec2(0.02f, 0.50f);
  style.SeparatorTextBorderSize  = 2.0f;
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
