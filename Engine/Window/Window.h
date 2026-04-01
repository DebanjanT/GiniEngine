#pragma once

#include "Core/Types.h"
#include "Core/Event.h"
#include <functional>
#include <string>

struct GLFWwindow;

namespace Gini {

struct WindowProps {
    std::string title = "Gini Engine";
    u32 width = 1280;
    u32 height = 720;
    bool vsync = true;
    bool fullscreen = false;
    bool resizable = true;
};

class Window {
public:
    using EventCallback = std::function<void(Event&)>;
    
    Window(const WindowProps& props = WindowProps());
    ~Window();
    
    void Update();
    void SwapBuffers();
    bool ShouldClose() const;
    void Close();
    
    // Getters
    u32 GetWidth() const { return m_Data.width; }
    u32 GetHeight() const { return m_Data.height; }
    f32 GetAspectRatio() const { return static_cast<f32>(m_Data.width) / static_cast<f32>(m_Data.height); }
    GLFWwindow* GetNativeWindow() const { return m_Window; }
    
    // Setters
    void SetVSync(bool enabled);
    bool IsVSync() const { return m_Data.vsync; }
    void SetTitle(const std::string& title);
    void SetEventCallback(const EventCallback& callback) { m_Data.eventCallback = callback; }
    
    // Window operations
    void SetFullscreen(bool fullscreen);
    void Maximize();
    void Minimize();
    void Restore();
    void Focus();
    void SetSize(u32 width, u32 height);
    void SetPosition(i32 x, i32 y);
    Vec2 GetPosition() const;
    
    // Cursor
    void SetCursorMode(bool visible, bool locked = false);
    void SetCursor(i32 cursorType);
    
    // Static
    static Scope<Window> Create(const WindowProps& props = WindowProps());
    
private:
    void Init(const WindowProps& props);
    void Shutdown();
    void SetupCallbacks();
    
    GLFWwindow* m_Window = nullptr;
    
    struct WindowData {
        std::string title;
        u32 width = 0;
        u32 height = 0;
        bool vsync = true;
        bool fullscreen = false;
        EventCallback eventCallback;
    };
    
    WindowData m_Data;
    
    // For restoring from fullscreen
    i32 m_WindowedX = 0, m_WindowedY = 0;
    u32 m_WindowedWidth = 0, m_WindowedHeight = 0;
};

} // namespace Gini
