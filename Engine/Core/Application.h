#pragma once

#include "Types.h"
#include "Event.h"
#include "Timer.h"
#include "Threading.h"
#include "Window/Window.h"
#include <mutex>

namespace Gini {

class Application {
public:
    Application(const EngineConfig& config = EngineConfig());
    virtual ~Application();
    
    void Run();
    void Quit() { m_Running = false; }
    
    // Override these in your game
    virtual void OnInit() {}
    virtual void OnRenderThreadInit() {}
    virtual void OnRenderThreadShutdown() {}
    virtual void OnShutdown() {}
    virtual void OnUpdate(f32 deltaTime) {}
    virtual void OnFixedUpdate(f32 fixedDeltaTime) {}
    virtual void OnRender() {}
    virtual void OnImGuiRender() {}
    virtual void OnEvent(Event& event) {}
    
    // Getters
    Window& GetWindow() { return *m_Window; }
    const EngineConfig& GetConfig() const { return m_Config; }
    f64 GetTime() const { return m_DeltaTime.GetTotal(); }
    f32 GetDeltaTime() const { return m_DeltaTime.GetF(); }
    f64 GetFPS() const { return m_DeltaTime.GetFPS(); }
    
    static Application& Get() { return *s_Instance; }
    
protected:
    EngineConfig m_Config;
    Scope<Window> m_Window;
    bool m_Running = true;
    bool m_Minimized = false;
    
    DeltaTime m_DeltaTime;
    FixedTimestep m_FixedTimestep;
    RenderThread m_RenderThread;
    std::mutex m_RenderStateMutex;
    u32 m_PendingViewportWidth = 0;
    u32 m_PendingViewportHeight = 0;
    bool m_HasPendingViewportResize = false;
    
private:
    void OnEventInternal(Event& event);
    bool OnWindowClose(WindowCloseEvent& event);
    bool OnWindowResize(WindowResizeEvent& event);
    
    static Application* s_Instance;

};

// Define this in your game
Application* CreateApplication();

} // namespace Gini
