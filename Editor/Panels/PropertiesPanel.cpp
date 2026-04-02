#include "PropertiesPanel.h"
#include "ECS/Components.h"
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

    ImGui::Separator();

    // Add Component button
    if (ImGui::Button("Add Component", ImVec2(-1, 0))) {
      ImGui::OpenPopup("AddComponent");
    }

    if (ImGui::BeginPopup("AddComponent")) {
      if (ImGui::MenuItem("Sprite")) {
        if (!m_Scene->GetWorld().HasComponent<SpriteComponent>(
                m_SelectedEntity)) {
          m_Scene->GetWorld().AddComponent<SpriteComponent>(m_SelectedEntity);
        }
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::MenuItem("Camera")) {
        // Add camera component
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::MenuItem("Light")) {
        // Add light component
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  } else {
    ImGui::TextDisabled("No entity selected");
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

  // X
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
  if (ImGui::Button("X", buttonSize))
    values.x = resetValue;
  ImGui::PopStyleColor(3);

  ImGui::SameLine();
  ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
  ImGui::PopItemWidth();
  ImGui::SameLine();

  // Y
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
  if (ImGui::Button("Y", buttonSize))
    values.y = resetValue;
  ImGui::PopStyleColor(3);

  ImGui::SameLine();
  ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
  ImGui::PopItemWidth();
  ImGui::SameLine();

  // Z
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImVec4(0.2f, 0.35f, 0.9f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
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
