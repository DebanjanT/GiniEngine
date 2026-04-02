#include "Vegetation.h"
#include "Core/Logger.h"

#include <algorithm>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <random>

namespace Gini {

// Vegetation instanced shader
static const char *s_VegetationVertexShader = R"(
#version 410 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;

// Instance data
layout (location = 3) in vec3 a_InstancePos;
layout (location = 4) in float a_InstanceRotation;
layout (location = 5) in float a_InstanceScale;
layout (location = 6) in vec3 a_InstanceColor;

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_Color;

uniform mat4 u_View;
uniform mat4 u_Projection;
uniform float u_Time;
uniform float u_WindStrength;

mat3 rotationY(float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return mat3(c, 0, s, 0, 1, 0, -s, 0, c);
}

void main() {
    // Apply instance rotation and scale
    vec3 localPos = rotationY(a_InstanceRotation) * (a_Position * a_InstanceScale);
    
    // Wind animation (simple sine wave based on position and time)
    float windOffset = sin(u_Time * 2.0 + a_InstancePos.x * 0.5 + a_InstancePos.z * 0.3) * u_WindStrength;
    windOffset *= a_Position.y; // More wind at top
    localPos.x += windOffset;
    localPos.z += windOffset * 0.5;
    
    vec3 worldPos = localPos + a_InstancePos;
    v_WorldPos = worldPos;
    v_Normal = rotationY(a_InstanceRotation) * a_Normal;
    v_TexCoord = a_TexCoord;
    v_Color = a_InstanceColor;
    
    gl_Position = u_Projection * u_View * vec4(worldPos, 1.0);
}
)";

static const char *s_VegetationFragmentShader = R"(
#version 410 core
out vec4 FragColor;

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_Color;

uniform sampler2D u_AlbedoTexture;
uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_CameraPos;
uniform bool u_HasTexture;

void main() {
    vec4 texColor = u_HasTexture ? texture(u_AlbedoTexture, v_TexCoord) : vec4(1.0);
    
    // Alpha test for vegetation
    if (texColor.a < 0.5) discard;
    
    vec3 albedo = texColor.rgb * v_Color;
    
    // Simple lighting
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(-u_LightDir);
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Subsurface scattering approximation for leaves
    float backlight = max(dot(-normal, lightDir), 0.0) * 0.3;
    
    vec3 ambient = 0.3 * albedo;
    vec3 diffuse = (diff + backlight) * albedo * u_LightColor;
    
    vec3 color = ambient + diffuse;
    color = pow(color, vec3(1.0/2.2)); // Gamma
    
    FragColor = vec4(color, texColor.a);
}
)";

// Billboard shader for grass
static const char *s_BillboardVertexShader = R"(
#version 410 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoord;

// Instance data
layout (location = 2) in vec3 a_InstancePos;
layout (location = 3) in float a_InstanceRotation;
layout (location = 4) in float a_InstanceScale;
layout (location = 5) in vec3 a_InstanceColor;

out vec2 v_TexCoord;
out vec3 v_Color;

uniform mat4 u_View;
uniform mat4 u_Projection;
uniform vec3 u_CameraRight;
uniform vec3 u_CameraUp;
uniform float u_Time;
uniform float u_WindStrength;

void main() {
    // Billboard facing camera
    vec3 right = u_CameraRight * a_Position.x * a_InstanceScale;
    vec3 up = u_CameraUp * a_Position.y * a_InstanceScale;
    
    vec3 worldPos = a_InstancePos + right + up;
    
    // Wind
    float windOffset = sin(u_Time * 3.0 + a_InstancePos.x * 0.7 + a_InstancePos.z * 0.5) * u_WindStrength;
    windOffset *= a_Position.y;
    worldPos.x += windOffset;
    
    v_TexCoord = a_TexCoord;
    v_Color = a_InstanceColor;
    
    gl_Position = u_Projection * u_View * vec4(worldPos, 1.0);
}
)";

static const char *s_BillboardFragmentShader = R"(
#version 410 core
out vec4 FragColor;

in vec2 v_TexCoord;
in vec3 v_Color;

uniform sampler2D u_AlbedoTexture;

void main() {
    vec4 texColor = texture(u_AlbedoTexture, v_TexCoord);
    if (texColor.a < 0.5) discard;
    
    vec3 color = texColor.rgb * v_Color;
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, texColor.a);
}
)";

VegetationSystem::VegetationSystem() {
  InitShader();
  GINI_INFO("Vegetation system initialized");
}

VegetationSystem::~VegetationSystem() { ClearAllInstances(); }

