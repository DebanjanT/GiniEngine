#include "RuntimeLayer.h"
#include "Renderer/Renderer3D.h"

namespace Gini {

RuntimeLayer::RuntimeLayer() {}

RuntimeLayer::~RuntimeLayer() {}

void RuntimeLayer::OnAttach() {
  // Initialize Renderer3D
  Renderer3D::Init();

  // Create framebuffer for rendering
  FramebufferSpec fbSpec;
  fbSpec.width = m_ViewportWidth;
  fbSpec.height = m_ViewportHeight;
  m_Framebuffer = CreateRef<Framebuffer>(fbSpec);

  // Create camera
  m_Camera = CreateRef<Camera3D>(45.0f, (float)m_ViewportWidth /
                                            (float)m_ViewportHeight);
  m_Camera->SetPosition(Vec3(0.0f, 5.0f, 10.0f));

  // Create default scene with some test entities
  if (!m_RuntimeScene) {
    m_RuntimeScene = CreateRef<Scene>("Runtime Scene");

    // Create test cubes
    auto cube1 = m_RuntimeScene->CreateEntity("Cube 1");
    auto &t1 =
        m_RuntimeScene->GetWorld().GetComponent<TransformComponent>(cube1);
    t1.position = Vec3(-2.0f, 0.5f, 0.0f);

    auto cube2 = m_RuntimeScene->CreateEntity("Cube 2");
    auto &t2 =
        m_RuntimeScene->GetWorld().GetComponent<TransformComponent>(cube2);
    t2.position = Vec3(0.0f, 0.5f, 0.0f);

    auto cube3 = m_RuntimeScene->CreateEntity("Cube 3");
    auto &t3 =
        m_RuntimeScene->GetWorld().GetComponent<TransformComponent>(cube3);
    t3.position = Vec3(2.0f, 0.5f, 0.0f);
  }

  OnScenePlay();
}

void RuntimeLayer::OnDetach() {
  OnSceneStop();
  m_RuntimeScene = nullptr;
}

void RuntimeLayer::OnScenePlay() {
  m_IsPlaying = true;
  // Scene runtime start - entities are now active
}

void RuntimeLayer::OnSceneStop() {
  m_IsPlaying = false;
  // Scene runtime stop - entities are now inactive
}

void RuntimeLayer::OnUpdate(f32 deltaTime) {
  if (!m_RuntimeScene || !m_IsPlaying)
    return;

  // Update scene entities here
  // TODO: Add script/behavior system updates
}

void RuntimeLayer::OnRender() {
  if (!m_RuntimeScene || !m_Camera)
    return;

  // Render directly to screen (no framebuffer for now)
  Renderer3D::SetViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
  Renderer3D::Clear();
  Renderer3D::SetClearColor(Color(0.1f, 0.1f, 0.15f));

  Renderer3D::BeginScene(*m_Camera);

  // Draw grid
  for (int i = -10; i <= 10; i++) {
    Color gridColor =
        (i == 0) ? Color(0.5f, 0.5f, 0.5f) : Color(0.3f, 0.3f, 0.3f);
    Renderer3D::DrawLine(Vec3(i, 0, -10), Vec3(i, 0, 10), gridColor);
    Renderer3D::DrawLine(Vec3(-10, 0, i), Vec3(10, 0, i), gridColor);
  }

  // Render scene entities
  auto &world = m_RuntimeScene->GetWorld();
  auto view = world.GetRegistry().view<TransformComponent>();

  int entityIndex = 0;
  for (auto entity : view) {
    auto &transform = view.get<TransformComponent>(entity);

    // Assign colors based on entity index
    Color cubeColor;
    switch (entityIndex % 3) {
    case 0:
      cubeColor = Color(0.8f, 0.3f, 0.3f);
      break; // Red
    case 1:
      cubeColor = Color(0.3f, 0.8f, 0.3f);
      break; // Green
    case 2:
      cubeColor = Color(0.3f, 0.3f, 0.8f);
      break; // Blue
    }

    Renderer3D::DrawCube(transform.position, transform.scale, cubeColor);
    entityIndex++;
  }

  Renderer3D::EndScene();
}

void RuntimeLayer::OnEvent(Event &event) {
  if (!m_RuntimeScene)
    return;

  // Handle runtime events
}

void RuntimeLayer::LoadScene(const std::string &scenePath) {
  // TODO: Implement scene loading from file
  m_RuntimeScene = CreateRef<Scene>("Loaded Scene");
}

void RuntimeLayer::SetScene(Ref<Scene> scene) {
  if (m_IsPlaying && m_RuntimeScene) {
    OnSceneStop();
  }

  m_RuntimeScene = scene;

  if (m_IsPlaying) {
    OnScenePlay();
  }
}

} // namespace Gini
