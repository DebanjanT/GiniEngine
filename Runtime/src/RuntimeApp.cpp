#include "Core/Application.h"
#include "RuntimeLayer.h"
#include "UI/ImGuiLayer.h"

#include <imgui.h>

namespace Gini {

class RuntimeApp : public Application {
public:
  RuntimeApp()
      : Application([]() {
          EngineConfig config;
          config.windowTitle = "Gini Runtime";
          config.windowWidth = 1280;
          config.windowHeight = 720;
          config.vsync = true;
          return config;
        }()) {
    m_RuntimeLayer = CreateScope<RuntimeLayer>();
  }

  void OnInit() override {
    // Initialize ImGui first
    ImGuiLayer::Init();

    // Then attach runtime layer (which initializes Renderer3D)
    m_RuntimeLayer->OnAttach();
  }

  void OnShutdown() override {
    m_RuntimeLayer->OnDetach();
    ImGuiLayer::Shutdown();
  }

  void OnUpdate(f32 deltaTime) override { m_RuntimeLayer->OnUpdate(deltaTime); }

  void OnRender() override {
    m_RuntimeLayer->OnRender();

    // Render ImGui debug overlay
    ImGuiLayer::Begin();

    // Debug overlay
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(200, 60));
    ImGui::Begin("Runtime", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    ImGui::Text("Gini Runtime");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();

    ImGuiLayer::End();
  }

  void OnEvent(Event &event) override { m_RuntimeLayer->OnEvent(event); }

private:
  Scope<RuntimeLayer> m_RuntimeLayer;
};

} // namespace Gini

// Entry point
Gini::Application *Gini::CreateApplication() { return new Gini::RuntimeApp(); }

int main(int argc, char **argv) {
  auto app = Gini::CreateApplication();
  app->Run();
  delete app;
  return 0;
}
