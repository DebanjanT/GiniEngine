#include "Camera.h"
#include "Core/Input.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Gini {

OrthographicCamera::OrthographicCamera(f32 left, f32 right, f32 bottom, f32 top) {
    SetProjection(left, right, bottom, top);
}

void OrthographicCamera::SetProjection(f32 left, f32 right, f32 bottom, f32 top) {
    m_Projection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
    RecalculateViewMatrix();
}

void OrthographicCamera::SetPosition(const Vec3& position) {
    m_Position = position;
    RecalculateViewMatrix();
}

void OrthographicCamera::SetRotation(f32 rotation) {
    m_Rotation = rotation;
    RecalculateViewMatrix();
}

void OrthographicCamera::SetZoom(f32 zoom) {
    m_Zoom = zoom;
    RecalculateViewMatrix();
}

void OrthographicCamera::RecalculateViewMatrix() {
    Mat4 transform = glm::translate(Mat4(1.0f), m_Position) *
        glm::rotate(Mat4(1.0f), glm::radians(m_Rotation), Vec3(0, 0, 1));
    
    m_View = glm::inverse(transform);
    m_ViewProjection = m_Projection * m_View;
}

// RTS Camera Controller
RTSCameraController::RTSCameraController(f32 aspectRatio, f32 zoomLevel)
    : m_AspectRatio(aspectRatio), m_ZoomLevel(zoomLevel),
      m_Camera(-aspectRatio * zoomLevel, aspectRatio * zoomLevel, -zoomLevel, zoomLevel) {
}

void RTSCameraController::OnUpdate(f32 deltaTime) {
    auto& input = Input::Get();
    Vec3 movement{0.0f, 0.0f, 0.0f};
    
    // Keyboard movement (WASD or Arrow keys)
    if (input.IsKeyDown(Key::W) || input.IsKeyDown(Key::Up))
        movement.y += 1.0f;
    if (input.IsKeyDown(Key::S) || input.IsKeyDown(Key::Down))
        movement.y -= 1.0f;
    if (input.IsKeyDown(Key::A) || input.IsKeyDown(Key::Left))
        movement.x -= 1.0f;
    if (input.IsKeyDown(Key::D) || input.IsKeyDown(Key::Right))
        movement.x += 1.0f;
    
    // Edge scrolling (RTS style)
    if (m_EdgeScrollingEnabled) {
        Vec2 mousePos = input.GetMousePosition();
        // Note: Window dimensions would need to be passed in for proper edge detection
        // This is a simplified version
    }
    
    // Apply movement
    if (glm::length(movement) > 0.0f) {
        movement = glm::normalize(movement);
        m_CameraPosition += movement * m_MoveSpeed * m_ZoomLevel * deltaTime;
    }
    
    // Zoom with scroll wheel
    Vec2 scroll = input.GetScrollDelta();
    if (scroll.y != 0.0f) {
        m_ZoomLevel -= scroll.y * m_ZoomSpeed;
        m_ZoomLevel = glm::clamp(m_ZoomLevel, m_MinZoom, m_MaxZoom);
        UpdateProjection();
    }
    
    // Apply bounds if set
    if (m_HasBounds) {
        m_CameraPosition.x = glm::clamp(m_CameraPosition.x, m_Bounds.x, m_Bounds.x + m_Bounds.width);
        m_CameraPosition.y = glm::clamp(m_CameraPosition.y, m_Bounds.y, m_Bounds.y + m_Bounds.height);
    }
    
    m_Camera.SetPosition(m_CameraPosition);
}

void RTSCameraController::OnResize(f32 width, f32 height) {
    m_AspectRatio = width / height;
    UpdateProjection();
}

void RTSCameraController::SetZoomLevel(f32 level) {
    m_ZoomLevel = glm::clamp(level, m_MinZoom, m_MaxZoom);
    UpdateProjection();
}

void RTSCameraController::UpdateProjection() {
    m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel,
                           -m_ZoomLevel, m_ZoomLevel);
}

} // namespace Gini
