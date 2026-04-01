#include "Input.h"

namespace Gini {

bool Input::IsKeyDown(Key key) const {
    i32 k = static_cast<i32>(key);
    if (k < 0 || k >= static_cast<i32>(MAX_KEYS)) return false;
    return m_KeyStates[k];
}

bool Input::IsKeyPressed(Key key) const {
    i32 k = static_cast<i32>(key);
    if (k < 0 || k >= static_cast<i32>(MAX_KEYS)) return false;
    return m_KeyStates[k] && !m_PrevKeyStates[k];
}

bool Input::IsKeyReleased(Key key) const {
    i32 k = static_cast<i32>(key);
    if (k < 0 || k >= static_cast<i32>(MAX_KEYS)) return false;
    return !m_KeyStates[k] && m_PrevKeyStates[k];
}

bool Input::IsMouseButtonDown(MouseButton button) const {
    i32 b = static_cast<i32>(button);
    if (b < 0 || b >= static_cast<i32>(MAX_MOUSE_BUTTONS)) return false;
    return m_MouseStates[b];
}

bool Input::IsMouseButtonPressed(MouseButton button) const {
    i32 b = static_cast<i32>(button);
    if (b < 0 || b >= static_cast<i32>(MAX_MOUSE_BUTTONS)) return false;
    return m_MouseStates[b] && !m_PrevMouseStates[b];
}

bool Input::IsMouseButtonReleased(MouseButton button) const {
    i32 b = static_cast<i32>(button);
    if (b < 0 || b >= static_cast<i32>(MAX_MOUSE_BUTTONS)) return false;
    return !m_MouseStates[b] && m_PrevMouseStates[b];
}

void Input::Update() {
    m_PrevKeyStates = m_KeyStates;
    m_PrevMouseStates = m_MouseStates;
    m_MouseDelta = m_MousePosition - m_PrevMousePosition;
    m_PrevMousePosition = m_MousePosition;
    m_ScrollDelta = {0.0f, 0.0f};
}

void Input::SetKeyState(Key key, bool pressed) {
    i32 k = static_cast<i32>(key);
    if (k >= 0 && k < static_cast<i32>(MAX_KEYS)) {
        m_KeyStates[k] = pressed;
    }
}

void Input::SetMouseButtonState(MouseButton button, bool pressed) {
    i32 b = static_cast<i32>(button);
    if (b >= 0 && b < static_cast<i32>(MAX_MOUSE_BUTTONS)) {
        m_MouseStates[b] = pressed;
    }
}

void Input::SetMousePosition(f32 x, f32 y) {
    m_MousePosition = {x, y};
}

void Input::SetScrollDelta(f32 x, f32 y) {
    m_ScrollDelta = {x, y};
}

} // namespace Gini