void VegetationSystem::InitShader() {
  m_Shader =
      Shader::Create(s_VegetationVertexShader, s_VegetationFragmentShader);
  m_BillboardShader =
      Shader::Create(s_BillboardVertexShader, s_BillboardFragmentShader);
}

u32 VegetationSystem::AddVegetationType(const VegetationType &type) {
  m_Types.push_back(type);
  m_Cells.push_back({});
  return static_cast<u32>(m_Types.size() - 1);
}

void VegetationSystem::RemoveVegetationType(u32 typeIndex) {
  if (typeIndex < m_Types.size()) {
    ClearInstances(typeIndex);
    m_Types.erase(m_Types.begin() + typeIndex);
    m_Cells.erase(m_Cells.begin() + typeIndex);
  }
}

void VegetationSystem::AddInstance(u32 typeIndex, const Vec3 &position,
                                   f32 rotation, f32 scale) {
  if (typeIndex >= m_Types.size())
    return;

  VegetationInstance instance;
  instance.position = position;
  instance.rotation = rotation;
  instance.scale = scale;
  instance.color = Vec3(1.0f); // Default white tint

  // Add color variation
  auto &type = m_Types[typeIndex];
  static std::mt19937 rng(42);
  std::uniform_real_distribution<f32> dist(-1.0f, 1.0f);
  instance.color.r += dist(rng) * type.colorVariation.r;
  instance.color.g += dist(rng) * type.colorVariation.g;
  instance.color.b += dist(rng) * type.colorVariation.b;
  instance.color = glm::clamp(instance.color, Vec3(0.5f), Vec3(1.5f));

  // Get cell for this position
  Vec2 cellCoord = GetCellCoord(position);
  u64 cellKey =
      (static_cast<u64>(static_cast<i32>(cellCoord.x) + 10000) << 32) |
      (static_cast<u64>(static_cast<i32>(cellCoord.y) + 10000));

  auto &cell = m_Cells[typeIndex][cellKey];
  cell.instances.push_back(instance);
  cell.dirty = true;

  // Update cell bounds
  if (cell.instances.size() == 1) {
    cell.center = position;
    cell.radius = 0.0f;
  } else {
    // Expand bounds
    f32 dist = glm::length(position - cell.center);
    if (dist > cell.radius) {
      cell.radius = dist;
    }
  }
}

void VegetationSystem::RemoveInstancesInRadius(u32 typeIndex,
                                               const Vec3 &center, f32 radius) {
  if (typeIndex >= m_Types.size())
    return;

  f32 radiusSq = radius * radius;

  for (auto &[key, cell] : m_Cells[typeIndex]) {
    auto &instances = cell.instances;
    instances.erase(std::remove_if(instances.begin(), instances.end(),
                                   [&](const VegetationInstance &inst) {
                                     Vec3 diff = inst.position - center;
                                     return glm::dot(diff, diff) < radiusSq;
                                   }),
                    instances.end());
    cell.dirty = true;
  }
}

void VegetationSystem::ClearInstances(u32 typeIndex) {
  if (typeIndex >= m_Cells.size())
    return;

  for (auto &[key, cell] : m_Cells[typeIndex]) {
    if (cell.instanceVBO) {
      glDeleteBuffers(1, &cell.instanceVBO);
    }
  }
  m_Cells[typeIndex].clear();
}

void VegetationSystem::ClearAllInstances() {
  for (u32 i = 0; i < m_Cells.size(); i++) {
    ClearInstances(i);
  }
}

void VegetationSystem::Paint(u32 typeIndex, const Vec3 &center, f32 radius,
                             f32 density, Terrain *terrain) {
  if (typeIndex >= m_Types.size())
    return;

  auto &type = m_Types[typeIndex];

  static std::mt19937 rng(std::random_device{}());
  std::uniform_real_distribution<f32> posDist(-radius, radius);
  std::uniform_real_distribution<f32> rotDist(0.0f, glm::two_pi<f32>());
  std::uniform_real_distribution<f32> scaleDist(type.minScale, type.maxScale);

  // Calculate number of instances based on density
  f32 area = glm::pi<f32>() * radius * radius;
  i32 count = static_cast<i32>(area * density * m_DensityMultiplier);

  for (i32 i = 0; i < count; i++) {
    f32 dx = posDist(rng);
    f32 dz = posDist(rng);

    // Check if within radius
    if (dx * dx + dz * dz > radius * radius)
      continue;

    Vec3 pos = center + Vec3(dx, 0.0f, dz);

    // Get height from terrain
    if (terrain) {
      pos.y = terrain->GetHeightAtPosition(pos.x, pos.z);

      // Check slope
      Vec3 normal = terrain->GetNormalAtPosition(pos.x, pos.z);
      f32 slope = std::acos(glm::dot(normal, Vec3(0, 1, 0)));
      if (slope < type.minSlope || slope > type.maxSlope)
        continue;

      // Check height
      if (pos.y < type.minHeight || pos.y > type.maxHeight)
        continue;
    }

    f32 rotation = rotDist(rng);
    f32 scale = scaleDist(rng);

    AddInstance(typeIndex, pos, rotation, scale);
  }
}

