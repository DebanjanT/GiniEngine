#include "SceneHierarchyPanel.h"
#include "ECS/Components.h"
#include <imgui.h>

namespace Gini {

SceneHierarchyPanel::SceneHierarchyPanel() : EditorPanel("Scene Hierarchy") {}

void SceneHierarchyPanel::OnImGuiRender() {
    if (!m_Visible) return;
    
    ImGui::Begin(m_Name.c_str(), &m_Visible);
    
    if (m_Scene) {
        // Draw root entities
        auto roots = m_Scene->GetRootEntities();
        for (Entity entity : roots) {
            DrawEntityNode(entity);
        }
        
        // Right-click context menu on empty space
        if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
            if (ImGui::MenuItem("Create Empty Entity")) {
                m_Scene->CreateEntity("Empty Entity");
            }
            if (ImGui::MenuItem("Create Camera")) {
                auto entity = m_Scene->CreateEntity("Camera");
                // Add camera component when implemented
            }
            if (ImGui::MenuItem("Create Light")) {
                auto entity = m_Scene->CreateEntity("Light");
                // Add light component when implemented
            }
            ImGui::EndPopup();
        }
    } else {
        ImGui::TextDisabled("No scene loaded");
    }
    
    // Deselect when clicking empty space
    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
        m_SelectedEntity = NullEntity;
    }
    
    ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(Entity entity) {
    if (!m_Scene) return;
    
    auto& world = m_Scene->GetWorld();
    if (!world.IsValid(entity)) return;
    
    std::string name = "Entity";
    if (world.HasComponent<TagComponent>(entity)) {
        name = world.GetComponent<TagComponent>(entity).tag;
    }
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | 
                                ImGuiTreeNodeFlags_SpanAvailWidth |
                                ImGuiTreeNodeFlags_OpenOnDoubleClick;
    
    if (m_SelectedEntity == entity) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    
    // Check if has children
    auto children = m_Scene->GetChildren(entity);
    if (children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    
    bool opened = ImGui::TreeNodeEx((void*)(u64)entity, flags, "%s", name.c_str());
    
    // Selection
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        m_SelectedEntity = entity;
        if (m_SelectionCallback) {
            m_SelectionCallback(entity);
        }
    }
    
    // Context menu
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Create Child")) {
            auto child = m_Scene->CreateEntity("Child");
            m_Scene->SetParent(child, entity);
        }
        if (ImGui::MenuItem("Duplicate")) {
            m_Scene->DuplicateEntity(entity);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete")) {
            m_Scene->DestroyEntity(entity);
            if (m_SelectedEntity == entity) {
                m_SelectedEntity = NullEntity;
            }
        }
        ImGui::EndPopup();
    }
    
    // Drag and drop for reparenting
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("ENTITY", &entity, sizeof(Entity));
        ImGui::Text("%s", name.c_str());
        ImGui::EndDragDropSource();
    }
    
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY")) {
            Entity droppedEntity = *(Entity*)payload->Data;
            m_Scene->SetParent(droppedEntity, entity);
        }
        ImGui::EndDragDropTarget();
    }
    
    if (opened) {
        for (Entity child : children) {
            DrawEntityNode(child);
        }
        ImGui::TreePop();
    }
}

} // namespace Gini
