#pragma once

#include "Core/Types.h"
#include "Renderer/Camera3D.h"
#include "Renderer/Light.h"
#include "Renderer/Mesh.h"
#include "Renderer/Model.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture.h"

namespace Gini {

struct Renderer3DStats {
  u32 drawCalls = 0;
  u32 triangles = 0;
  u32 vertices = 0;
  u32 meshesDrawn = 0;

  void Reset() {
    drawCalls = 0;
    triangles = 0;
    vertices = 0;
    meshesDrawn = 0;
  }
};

class Renderer3D {
public:
  static void Init();
  static void Shutdown();

  static void BeginScene(const Camera3D &camera);
  static void BeginScene(const Mat4 &viewMatrix, const Mat4 &projectionMatrix,
                         const Vec3 &cameraPosition);
  static void EndScene();

  static void SetClearColor(const Color &color);
  static void Clear();
  static void SetViewport(i32 x, i32 y, i32 width, i32 height);

  // Mesh drawing
  static void DrawMesh(const Ref<Mesh> &mesh, const Mat4 &transform,
                       const Color &color = Color::White());
  static void DrawMesh(const Ref<Mesh> &mesh, const Mat4 &transform,
                       const Ref<Texture2D> &texture);
  static void DrawMesh(const Ref<Mesh> &mesh, const Mat4 &transform,
                       const Material3D &material);

  // Model drawing
  static void DrawModel(const Ref<Model> &model, const Mat4 &transform);
  static void DrawModel(const Ref<Model> &model, const Vec3 &position,
                        const Vec3 &rotation = Vec3(0.0f),
                        const Vec3 &scale = Vec3(1.0f));

  // Primitive shapes
  static void DrawCube(const Vec3 &position, const Vec3 &size,
                       const Color &color);
  static void DrawCube(const Vec3 &position, const Vec3 &size,
                       const Ref<Texture2D> &texture);
  static void DrawSphere(const Vec3 &position, f32 radius, const Color &color);
  static void DrawPlane(const Vec3 &position, const Vec2 &size,
                        const Color &color);

  // Skybox
  static void DrawSkybox(const Ref<class TextureCube> &cubemap);

  // Debug drawing
  static void DrawLine(const Vec3 &start, const Vec3 &end, const Color &color);
  static void DrawWireCube(const Vec3 &position, const Vec3 &size,
                           const Color &color);
  static void DrawWireSphere(const Vec3 &position, f32 radius,
                             const Color &color);
  static void DrawGrid(f32 size, u32 divisions, const Color &color);

  // Skinned model drawing
  static void DrawSkinnedModel(const Ref<Model> &model, const Mat4 &transform,
                               const std::vector<Mat4> &boneMatrices);

  // Shaders
  static Ref<Shader> GetPBRShader();
  static Ref<Shader> GetSkinnedPBRShader();
  static Ref<Shader> GetBasicShader();
  static Ref<Shader> GetSkyboxShader();

  // Stats
  static const Renderer3DStats &GetStats();
  static void ResetStats();

  // Settings
  static void SetWireframeMode(bool enabled);
  static void SetDepthTest(bool enabled);
  static void SetCullFace(bool enabled);

private:
  static void InitShaders();
  static void InitPrimitives();
};

} // namespace Gini
