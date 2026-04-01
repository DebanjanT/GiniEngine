#pragma once

#include "Types.h"
#include <functional>
#include <vector>
#include <unordered_map>
#include <string>

namespace Gini {

enum class EventType {
    None = 0,
    // Window events
    WindowClose, WindowResize, WindowFocus, WindowLostFocus,
    // Keyboard events
    KeyPressed, KeyReleased, KeyTyped,
    // Mouse events
    MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
    // Game events
    UnitSelected, UnitDeselected, UnitCommand, BuildingPlaced,
    // Custom events
    Custom
};

enum class EventCategory : u32 {
    None        = 0,
    Application = 1 << 0,
    Input       = 1 << 1,
    Keyboard    = 1 << 2,
    Mouse       = 1 << 3,
    Game        = 1 << 4
};

inline EventCategory operator|(EventCategory a, EventCategory b) {
    return static_cast<EventCategory>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline bool operator&(EventCategory a, EventCategory b) {
    return (static_cast<u32>(a) & static_cast<u32>(b)) != 0;
}

class Event {
public:
    virtual ~Event() = default;
    virtual EventType GetType() const = 0;
    virtual const char* GetName() const = 0;
    virtual EventCategory GetCategory() const = 0;
    
    bool handled = false;
    
    bool IsInCategory(EventCategory category) const {
        return GetCategory() & category;
    }
};

// Window Events
class WindowCloseEvent : public Event {
public:
    EventType GetType() const override { return EventType::WindowClose; }
    const char* GetName() const override { return "WindowClose"; }
    EventCategory GetCategory() const override { return EventCategory::Application; }
};

class WindowResizeEvent : public Event {
public:
    WindowResizeEvent(u32 width, u32 height) : m_Width(width), m_Height(height) {}
    
    u32 GetWidth() const { return m_Width; }
    u32 GetHeight() const { return m_Height; }
    
    EventType GetType() const override { return EventType::WindowResize; }
    const char* GetName() const override { return "WindowResize"; }
    EventCategory GetCategory() const override { return EventCategory::Application; }
    
private:
    u32 m_Width, m_Height;
};

// Keyboard Events
class KeyEvent : public Event {
public:
    i32 GetKeyCode() const { return m_KeyCode; }
    EventCategory GetCategory() const override { 
        return EventCategory::Input | EventCategory::Keyboard; 
    }
    
protected:
    KeyEvent(i32 keycode) : m_KeyCode(keycode) {}
    i32 m_KeyCode;
};

class KeyPressedEvent : public KeyEvent {
public:
    KeyPressedEvent(i32 keycode, bool repeat = false) 
        : KeyEvent(keycode), m_IsRepeat(repeat) {}
    
    bool IsRepeat() const { return m_IsRepeat; }
    
    EventType GetType() const override { return EventType::KeyPressed; }
    const char* GetName() const override { return "KeyPressed"; }
    
private:
    bool m_IsRepeat;
};

class KeyReleasedEvent : public KeyEvent {
public:
    KeyReleasedEvent(i32 keycode) : KeyEvent(keycode) {}
    
    EventType GetType() const override { return EventType::KeyReleased; }
    const char* GetName() const override { return "KeyReleased"; }
};

// Mouse Events
class MouseMovedEvent : public Event {
public:
    MouseMovedEvent(f32 x, f32 y) : m_X(x), m_Y(y) {}
    
    f32 GetX() const { return m_X; }
    f32 GetY() const { return m_Y; }
    
    EventType GetType() const override { return EventType::MouseMoved; }
    const char* GetName() const override { return "MouseMoved"; }
    EventCategory GetCategory() const override { 
        return EventCategory::Input | EventCategory::Mouse; 
    }
    
private:
    f32 m_X, m_Y;
};

class MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(f32 xOffset, f32 yOffset) 
        : m_XOffset(xOffset), m_YOffset(yOffset) {}
    
    f32 GetXOffset() const { return m_XOffset; }
    f32 GetYOffset() const { return m_YOffset; }
    
    EventType GetType() const override { return EventType::MouseScrolled; }
    const char* GetName() const override { return "MouseScrolled"; }
    EventCategory GetCategory() const override { 
        return EventCategory::Input | EventCategory::Mouse; 
    }
    
private:
    f32 m_XOffset, m_YOffset;
};

class MouseButtonEvent : public Event {
public:
    i32 GetButton() const { return m_Button; }
    EventCategory GetCategory() const override { 
        return EventCategory::Input | EventCategory::Mouse; 
    }
    
protected:
    MouseButtonEvent(i32 button) : m_Button(button) {}
    i32 m_Button;
};

class MouseButtonPressedEvent : public MouseButtonEvent {
public:
    MouseButtonPressedEvent(i32 button) : MouseButtonEvent(button) {}
    
    EventType GetType() const override { return EventType::MouseButtonPressed; }
    const char* GetName() const override { return "MouseButtonPressed"; }
};

class MouseButtonReleasedEvent : public MouseButtonEvent {
public:
    MouseButtonReleasedEvent(i32 button) : MouseButtonEvent(button) {}
    
    EventType GetType() const override { return EventType::MouseButtonReleased; }
    const char* GetName() const override { return "MouseButtonReleased"; }
};

// Event Dispatcher
class EventDispatcher {
public:
    using EventCallback = std::function<void(Event&)>;
    
    void Subscribe(EventType type, EventCallback callback) {
        m_Callbacks[type].push_back(callback);
    }
    
    void Dispatch(Event& event) {
        auto it = m_Callbacks.find(event.GetType());
        if (it != m_Callbacks.end()) {
            for (auto& callback : it->second) {
                if (!event.handled) {
                    callback(event);
                }
            }
        }
    }
    
private:
    std::unordered_map<EventType, std::vector<EventCallback>> m_Callbacks;
};

} // namespace Gini
