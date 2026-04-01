#pragma once

#include "Core/Types.h"

namespace Gini {

class Camera {
public:
    Camera() = default;
    virtual ~Camera() = default;
    
    const Mat4& GetProjection() const { return m_Projection; }
    const Mat4& GetView() const { return m_View; }
    const Mat4& GetViewProjection() const { return m_ViewProjection; }
    
protected:
    Mat4 m_Projection{1.0f};
    Mat4 m_View{1.0f};
    Mat4 m_ViewProjection{1.0f};
};

// Orthographic camera for 2D RTS games
class OrthographicCamera : public Camera {
public:
    OrthographicCamera(f32 left, f32 right, f32 bottom, f32 top);
    
    void SetProjection(f32 left, f32 right, f32 bottom, f32 top);
    
    const Vec3& GetPosition() const { return m_Position; }
    void SetPosition(const Vec3& position);
    
    f32 GetRotation() const { return m_Rotation; }
    void SetRotation(f32 rotation);
    
    f32 GetZoom() const { return m_Zoom; }
    void SetZoom(f32 zoom);
    
private:
    void RecalculateViewMatrix();
    
    Vec3 m_Position{0.0f, 0.0f, 0.0f};
    f32 m_Rotation = 0.0f;
    f32 m_Zoom = 1.0f;
};

// Camera controller for RTS-style camera movement
class RTSCameraController {
public:
    RTSCameraController(f32 aspectRatio, f32 zoomLevel = 1.0f);
    
    void OnUpdate(f32 deltaTime);
    void OnResize(f32 width, f32 height);
    
    OrthographicCamera& GetCamera() { return m_Camera; }
    const OrthographicCamera& GetCamera() const { return m_Camera; }
    
    // Settings
    void SetMoveSpeed(f32 speed) { m_MoveSpeed = speed; }
    void SetZoomSpeed(f32 speed) { m_ZoomSpeed = speed; }
    void SetBounds(const Rect& bounds) { m_Bounds = bounds; m_HasBounds = true; }
    void ClearBounds() { m_HasBounds = false; }
    
    // Edge scrolling (RTS style)
    void SetEdgeScrolling(bool enabled) { m_EdgeScrollingEnabled = enabled; }
    void SetEdgeScrollMargin(f32 margin) { m_EdgeScrollMargin = margin; }
    
    f32 GetZoomLevel() const { return m_ZoomLevel; }
    void SetZoomLevel(f32 level);
    
private:
    void UpdateProjection();
    
    OrthographicCamera m_Camera;
    f32 m_AspectRatio;
    f32 m_ZoomLevel = 1.0f;
    f32 m_MinZoom = 0.25f;
    f32 m_MaxZoom = 10.0f;
    
    Vec3 m_CameraPosition{0.0f, 0.0f, 0.0f};
    f32 m_MoveSpeed = 500.0f;
    f32 m_ZoomSpeed = 0.5f;
    
    bool m_EdgeScrollingEnabled = true;
    f32 m_EdgeScrollMargin = 50.0f;
    
    bool m_HasBounds = false;
    Rect m_Bounds;
};

} // namespace Gini
