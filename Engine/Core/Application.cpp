#include "Application.h"
#include "Assets/AssetManager.h"
#include "Logger.h"
#include "Network/Network.h"
#include "ThreadProfiler.h"

#include <chrono>
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

    AssetManager::Get().Init(config.assetPath, config.enableAssetLoadingThread);
    if (config.enableNetworkThread) {
        NetworkManager::Get().InitClient();
    }

    // Disable render thread on Windows due to WGL context sharing limitations
#ifdef _WIN32
    if (m_Config.enableRenderThread) {
        GINI_WARN("Render thread is not supported on Windows due to WGL context sharing limitations. Disabling.");
        m_Config.enableRenderThread = false;
    }
#endif

    if (m_Config.enableRenderThread) {
        m_Window->CreateSharedContext();
        m_RenderThread.Start();
    }
}

Application::~Application() {
    if (m_Config.enableNetworkThread) {
        NetworkManager::Get().Shutdown();
    }
    AssetManager::Get().Shutdown();

    if (m_Config.enableRenderThread) {
        m_RenderThread.Stop();
    }
    s_Instance = nullptr;
}

void Application::Run() {
    // Always initialize main thread systems (ImGui, etc.)
    OnInit();

    if (m_Config.enableRenderThread && m_RenderThread.IsRunning()) {
        // Initialize render thread systems (3D renderer, etc.) using shared context
        m_RenderThread.SubmitAndWait([this]() {
            m_Window->MakeSharedContextCurrent();
            OnRenderThreadInit();
            return 0;
        });
    } else {
        // Initialize render systems on main thread when render thread is disabled
        OnRenderThreadInit();
    }
    
    ThreadProfiler::Get().RegisterThread("MainThread",
                                          std::this_thread::get_id());

    while (m_Running) {
        ThreadProfiler::Get().BeginFrame();
        m_DeltaTime.Update();
        f32 dt = m_DeltaTime.GetF();
        
        m_Window->Update();
        
        if (!m_Minimized) {
            // Fixed timestep updates (for deterministic simulation)
            m_FixedTimestep.Update(dt);
            while (m_FixedTimestep.ShouldTick()) {
                OnFixedUpdate(static_cast<f32>(m_FixedTimestep.GetTickDuration()));
            }
            
            {
                auto updateStart = std::chrono::steady_clock::now();
                OnUpdate(dt);
                auto updateEnd = std::chrono::steady_clock::now();
                ThreadProfiler::Get().SetUpdateTimeMs(
                    std::chrono::duration<f64, std::milli>(updateEnd - updateStart)
                        .count());
            }
            AssetManager::Get().Update();
            if (m_Config.enableNetworkThread) {
                NetworkManager::Get().Update();
            }
            
            if (m_Config.enableRenderThread && m_RenderThread.IsRunning()) {
                // Submit 3D rendering to render thread using shared context
                m_RenderThread.SubmitAndWait([this]() -> int {
                    auto renderStart = std::chrono::steady_clock::now();
                    u32 viewportWidth = 0;
                    u32 viewportHeight = 0;
                    bool hasResize = false;
                    {
                        std::lock_guard<std::mutex> lock(m_RenderStateMutex);
                        hasResize = m_HasPendingViewportResize;
                        viewportWidth = m_PendingViewportWidth;
                        viewportHeight = m_PendingViewportHeight;
                        m_HasPendingViewportResize = false;
                    }

                    if (hasResize) {
                        glViewport(0, 0, static_cast<i32>(viewportWidth),
                                   static_cast<i32>(viewportHeight));
                    }

                    OnRender();
                    glFinish(); // Ensure all rendering commands complete before main thread uses results
                    auto renderEnd = std::chrono::steady_clock::now();
                    ThreadProfiler::Get().SetRenderTimeMs(
                        std::chrono::duration<f64, std::milli>(renderEnd - renderStart)
                            .count());
                    return 0;
                });

                // Render ImGui and swap buffers on main thread
                auto imguiStart = std::chrono::steady_clock::now();
                OnImGuiRender();
                m_Window->SwapBuffers();
                auto imguiEnd = std::chrono::steady_clock::now();
                ThreadProfiler::Get().SetRenderTimeMs(
                    std::chrono::duration<f64, std::milli>(imguiEnd - imguiStart)
                        .count());
            } else {
                auto renderStart = std::chrono::steady_clock::now();
                OnRender();
                OnImGuiRender();
                m_Window->SwapBuffers();
                auto renderEnd = std::chrono::steady_clock::now();
                ThreadProfiler::Get().SetRenderTimeMs(
                    std::chrono::duration<f64, std::milli>(renderEnd - renderStart)
                        .count());
            }
        }
    }

    if (m_Config.enableRenderThread && m_RenderThread.IsRunning()) {
        m_RenderThread.SubmitAndWait([this]() {
            OnRenderThreadShutdown();
            return 0;
        });
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
    if (m_Config.enableRenderThread && m_RenderThread.IsRunning()) {
        std::lock_guard<std::mutex> lock(m_RenderStateMutex);
        m_PendingViewportWidth = static_cast<u32>(event.GetWidth());
        m_PendingViewportHeight = static_cast<u32>(event.GetHeight());
        m_HasPendingViewportResize = true;
    } else {
        glViewport(0, 0, event.GetWidth(), event.GetHeight());
    }
    return false;
}

} // namespace Gini
