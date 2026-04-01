#include "Gini.h"
#include "Renderer/Camera3D.h"
#include "Renderer/Light.h"
#include "Renderer/Mesh.h"
#include "Renderer/Model.h"
#include "Renderer/Renderer3D.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace Gini;

class Demo3D : public Application {
public:
  Demo3D() : Application(CreateConfig()) {}

  static EngineConfig CreateConfig() {
    EngineConfig config;
    config.windowTitle = "Gini Engine - 3D Demo";
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.vsync = true;
    return config;
  }

  void OnInit() override {
    GINI_INFO("3D Demo initialized!");

    // Initialize 3D renderer
    Renderer3D::Init();
    Renderer3D::SetClearColor(Color(0.1f, 0.1f, 0.15f));

    // Setup camera
    m_Camera = CreateScope<Camera3D>(45.0f, GetWindow().GetAspectRatio(), 0.1f,
                                     1000.0f);
    m_Camera->SetPosition(Vec3(0.0f, 5.0f, 10.0f));
    m_Camera->LookAt(Vec3(0.0f, 0.0f, 0.0f));

    m_CameraController = CreateScope<OrbitCameraController>(m_Camera.get());
    m_CameraController->SetDistance(15.0f);

    // Setup lighting
    auto &lights = LightManager::Get();
    lights.Clear();

    // Ambient light
    AmbientLight ambient;
    ambient.color = Vec3(0.1f, 0.1f, 0.15f);
    ambient.intensity = 1.0f;
    lights.SetAmbientLight(ambient);

    // Directional light (sun)
    DirectionalLight sun;
    sun.direction = Vec3(-0.5f, -1.0f, -0.3f);
    sun.color = Vec3(1.0f, 0.95f, 0.9f);
    sun.intensity = 1.0f;
    lights.SetDirectionalLight(sun);

    // Point lights
    PointLight redLight;
    redLight.position = Vec3(-5.0f, 3.0f, 0.0f);
    redLight.color = Vec3(1.0f, 0.2f, 0.2f);
    redLight.intensity = 5.0f;
    lights.AddPointLight(redLight);

    PointLight blueLight;
    blueLight.position = Vec3(5.0f, 3.0f, 0.0f);
    blueLight.color = Vec3(0.2f, 0.2f, 1.0f);
    blueLight.intensity = 5.0f;
    lights.AddPointLight(blueLight);

    // Create primitive meshes
    m_CubeMesh = Mesh::CreateCube(1.0f);
    m_SphereMesh = Mesh::CreateSphere(1.0f, 32, 16);
    m_PlaneMesh = Mesh::CreatePlane(20.0f, 20.0f);

    // Load terrain model (use path relative to project root, not build dir)
    m_TerrainModel =
        Model::Create("../Samples/Demo3D/model/terrian/terrian.obj");
    if (m_TerrainModel) {
      GINI_INFO("Terrain model loaded successfully!");
      // Load terrain texture manually and apply to material
      m_TerrainTexture =
          Texture2D::Create("../Samples/Demo3D/model/terrian/terrian.png");
    } else {
      GINI_ERROR("Failed to load terrain model!");
    }

    // Setup materials
    m_FloorMaterial.albedo = Vec3(0.3f, 0.3f, 0.35f);
    m_FloorMaterial.metallic = 0.0f;
    m_FloorMaterial.roughness = 0.8f;

    m_MetalMaterial.albedo = Vec3(0.9f, 0.9f, 0.9f);
    m_MetalMaterial.metallic = 1.0f;
    m_MetalMaterial.roughness = 0.2f;

    m_RoughMaterial.albedo = Vec3(0.8f, 0.2f, 0.2f);
    m_RoughMaterial.metallic = 0.0f;
    m_RoughMaterial.roughness = 0.9f;

    m_GoldMaterial.albedo = Vec3(1.0f, 0.765f, 0.336f);
    m_GoldMaterial.metallic = 1.0f;
    m_GoldMaterial.roughness = 0.3f;
  }

  void OnShutdown() override {
    Renderer3D::Shutdown();
    GINI_INFO("3D Demo shutdown!");
  }

  void OnUpdate(f32 dt) override {
    HandleInput(dt);
    m_CameraController->OnUpdate(dt);

    // Rotate objects
    m_Rotation += dt * 30.0f;
  }

