#pragma once

#include "Types.h"
#include <array>

namespace Gini {

// Key codes (matching GLFW)
enum class Key : i32 {
    Unknown = -1,
    Space = 32, Apostrophe = 39, Comma = 44, Minus = 45, Period = 46, Slash = 47,
    D0 = 48, D1, D2, D3, D4, D5, D6, D7, D8, D9,
    Semicolon = 59, Equal = 61,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LeftBracket = 91, Backslash, RightBracket, GraveAccent = 96,
    Escape = 256, Enter, Tab, Backspace, Insert, Delete,
    Right, Left, Down, Up,
    PageUp, PageDown, Home, End,
    CapsLock = 280, ScrollLock, NumLock, PrintScreen, Pause,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    KP0 = 320, KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9,
    KPDecimal, KPDivide, KPMultiply, KPSubtract, KPAdd, KPEnter, KPEqual,
    LeftShift = 340, LeftControl, LeftAlt, LeftSuper,
    RightShift, RightControl, RightAlt, RightSuper, Menu,
    Last = Menu
};

enum class MouseButton : i32 {
    Left = 0, Right = 1, Middle = 2,
    Button4, Button5, Button6, Button7, Button8,
    Last = Button8
};

enum class KeyMod : u32 {
    None = 0,
    Shift = 1 << 0,
    Control = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3,
    CapsLock = 1 << 4,
    NumLock = 1 << 5
};

inline KeyMod operator|(KeyMod a, KeyMod b) {
    return static_cast<KeyMod>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline bool operator&(KeyMod a, KeyMod b) {
    return (static_cast<u32>(a) & static_cast<u32>(b)) != 0;
}

class Input {
public:
    static Input& Get() {
        static Input instance;
        return instance;
    }
    
    // Keyboard
    bool IsKeyDown(Key key) const;
    bool IsKeyPressed(Key key) const;  // Just pressed this frame
    bool IsKeyReleased(Key key) const; // Just released this frame
    
    // Mouse
    bool IsMouseButtonDown(MouseButton button) const;
    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonReleased(MouseButton button) const;
    
    Vec2 GetMousePosition() const { return m_MousePosition; }
    Vec2 GetMouseDelta() const { return m_MouseDelta; }
    f32 GetMouseX() const { return m_MousePosition.x; }
    f32 GetMouseY() const { return m_MousePosition.y; }
    Vec2 GetScrollDelta() const { return m_ScrollDelta; }
    
    // Modifiers
    KeyMod GetModifiers() const { return m_Modifiers; }
    bool IsShiftDown() const { return m_Modifiers & KeyMod::Shift; }
    bool IsControlDown() const { return m_Modifiers & KeyMod::Control; }
    bool IsAltDown() const { return m_Modifiers & KeyMod::Alt; }
    
    // Called by Window system
    void Update();
    void SetKeyState(Key key, bool pressed);
    void SetMouseButtonState(MouseButton button, bool pressed);
    void SetMousePosition(f32 x, f32 y);
    void SetScrollDelta(f32 x, f32 y);
    void SetModifiers(KeyMod mods) { m_Modifiers = mods; }
    
private:
    Input() {
        m_KeyStates.fill(false);
        m_PrevKeyStates.fill(false);
        m_MouseStates.fill(false);
        m_PrevMouseStates.fill(false);
    }
    
    static constexpr size_t MAX_KEYS = 512;
    static constexpr size_t MAX_MOUSE_BUTTONS = 8;
    
    std::array<bool, MAX_KEYS> m_KeyStates;
    std::array<bool, MAX_KEYS> m_PrevKeyStates;
    std::array<bool, MAX_MOUSE_BUTTONS> m_MouseStates;
    std::array<bool, MAX_MOUSE_BUTTONS> m_PrevMouseStates;
    
    Vec2 m_MousePosition{0.0f, 0.0f};
    Vec2 m_PrevMousePosition{0.0f, 0.0f};
    Vec2 m_MouseDelta{0.0f, 0.0f};
    Vec2 m_ScrollDelta{0.0f, 0.0f};
    KeyMod m_Modifiers = KeyMod::None;
};

} // namespace Gini
