#include "Application.h"
#include "Logger.h"

#include <glad/gl.h>

namespace Gini {

Application* Application::s_Instance = nullptr;

Application::Application(const EngineConfig& config) : m_Config(config) {
    GINI_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;
    
    GINI_INFO("Initializing Gini Engine v1.0.0");
    
    WindowProps windowProps;
    windowProps.title = config.windowTitle;
    windowProps.width = config.windowWidth;
    windowProps.height = config.windowHeight;
    windowProps.vsync = config.vsync;
    windowProps.fullscreen = config.fullscreen;
    windowProps.resizable = config.resizable;
    
    m_Window = Window::Create(windowProps);
    m_Window->SetEventCallback([this](Event& e) { OnEventInternal(e); });
    
    m_FixedTimestep = FixedTimestep(static_cast<f64>(config.targetFPS));
}

Application::~Application() {
    s_Instance = nullptr;
}

void Application::Run() {
    OnInit();
    
    while (m_Running) {
        m_DeltaTime.Update();
        f32 dt = m_DeltaTime.GetF();
        
        m_Window->Update();
        
        if (!m_Minimized) {
            // Fixed timestep updates (for deterministic simulation)
            m_FixedTimestep.Update(dt);
            while (m_FixedTimestep.ShouldTick()) {
                OnFixedUpdate(static_cast<f32>(m_FixedTimestep.GetTickDuration()));
            }
            
            // Variable timestep update
            OnUpdate(dt);
            
            // Render
            OnRender();
            OnImGuiRender();
        }
        
        m_Window->SwapBuffers();
    }
    
    OnShutdown();
}

void Application::OnEventInternal(Event& event) {
    if (event.GetType() == EventType::WindowClose) {
        auto& e = static_cast<WindowCloseEvent&>(event);
        OnWindowClose(e);
    }
    else if (event.GetType() == EventType::WindowResize) {
        auto& e = static_cast<WindowResizeEvent&>(event);
        OnWindowResize(e);
    }
    
    OnEvent(event);
}

bool Application::OnWindowClose(WindowCloseEvent& event) {
    m_Running = false;
    return true;
}

bool Application::OnWindowResize(WindowResizeEvent& event) {
    if (event.GetWidth() == 0 || event.GetHeight() == 0) {
        m_Minimized = true;
        return false;
    }
    
    m_Minimized = false;
    glViewport(0, 0, event.GetWidth(), event.GetHeight());
    return false;
}

} // namespace Gini
