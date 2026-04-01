#include "Camera3D.h"
#include "Core/Input.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Gini {

Camera3D::Camera3D(f32 fov, f32 aspectRatio, f32 nearPlane, f32 farPlane)
    : m_FOV(fov), m_AspectRatio(aspectRatio), m_NearPlane(nearPlane), m_FarPlane(farPlane) {
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

void Camera3D::SetPerspective(f32 fov, f32 aspectRatio, f32 nearPlane, f32 farPlane) {
    m_FOV = fov;
    m_AspectRatio = aspectRatio;
    m_NearPlane = nearPlane;
    m_FarPlane = farPlane;
    m_IsPerspective = true;
    UpdateProjectionMatrix();
}

void Camera3D::SetOrthographic(f32 left, f32 right, f32 bottom, f32 top, f32 nearPlane, f32 farPlane) {
    m_NearPlane = nearPlane;
    m_FarPlane = farPlane;
    m_IsPerspective = false;
    m_ProjectionMatrix = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
}

void Camera3D::SetPosition(const Vec3& position) {
    m_Position = position;
    UpdateViewMatrix();
}

void Camera3D::SetRotation(const Vec3& rotation) {
    m_Rotation = rotation;
    UpdateVectors();
    UpdateViewMatrix();
}

void Camera3D::LookAt(const Vec3& target, const Vec3& up) {
    m_Forward = glm::normalize(target - m_Position);
    m_Right = glm::normalize(glm::cross(m_Forward, up));
    m_Up = glm::cross(m_Right, m_Forward);
    m_ViewMatrix = glm::lookAt(m_Position, target, up);
}

void Camera3D::Move(const Vec3& offset) {
    m_Position += offset;
    UpdateViewMatrix();
}

void Camera3D::Rotate(f32 yaw, f32 pitch) {
    m_Rotation.y += yaw;
    m_Rotation.x += pitch;
    
    // Clamp pitch
    if (m_Rotation.x > 89.0f) m_Rotation.x = 89.0f;
    if (m_Rotation.x < -89.0f) m_Rotation.x = -89.0f;
    
    UpdateVectors();
    UpdateViewMatrix();
}

void Camera3D::SetFOV(f32 fov) {
    m_FOV = fov;
    if (m_IsPerspective) {
        UpdateProjectionMatrix();
    }
}

void Camera3D::SetAspectRatio(f32 aspectRatio) {
    m_AspectRatio = aspectRatio;
    if (m_IsPerspective) {
        UpdateProjectionMatrix();
    }
}

void Camera3D::UpdateViewMatrix() {
    m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
}

void Camera3D::UpdateProjectionMatrix() {
    m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearPlane, m_FarPlane);
}

void Camera3D::UpdateVectors() {
    Vec3 front;
    front.x = cos(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
    front.y = sin(glm::radians(m_Rotation.x));
    front.z = sin(glm::radians(m_Rotation.y)) * cos(glm::radians(m_Rotation.x));
    m_Forward = glm::normalize(front);
    m_Right = glm::normalize(glm::cross(m_Forward, Vec3(0.0f, 1.0f, 0.0f)));
    m_Up = glm::normalize(glm::cross(m_Right, m_Forward));
}

// FPS Camera Controller
FPSCameraController::FPSCameraController(Camera3D* camera) : m_Camera(camera) {}

void FPSCameraController::OnUpdate(f32 deltaTime) {
    if (!m_Enabled || !m_Camera) return;
    
    auto& input = Input::Get();
    f32 velocity = m_MoveSpeed * deltaTime;
    
    Vec3 forward = m_Camera->GetForward();
    Vec3 right = m_Camera->GetRight();
    
    Vec3 movement(0.0f);
    
    if (input.IsKeyDown(Key::W)) movement += forward;
    if (input.IsKeyDown(Key::S)) movement -= forward;
    if (input.IsKeyDown(Key::A)) movement -= right;
    if (input.IsKeyDown(Key::D)) movement += right;
    if (input.IsKeyDown(Key::Space)) movement.y += 1.0f;
    if (input.IsKeyDown(Key::LeftShift)) movement.y -= 1.0f;
    
    if (glm::length(movement) > 0.0f) {
        movement = glm::normalize(movement) * velocity;
        m_Camera->Move(movement);
    }
}

void FPSCameraController::OnMouseMove(f32 xOffset, f32 yOffset) {
    if (!m_Enabled || !m_Camera) return;
    
    m_Yaw += xOffset * m_MouseSensitivity;
    m_Pitch -= yOffset * m_MouseSensitivity;
    
    if (m_Pitch > 89.0f) m_Pitch = 89.0f;
    if (m_Pitch < -89.0f) m_Pitch = -89.0f;
    
    m_Camera->SetRotation(Vec3(m_Pitch, m_Yaw, 0.0f));
}

void FPSCameraController::OnScroll(f32 yOffset) {
    if (!m_Enabled || !m_Camera) return;
    
    f32 fov = m_Camera->GetFOV() - yOffset * 2.0f;
    fov = glm::clamp(fov, 1.0f, 120.0f);
    m_Camera->SetFOV(fov);
}

// Orbit Camera Controller
OrbitCameraController::OrbitCameraController(Camera3D* camera) : m_Camera(camera) {
    UpdateCameraPosition();
}

void OrbitCameraController::OnUpdate(f32 deltaTime) {
    // Smooth interpolation could be added here
}

void OrbitCameraController::OnMouseMove(f32 xOffset, f32 yOffset, bool rotating, bool panning) {
    if (!m_Camera) return;
    
    if (rotating) {
        m_Yaw += xOffset * m_RotationSpeed;
        m_Pitch -= yOffset * m_RotationSpeed;
        m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);
        UpdateCameraPosition();
    }
    
    if (panning) {
        Vec3 right = m_Camera->GetRight();
        Vec3 up = m_Camera->GetUp();
        m_Target -= right * xOffset * m_PanSpeed * m_Distance;
        m_Target += up * yOffset * m_PanSpeed * m_Distance;
        UpdateCameraPosition();
    }
}

void OrbitCameraController::OnScroll(f32 yOffset) {
    m_Distance -= yOffset * m_ZoomSpeed;
    m_Distance = glm::clamp(m_Distance, m_MinDistance, m_MaxDistance);
    UpdateCameraPosition();
}

void OrbitCameraController::SetTarget(const Vec3& target) {
    m_Target = target;
    UpdateCameraPosition();
}

void OrbitCameraController::SetDistance(f32 distance) {
    m_Distance = glm::clamp(distance, m_MinDistance, m_MaxDistance);
    UpdateCameraPosition();
}

void OrbitCameraController::UpdateCameraPosition() {
    f32 x = m_Distance * cos(glm::radians(m_Pitch)) * cos(glm::radians(m_Yaw));
    f32 y = m_Distance * sin(glm::radians(m_Pitch));
    f32 z = m_Distance * cos(glm::radians(m_Pitch)) * sin(glm::radians(m_Yaw));
    
    Vec3 position = m_Target + Vec3(x, y, z);
    m_Camera->SetPosition(position);
    m_Camera->LookAt(m_Target);
}

} // namespace Gini