  void OnRender() override {
    Renderer3D::Clear();
    Renderer3D::BeginScene(*m_Camera);

    // Draw terrain model instead of floor plane
    if (m_TerrainModel) {
      Mat4 terrainTransform =
          glm::translate(Mat4(1.0f), Vec3(0.0f, -5.0f, 0.0f));
      terrainTransform =
          glm::scale(terrainTransform, Vec3(0.1f)); // Scale down if needed

      // Create terrain material with texture
      Material3D terrainMat;
      terrainMat.albedo = Vec3(1.0f);
      terrainMat.metallic = 0.0f;
      terrainMat.roughness = 0.8f;
      terrainMat.albedoMap = m_TerrainTexture;

      // Draw each mesh in the model with the terrain material
      const auto &meshes = m_TerrainModel->GetMeshes();
      for (const auto &mesh : meshes) {
        Renderer3D::DrawMesh(mesh, terrainTransform, terrainMat);
      }
    }

    // Draw spheres in a row with different materials
    f32 spacing = 3.0f;

    // Metal sphere
    Mat4 metalTransform =
        glm::translate(Mat4(1.0f), Vec3(-spacing * 1.5f, 1.0f, 0.0f));
    Renderer3D::DrawMesh(m_SphereMesh, metalTransform, m_MetalMaterial);

    // Rough sphere
    Mat4 roughTransform =
        glm::translate(Mat4(1.0f), Vec3(-spacing * 0.5f, 1.0f, 0.0f));
    Renderer3D::DrawMesh(m_SphereMesh, roughTransform, m_RoughMaterial);

    // Gold sphere
    Mat4 goldTransform =
        glm::translate(Mat4(1.0f), Vec3(spacing * 0.5f, 1.0f, 0.0f));
    Renderer3D::DrawMesh(m_SphereMesh, goldTransform, m_GoldMaterial);

    // Rotating cube
    Mat4 cubeTransform =
        glm::translate(Mat4(1.0f), Vec3(spacing * 1.5f, 1.0f, 0.0f));
    cubeTransform = glm::rotate(cubeTransform, glm::radians(m_Rotation),
                                Vec3(0.0f, 1.0f, 0.0f));
    cubeTransform = glm::rotate(cubeTransform, glm::radians(m_Rotation * 0.5f),
                                Vec3(1.0f, 0.0f, 0.0f));

    Material3D cubeMaterial;
    cubeMaterial.albedo = Vec3(0.2f, 0.8f, 0.3f);
    cubeMaterial.metallic = 0.5f;
    cubeMaterial.roughness = 0.4f;
    Renderer3D::DrawMesh(m_CubeMesh, cubeTransform, cubeMaterial);

    // Draw light indicators
    auto &lights = LightManager::Get();
    for (u32 i = 0; i < lights.GetPointLightCount(); i++) {
      const auto &light = lights.GetPointLights()[i];
      Mat4 lightTransform = glm::translate(Mat4(1.0f), light.position);
      lightTransform = glm::scale(lightTransform, Vec3(0.2f));

      Material3D lightMat;
      lightMat.albedo = light.color;
      lightMat.emissive = light.color * light.intensity;
      Renderer3D::DrawMesh(m_SphereMesh, lightTransform, lightMat);
    }

    Renderer3D::EndScene();
  }

  void OnEvent(Event &event) override {
    if (event.GetType() == EventType::WindowResize) {
      auto &e = static_cast<WindowResizeEvent &>(event);
      m_Camera->SetAspectRatio(static_cast<f32>(e.GetWidth()) /
                               static_cast<f32>(e.GetHeight()));
      Renderer3D::SetViewport(0, 0, e.GetWidth(), e.GetHeight());
    }

    if (event.GetType() == EventType::MouseScrolled) {
      auto &e = static_cast<MouseScrolledEvent &>(event);
      m_CameraController->OnScroll(e.GetYOffset());
    }
  }

private:
  void HandleInput(f32 dt) {
    auto &input = Input::Get();

    // Camera rotation with mouse
    if (input.IsMouseButtonDown(MouseButton::Right)) {
      Vec2 delta = input.GetMouseDelta();
      m_CameraController->OnMouseMove(delta.x, delta.y, true, false);
    }

    if (input.IsMouseButtonDown(MouseButton::Middle)) {
      Vec2 delta = input.GetMouseDelta();
      m_CameraController->OnMouseMove(delta.x, delta.y, false, true);
    }

    // Toggle wireframe
    if (input.IsKeyPressed(Key::F1)) {
      m_Wireframe = !m_Wireframe;
      Renderer3D::SetWireframeMode(m_Wireframe);
    }

    // Reset camera
    if (input.IsKeyPressed(Key::R)) {
      m_CameraController->SetTarget(Vec3(0.0f));
      m_CameraController->SetDistance(15.0f);
    }
  }

  Scope<Camera3D> m_Camera;
  Scope<OrbitCameraController> m_CameraController;

  Ref<Mesh> m_CubeMesh;
  Ref<Mesh> m_SphereMesh;
  Ref<Mesh> m_PlaneMesh;

  Ref<Model> m_TerrainModel;
  Ref<Texture2D> m_TerrainTexture;

  Material3D m_FloorMaterial;
  Material3D m_MetalMaterial;
  Material3D m_RoughMaterial;
  Material3D m_GoldMaterial;

  f32 m_Rotation = 0.0f;
  bool m_Wireframe = false;
};

Gini::Application *Gini::CreateApplication() { return new Demo3D(); }

int main(int argc, char **argv) {
  auto app = Gini::CreateApplication();
  app->Run();
  delete app;
  return 0;
}
