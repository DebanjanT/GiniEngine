#pragma once

#include "Core/Types.h"
#include "ECS/Components.h"
#include "Renderer/Camera3D.h"
#include "Renderer/Light.h"
#include "Renderer/Material.h"
#include "Renderer/Mesh.h"
#include "Renderer/MeshSource.h"
#include "Renderer/MaterialAsset.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture.h"

namespace Gini {

// Draw command for MeshSource-based rendering (Hazel-style)
struct MeshDrawCommand {
  Ref<MeshSource> meshSource;
  Ref<MaterialTable> materialTable;  // Can be null for material override
  u32 submeshIndex = 0;
  Mat4 transform{1.0f};
  bool isRigged = false;
  
  // For skeletal animation
  u32 boneTransformsOffset = 0;
  u32 boneTransformsStride = 0;
};

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

  // MeshSource-based rendering (Hazel-style)
  // Renders a submesh using materials from MaterialTable
  static void RenderMesh(const MeshDrawCommand &drawCmd);
  
  // Renders a submesh with explicit material override
  static void RenderMesh(const MeshDrawCommand &drawCmd,
                         Ref<MaterialAsset> materialOverride);
  
  // Renders all submeshes of a MeshSource
  static void RenderMeshSource(Ref<MeshSource> meshSource, const Mat4 &transform,
                               Ref<MaterialTable> materials = nullptr);
  
  // Renders a StaticMeshComponent
  static void RenderStaticMesh(u64 meshSourceHandle, const Mat4 &transform,
                               const std::vector<u32> &submeshIndices,
                               const std::vector<u64> &materialOverrides);

  // Renders a primitive mesh with material
  static void RenderPrimitive(MeshType primitiveType, const Mat4 &transform,
                              const Material3D &material);

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
