#pragma once

#include "Core/Types.h"

namespace Gini {

class Camera3D {
public:
    Camera3D(f32 fov = 45.0f, f32 aspectRatio = 16.0f/9.0f, f32 nearPlane = 0.1f, f32 farPlane = 1000.0f);
    
    void SetPerspective(f32 fov, f32 aspectRatio, f32 nearPlane, f32 farPlane);
    void SetOrthographic(f32 left, f32 right, f32 bottom, f32 top, f32 nearPlane, f32 farPlane);
    
    void SetPosition(const Vec3& position);
    void SetRotation(const Vec3& rotation);
    void LookAt(const Vec3& target, const Vec3& up = Vec3(0, 1, 0));
    
    void Move(const Vec3& offset);
    void Rotate(f32 yaw, f32 pitch);
    
    const Mat4& GetViewMatrix() const { return m_ViewMatrix; }
    const Mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
    Mat4 GetViewProjectionMatrix() const { return m_ProjectionMatrix * m_ViewMatrix; }
    
    const Vec3& GetPosition() const { return m_Position; }
    const Vec3& GetRotation() const { return m_Rotation; }
    Vec3 GetForward() const { return m_Forward; }
    Vec3 GetRight() const { return m_Right; }
    Vec3 GetUp() const { return m_Up; }
    
    f32 GetFOV() const { return m_FOV; }
    f32 GetAspectRatio() const { return m_AspectRatio; }
    f32 GetNearPlane() const { return m_NearPlane; }
    f32 GetFarPlane() const { return m_FarPlane; }
    
    void SetFOV(f32 fov);
    void SetAspectRatio(f32 aspectRatio);
    
    bool IsPerspective() const { return m_IsPerspective; }
    
private:
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();
    void UpdateVectors();
    
    Vec3 m_Position = Vec3(0.0f, 0.0f, 5.0f);
    Vec3 m_Rotation = Vec3(0.0f); // pitch, yaw, roll
    
    Vec3 m_Forward = Vec3(0.0f, 0.0f, -1.0f);
    Vec3 m_Right = Vec3(1.0f, 0.0f, 0.0f);
    Vec3 m_Up = Vec3(0.0f, 1.0f, 0.0f);
    
    Mat4 m_ViewMatrix = Mat4(1.0f);
    Mat4 m_ProjectionMatrix = Mat4(1.0f);
    
    f32 m_FOV = 45.0f;
    f32 m_AspectRatio = 16.0f / 9.0f;
    f32 m_NearPlane = 0.1f;
    f32 m_FarPlane = 1000.0f;
    
    bool m_IsPerspective = true;
};

class FPSCameraController {
public:
    FPSCameraController(Camera3D* camera);
    
    void OnUpdate(f32 deltaTime);
    void OnMouseMove(f32 xOffset, f32 yOffset);
    void OnScroll(f32 yOffset);
    
    void SetMoveSpeed(f32 speed) { m_MoveSpeed = speed; }
    void SetMouseSensitivity(f32 sensitivity) { m_MouseSensitivity = sensitivity; }
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    
    f32 GetMoveSpeed() const { return m_MoveSpeed; }
    bool IsEnabled() const { return m_Enabled; }
    
private:
    Camera3D* m_Camera;
    f32 m_MoveSpeed = 5.0f;
    f32 m_MouseSensitivity = 0.1f;
    f32 m_Yaw = -90.0f;
    f32 m_Pitch = 0.0f;
    bool m_Enabled = true;
};

class OrbitCameraController {
public:
    OrbitCameraController(Camera3D* camera);
    
    void OnUpdate(f32 deltaTime);
    void OnMouseMove(f32 xOffset, f32 yOffset, bool rotating, bool panning);
    void OnScroll(f32 yOffset);
    
    void SetTarget(const Vec3& target);
    void SetDistance(f32 distance);
    
    const Vec3& GetTarget() const { return m_Target; }
    f32 GetDistance() const { return m_Distance; }
    
private:
    void UpdateCameraPosition();
    
    Camera3D* m_Camera;
    Vec3 m_Target = Vec3(0.0f);
    f32 m_Distance = 10.0f;
    f32 m_Yaw = 0.0f;
    f32 m_Pitch = 30.0f;
    f32 m_MinDistance = 1.0f;
    f32 m_MaxDistance = 100.0f;
    f32 m_RotationSpeed = 0.3f;
    f32 m_PanSpeed = 0.01f;
    f32 m_ZoomSpeed = 1.0f;
};

} // namespace Gini