void VegetationSystem::Erase(u32 typeIndex, const Vec3 &center, f32 radius) {
  RemoveInstancesInRadius(typeIndex, center, radius);
}

void VegetationSystem::UpdateCulling(const Camera3D &camera) {
  m_VisibleInstances = 0;

  for (u32 typeIdx = 0; typeIdx < m_Types.size(); typeIdx++) {
    auto &type = m_Types[typeIdx];

    for (auto &[key, cell] : m_Cells[typeIdx]) {
      if (cell.instances.empty()) {
        cell.visible = false;
        continue;
      }

      // Distance culling
      if (m_DistanceCulling) {
        f32 dist = glm::length(cell.center - camera.GetPosition());
        if (dist - cell.radius > type.cullDistance) {
          cell.visible = false;
          continue;
        }
      }

      // Frustum culling
      if (m_FrustumCulling) {
        cell.visible =
            IsInFrustum(cell.center, cell.radius + m_CellSize, camera);
      } else {
        cell.visible = true;
      }

      if (cell.visible) {
        m_VisibleInstances += static_cast<u32>(cell.instances.size());
      }
    }
  }
}

bool VegetationSystem::IsInFrustum(const Vec3 &center, f32 radius,
                                   const Camera3D &camera) const {
  // Simple frustum check using view-projection matrix
  Mat4 vp = camera.GetProjectionMatrix() * camera.GetViewMatrix();
  Vec4 clipPos = vp * Vec4(center, 1.0f);

  // Check if sphere is completely outside any frustum plane
  f32 w = clipPos.w + radius;
  if (clipPos.x < -w || clipPos.x > w)
    return false;
  if (clipPos.y < -w || clipPos.y > w)
    return false;
  if (clipPos.z < -w || clipPos.z > w)
    return false;

  return true;
}

void VegetationSystem::UpdateInstanceBuffer(u32 typeIndex,
                                            VegetationCell &cell) {
  if (!cell.dirty)
    return;

  if (cell.instances.empty()) {
    if (cell.instanceVBO) {
      glDeleteBuffers(1, &cell.instanceVBO);
      cell.instanceVBO = 0;
    }
    cell.instanceCount = 0;
    cell.dirty = false;
    return;
  }

  // Create instance data buffer
  // Layout: position (3) + rotation (1) + scale (1) + color (3) = 8 floats
  std::vector<f32> instanceData;
  instanceData.reserve(cell.instances.size() * 8);

  for (const auto &inst : cell.instances) {
    instanceData.push_back(inst.position.x);
    instanceData.push_back(inst.position.y);
    instanceData.push_back(inst.position.z);
    instanceData.push_back(inst.rotation);
    instanceData.push_back(inst.scale);
    instanceData.push_back(inst.color.r);
    instanceData.push_back(inst.color.g);
    instanceData.push_back(inst.color.b);
  }

  if (!cell.instanceVBO) {
    glGenBuffers(1, &cell.instanceVBO);
  }

  glBindBuffer(GL_ARRAY_BUFFER, cell.instanceVBO);
  glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(f32),
               instanceData.data(), GL_DYNAMIC_DRAW);

  cell.instanceCount = static_cast<u32>(cell.instances.size());
  cell.dirty = false;
}

u32 VegetationSystem::GetLODLevel(f32 distance, u32 typeIndex) const {
  if (typeIndex >= m_Types.size())
    return 0;

  const auto &lods = m_Types[typeIndex].lods;
  for (u32 i = 0; i < lods.size(); i++) {
    if (distance < lods[i].maxDistance) {
      return i;
    }
  }
  return static_cast<u32>(lods.size() - 1);
}

void VegetationSystem::RenderCell(u32 typeIndex, VegetationCell &cell,
                                  const Camera3D &camera, u32 lodLevel) {
  if (!cell.visible || cell.instanceCount == 0)
    return;

  auto &type = m_Types[typeIndex];
  if (lodLevel >= type.lods.size() || !type.lods[lodLevel].mesh)
    return;

  auto &mesh = type.lods[lodLevel].mesh;

  // Bind mesh VAO
  glBindVertexArray(mesh->GetVAO());

  // Setup instance attributes
  glBindBuffer(GL_ARRAY_BUFFER, cell.instanceVBO);

  // Instance position
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), (void *)0);
  glVertexAttribDivisor(3, 1);

  // Instance rotation
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(f32),
                        (void *)(3 * sizeof(f32)));
  glVertexAttribDivisor(4, 1);

  // Instance scale
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(f32),
                        (void *)(4 * sizeof(f32)));
  glVertexAttribDivisor(5, 1);

  // Instance color
  glEnableVertexAttribArray(6);
  glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32),
                        (void *)(5 * sizeof(f32)));
  glVertexAttribDivisor(6, 1);

  // Draw instanced
  glDrawElementsInstanced(GL_TRIANGLES, mesh->GetIndexCount(), GL_UNSIGNED_INT,
                          0, cell.instanceCount);

  m_DrawCallCount++;
}

