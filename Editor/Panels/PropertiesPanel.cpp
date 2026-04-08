#include "PropertiesPanel.h"
#include "AssetBrowserPanel.h"
#include "ECS/Components.h"
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_internal.h>

namespace Gini {

PropertiesPanel::PropertiesPanel() : EditorPanel("Properties") {}

void PropertiesPanel::OnImGuiRender() {
  if (!m_Visible)
    return;

  ImGui::Begin(m_Name.c_str(), &m_Visible);

  if (m_Scene && m_SelectedEntity != NullEntity) {
    DrawComponents(m_SelectedEntity);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Add Component button (accent colored)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.56f, 0.72f, 0.35f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.56f, 0.72f, 0.55f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.56f, 0.72f, 0.70f));
    if (ImGui::Button("+ Add Component", ImVec2(-1, 28))) {
      ImGui::OpenPopup("AddComponent");
    }

    if (ImGui::BeginPopup("AddComponent")) {
      auto &addWorld = m_Scene->GetWorld();
      if (ImGui::MenuItem("Sprite")) {
        if (!addWorld.HasComponent<SpriteComponent>(m_SelectedEntity))
          addWorld.AddComponent<SpriteComponent>(m_SelectedEntity);
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::MenuItem("Mesh")) {
        if (!addWorld.HasComponent<MeshComponent>(m_SelectedEntity))
          addWorld.AddComponent<MeshComponent>(m_SelectedEntity);
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::MenuItem("Material")) {
        if (!addWorld.HasComponent<MaterialComponent>(m_SelectedEntity))
          addWorld.AddComponent<MaterialComponent>(m_SelectedEntity);
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::MenuItem("Light")) {
        if (!addWorld.HasComponent<LightComponent>(m_SelectedEntity))
          addWorld.AddComponent<LightComponent>(m_SelectedEntity);
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::MenuItem("Camera")) {
        if (!addWorld.HasComponent<CameraComponent>(m_SelectedEntity))
          addWorld.AddComponent<CameraComponent>(m_SelectedEntity);
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
    ImGui::PopStyleColor(3);
  } else {
    ImGui::Spacing();
    ImGui::Spacing();
    float width = ImGui::GetContentRegionAvail().x;
    float textWidth = ImGui::CalcTextSize("No entity selected").x;
    ImGui::SetCursorPosX((width - textWidth) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.46f, 0.48f, 1.0f));
    ImGui::Text("No entity selected");
    ImGui::PopStyleColor();
  }

  ImGui::End();
}

