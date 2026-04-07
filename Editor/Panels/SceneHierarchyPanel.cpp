#include "SceneHierarchyPanel.h"
#include "ECS/Components.h"
#include <cstring>
#include <imgui.h>

namespace Gini {

SceneHierarchyPanel::SceneHierarchyPanel() : EditorPanel("Scene Hierarchy") {}

void SceneHierarchyPanel::OnImGuiRender() {
    if (!m_Visible) return;
    
    ImGui::Begin(m_Name.c_str(), &m_Visible);
    
    if (m_Scene) {
        // Search filter
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##search", "Search entities...", m_SearchBuffer, sizeof(m_SearchBuffer));
        ImGui::PopStyleVar(2);
        ImGui::Spacing();

        // Entity count
        auto roots = m_Scene->GetRootEntities();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.56f, 0.58f, 1.0f));
        ImGui::Text("%d entities", (int)roots.size());
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        // Draw root entities
        for (Entity entity : roots) {
            if (strlen(m_SearchBuffer) > 0 && !MatchesFilter(entity))
                continue;
            DrawEntityNode(entity);
        }
        
        // Right-click context menu on empty space
        if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
            ImGui::TextDisabled("Create");
            ImGui::Separator();
            if (ImGui::MenuItem("Empty Entity")) {
                m_Scene->CreateEntity("Empty Entity");
            }
            if (ImGui::MenuItem("Camera")) {
                m_Scene->CreateEntity("Camera");
            }
            if (ImGui::MenuItem("Light")) {
                m_Scene->CreateEntity("Light");
            }
            ImGui::EndPopup();
        }
    } else {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.56f, 0.58f, 1.0f));
        ImGui::TextWrapped("No scene loaded. Create or open a scene from File menu.");
        ImGui::PopStyleColor();
    }
    
    // Deselect when clicking empty space
    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
        m_SelectedEntity = NullEntity;
    }
    
    ImGui::End();
}

bool SceneHierarchyPanel::MatchesFilter(Entity entity) {
    if (!m_Scene) return false;
    auto& world = m_Scene->GetWorld();
    if (!world.IsValid(entity)) return false;
    
    std::string name = "Entity";
    if (world.HasComponent<TagComponent>(entity))
        name = world.GetComponent<TagComponent>(entity).tag;
    
    // Case-insensitive substring match
    std::string lowerName = name;
    std::string lowerFilter(m_SearchBuffer);
    for (auto& c : lowerName) c = static_cast<char>(tolower(c));
    for (auto& c : lowerFilter) c = static_cast<char>(tolower(c));
    
    if (lowerName.find(lowerFilter) != std::string::npos)
        return true;
    
    // Check children recursively
    for (Entity child : m_Scene->GetChildren(entity)) {
        if (MatchesFilter(child)) return true;
    }
    return false;
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