void VegetationSystem::Render(const Camera3D &camera) {
  m_DrawCallCount = 0;

  UpdateCulling(camera);

  // Get time for wind animation
  static f32 time = 0.0f;
  time += 0.016f; // Approximate 60fps

  for (u32 typeIdx = 0; typeIdx < m_Types.size(); typeIdx++) {
    auto &type = m_Types[typeIdx];

    if (type.lods.empty())
      continue;

    // Choose shader
    Ref<Shader> shader = type.billboard ? m_BillboardShader : m_Shader;
    shader->Bind();

    // Set uniforms
    shader->SetMat4("u_View", camera.GetViewMatrix());
    shader->SetMat4("u_Projection", camera.GetProjectionMatrix());
    shader->SetVec3("u_CameraPos", camera.GetPosition());
    shader->SetFloat("u_Time", time);
    shader->SetFloat("u_WindStrength", type.windStrength);

    if (type.billboard) {
      // Billboard camera vectors
      Mat4 view = camera.GetViewMatrix();
      Vec3 right = Vec3(view[0][0], view[1][0], view[2][0]);
      Vec3 up = Vec3(view[0][1], view[1][1], view[2][1]);
      shader->SetVec3("u_CameraRight", right);
      shader->SetVec3("u_CameraUp", up);
    } else {
      shader->SetVec3("u_LightDir", Vec3(-0.2f, -1.0f, -0.3f));
      shader->SetVec3("u_LightColor", Vec3(1.0f));
    }

    // Bind texture
    if (type.albedoTexture) {
      type.albedoTexture->Bind(0);
      shader->SetInt("u_AlbedoTexture", 0);
      shader->SetInt("u_HasTexture", 1);
    } else {
      shader->SetInt("u_HasTexture", 0);
    }

    // Render all visible cells
    for (auto &[key, cell] : m_Cells[typeIdx]) {
      if (!cell.visible)
        continue;

      // Update instance buffer if needed
      UpdateInstanceBuffer(typeIdx, cell);

      // Get LOD level based on distance
      f32 dist = glm::length(cell.center - camera.GetPosition());
      u32 lod = GetLODLevel(dist, typeIdx);

      RenderCell(typeIdx, cell, camera, lod);
    }
  }

  // Reset vertex attrib divisors
  for (u32 i = 3; i <= 6; i++) {
    glVertexAttribDivisor(i, 0);
  }
}

void VegetationSystem::RenderShadows(const Camera3D &lightCamera) {
  // Similar to Render but with shadow shader
  // TODO: Implement shadow rendering
}

Vec2 VegetationSystem::GetCellCoord(const Vec3 &position) const {
  return Vec2(std::floor(position.x / m_CellSize),
              std::floor(position.z / m_CellSize));
}

void VegetationSystem::RebuildCells() {
  // Rebuild all cells with new cell size
  for (u32 typeIdx = 0; typeIdx < m_Types.size(); typeIdx++) {
    std::vector<VegetationInstance> allInstances;

    // Collect all instances
    for (auto &[key, cell] : m_Cells[typeIdx]) {
      allInstances.insert(allInstances.end(), cell.instances.begin(),
                          cell.instances.end());
      if (cell.instanceVBO) {
        glDeleteBuffers(1, &cell.instanceVBO);
      }
    }

    m_Cells[typeIdx].clear();

    // Re-add all instances
    for (const auto &inst : allInstances) {
      AddInstance(typeIdx, inst.position, inst.rotation, inst.scale);
    }
  }
}

u32 VegetationSystem::GetTotalInstanceCount() const {
  u32 total = 0;
  for (const auto &cells : m_Cells) {
    for (const auto &[key, cell] : cells) {
      total += static_cast<u32>(cell.instances.size());
    }
  }
  return total;
}

u32 VegetationSystem::GetVisibleInstanceCount() const {
  return m_VisibleInstances;
}

Ref<VegetationSystem> VegetationSystem::Create() {
  return CreateRef<VegetationSystem>();
}

} // namespace Gini