void PropertiesPanel::DrawComponents(Entity entity) {
  auto &world = m_Scene->GetWorld();

  // Tag Component
  if (world.HasComponent<TagComponent>(entity)) {
    auto &tag = world.GetComponent<TagComponent>(entity);

    char buffer[256];
    std::strncpy(buffer, tag.tag.c_str(), sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    if (ImGui::InputText("##Tag", buffer, sizeof(buffer))) {
      tag.tag = buffer;
    }
  }

  ImGui::SameLine();
  ImGui::PushItemWidth(-1);

  if (ImGui::Button("Add Component"))
    ImGui::OpenPopup("AddComponentPopup");

  ImGui::PopItemWidth();

  // Transform Component
  if (world.HasComponent<TransformComponent>(entity)) {
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap;

    bool open = ImGui::TreeNodeEx(
        (void *)typeid(TransformComponent).hash_code(), flags, "Transform");

    if (open) {
      auto &transform = world.GetComponent<TransformComponent>(entity);

      DrawVec3Control("Position", transform.position);

      Vec3 rotationDegrees = transform.rotation;
      DrawVec3Control("Rotation", rotationDegrees);
      transform.rotation = rotationDegrees;

      DrawVec3Control("Scale", transform.scale, 1.0f);

      ImGui::TreePop();
    }
  }

  // Sprite Component
  if (world.HasComponent<SpriteComponent>(entity)) {
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap;

    bool removeComponent = false;
    bool open = ImGui::TreeNodeEx((void *)typeid(SpriteComponent).hash_code(),
                                  flags, "Sprite");

    ImGui::SameLine(ImGui::GetWindowWidth() - 25);
    if (ImGui::Button("X", ImVec2(20, 20))) {
      removeComponent = true;
    }

    if (open) {
      auto &sprite = world.GetComponent<SpriteComponent>(entity);

      ImGui::ColorEdit4("Color", &sprite.color.r);
      ImGui::DragInt("Z Order", &sprite.zOrder);

      // Texture selection would go here
      ImGui::Text("Texture: %s", sprite.texture ? "Loaded" : "None");

      ImGui::TreePop();
    }

    if (removeComponent) {
      world.RemoveComponent<SpriteComponent>(entity);
    }
  }

  // MeshComponent
  if (world.HasComponent<MeshComponent>(entity)) {
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap;

    bool removeMesh = false;
    bool open = ImGui::TreeNodeEx((void *)typeid(MeshComponent).hash_code(),
                                  flags, "Mesh");
    ImGui::SameLine(ImGui::GetWindowWidth() - 25);
    if (ImGui::Button("X##mesh", ImVec2(20, 20))) {
      removeMesh = true;
    }

    if (open) {
      auto &mc = world.GetComponent<MeshComponent>(entity);

      const char *meshTypes[] = {"None", "Cube", "Sphere", "Plane", "Cylinder", "Custom"};
      int currentType = static_cast<int>(mc.meshType);
      if (ImGui::Combo("Mesh Type", &currentType, meshTypes, 6)) {
        mc.meshType = static_cast<MeshType>(currentType);
      }

      if (mc.meshType == MeshType::Custom) {
        char pathBuf[512];
        std::strncpy(pathBuf, mc.modelPath.c_str(), sizeof(pathBuf));
        pathBuf[sizeof(pathBuf) - 1] = '\0';
        if (ImGui::InputText("Model Path", pathBuf, sizeof(pathBuf))) {
          mc.modelPath = pathBuf;
        }
        if (ImGui::BeginDragDropTarget()) {
          if (const ImGuiPayload *payload =
                  ImGui::AcceptDragDropPayload(AssetBrowserPanel::PAYLOAD_MESH)) {
            mc.modelPath = std::string(static_cast<const char *>(payload->Data));
          }
          if (const ImGuiPayload *payload =
                  ImGui::AcceptDragDropPayload(AssetBrowserPanel::PAYLOAD_ASSET)) {
            mc.modelPath = std::string(static_cast<const char *>(payload->Data));
          }
          ImGui::EndDragDropTarget();
        }
      }

      ImGui::Checkbox("Cast Shadows", &mc.castShadows);
      ImGui::Checkbox("Receive Shadows", &mc.receiveShadows);

      ImGui::TreePop();
    }

    if (removeMesh) {
      world.RemoveComponent<MeshComponent>(entity);
    }
  }

  // MaterialComponent
  if (world.HasComponent<MaterialComponent>(entity)) {
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap;

    bool removeMat = false;
    bool open = ImGui::TreeNodeEx((void *)typeid(MaterialComponent).hash_code(),
                                  flags, "Material");
    ImGui::SameLine(ImGui::GetWindowWidth() - 25);
    if (ImGui::Button("X##mat", ImVec2(20, 20))) {
      removeMat = true;
    }

    if (open) {
      auto &mat = world.GetComponent<MaterialComponent>(entity);

      ImGui::ColorEdit3("Albedo", glm::value_ptr(mat.albedo));
      ImGui::SliderFloat("Metallic", &mat.metallic, 0.0f, 1.0f);
      ImGui::SliderFloat("Roughness", &mat.roughness, 0.0f, 1.0f);
      ImGui::SliderFloat("AO", &mat.ao, 0.0f, 1.0f);
      ImGui::ColorEdit3("Emissive", glm::value_ptr(mat.emissive));

      ImGui::Separator();
      ImGui::Text("Textures");

      auto drawTexturePath = [](const char *label, std::string &path) {
        char buf[512];
        std::strncpy(buf, path.c_str(), sizeof(buf));
        buf[sizeof(buf) - 1] = '\0';
        if (ImGui::InputText(label, buf, sizeof(buf))) {
          path = buf;
        }
        if (ImGui::BeginDragDropTarget()) {
          if (const ImGuiPayload *payload =
                  ImGui::AcceptDragDropPayload(AssetBrowserPanel::PAYLOAD_TEXTURE)) {
            path = std::string(static_cast<const char *>(payload->Data));
          }
          ImGui::EndDragDropTarget();
        }
      };

      drawTexturePath("Albedo Map", mat.albedoTexturePath);
      drawTexturePath("Normal Map", mat.normalTexturePath);
      drawTexturePath("Metallic Map", mat.metallicTexturePath);
      drawTexturePath("Roughness Map", mat.roughnessTexturePath);
      drawTexturePath("AO Map", mat.aoTexturePath);

      ImGui::TreePop();
    }

    if (removeMat) {
      world.RemoveComponent<MaterialComponent>(entity);
    }
  }

  // LightComponent
  if (world.HasComponent<LightComponent>(entity)) {
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap;

    bool removeLight = false;
    bool open = ImGui::TreeNodeEx((void *)typeid(LightComponent).hash_code(),
                                  flags, "Light");
    ImGui::SameLine(ImGui::GetWindowWidth() - 25);
    if (ImGui::Button("X##light", ImVec2(20, 20))) {
      removeLight = true;
    }

    if (open) {
      auto &lc = world.GetComponent<LightComponent>(entity);

      const char *lightTypes[] = {"Directional", "Point", "Spot"};
      ImGui::Combo("Type", &lc.type, lightTypes, 3);
      ImGui::ColorEdit3("Color", glm::value_ptr(lc.color));
      ImGui::DragFloat("Intensity", &lc.intensity, 0.1f, 0.0f, 100.0f);

      if (lc.type == 1 || lc.type == 2) {
        ImGui::DragFloat("Range", &lc.range, 0.5f, 0.0f, 1000.0f);
      }
      if (lc.type == 2) {
        ImGui::SliderFloat("Inner Cone", &lc.innerConeAngle, 0.0f, 90.0f);
        ImGui::SliderFloat("Outer Cone", &lc.outerConeAngle, 0.0f, 90.0f);
      }

      ImGui::Checkbox("Cast Shadows", &lc.castShadows);

      ImGui::TreePop();
    }

    if (removeLight) {
      world.RemoveComponent<LightComponent>(entity);
    }
  }

  // CameraComponent
  if (world.HasComponent<CameraComponent>(entity)) {
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap;

    bool removeCam = false;
    bool open = ImGui::TreeNodeEx((void *)typeid(CameraComponent).hash_code(),
                                  flags, "Camera");
    ImGui::SameLine(ImGui::GetWindowWidth() - 25);
    if (ImGui::Button("X##cam", ImVec2(20, 20))) {
      removeCam = true;
    }

    if (open) {
      auto &cc = world.GetComponent<CameraComponent>(entity);

      ImGui::Checkbox("Primary", &cc.isPrimary);
      ImGui::SliderFloat("FOV", &cc.fov, 1.0f, 120.0f);
      ImGui::DragFloat("Near Clip", &cc.nearClip, 0.01f, 0.001f, 10.0f);
      ImGui::DragFloat("Far Clip", &cc.farClip, 1.0f, 1.0f, 100000.0f);

      ImGui::TreePop();
    }

    if (removeCam) {
      world.RemoveComponent<CameraComponent>(entity);
    }
  }
}

void PropertiesPanel::DrawVec3Control(const std::string &label, Vec3 &values,
                                      f32 resetValue, f32 columnWidth) {
  ImGuiIO &io = ImGui::GetIO();
  auto boldFont = io.Fonts->Fonts[0];

  ImGui::PushID(label.c_str());

  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, columnWidth);
  ImGui::Text("%s", label.c_str());
  ImGui::NextColumn();

  ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

  f32 lineHeight = ImGui::GetFontSize() + GImGui->Style.FramePadding.y * 2.0f;
  ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

  // X (red - semantic for X axis)
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.4f, 0.4f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
  if (ImGui::Button("X", buttonSize))
    values.x = resetValue;
  ImGui::PopStyleColor(3);

  ImGui::SameLine();
  ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
  ImGui::PopItemWidth();
  ImGui::SameLine();

  // Y (green - semantic for Y axis)
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
  if (ImGui::Button("Y", buttonSize))
    values.y = resetValue;
  ImGui::PopStyleColor(3);

  ImGui::SameLine();
  ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
  ImGui::PopItemWidth();
  ImGui::SameLine();

  // Z (blue - semantic for Z axis, using primary blue)
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImVec4(0.39f, 0.68f, 1.00f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                        ImVec4(0.20f, 0.50f, 0.85f, 1.0f));
  if (ImGui::Button("Z", buttonSize))
    values.z = resetValue;
  ImGui::PopStyleColor(3);

  ImGui::SameLine();
  ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
  ImGui::PopItemWidth();

  ImGui::PopStyleVar();
  ImGui::Columns(1);

  ImGui::PopID();
}

} // namespace Gini
