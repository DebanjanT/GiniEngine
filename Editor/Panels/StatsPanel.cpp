#include "StatsPanel.h"
#include <imgui.h>

namespace Gini {

RenderStats StatsPanel::s_Stats;

StatsPanel::StatsPanel() : EditorPanel("Stats") {}

void StatsPanel::OnUpdate(f32 deltaTime) {
    // Update frame time history
    m_FrameTimeHistory[m_FrameHistoryIndex] = deltaTime * 1000.0f;
    m_FrameHistoryIndex = (m_FrameHistoryIndex + 1) % FRAME_HISTORY_SIZE;
    
    // Update FPS display every 0.5 seconds
    m_FpsUpdateTimer += deltaTime;
    if (m_FpsUpdateTimer >= 0.5f) {
        m_DisplayedFps = 1.0f / deltaTime;
        m_DisplayedFrameTime = deltaTime * 1000.0f;
        m_FpsUpdateTimer = 0.0f;
    }
}

void StatsPanel::OnImGuiRender() {
    if (!m_Visible) return;
    
    ImGui::Begin(m_Name.c_str(), &m_Visible);
    
    // Performance Section
    if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("FPS: %.1f", m_DisplayedFps);
        ImGui::Text("Frame Time: %.3f ms", m_DisplayedFrameTime);
        
        // Frame time graph
        ImGui::PlotLines("##FrameTime", m_FrameTimeHistory, FRAME_HISTORY_SIZE, 
                         m_FrameHistoryIndex, "Frame Time (ms)", 
                         0.0f, 33.3f, ImVec2(0, 80));
        
        // Target frame time lines
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "60 FPS = 16.67ms");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "30 FPS = 33.33ms");
    }
    
    ImGui::Separator();
    
    // Renderer Section
    if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Draw Calls: %u", m_RenderStats.drawCalls);
        ImGui::Text("Triangles: %u", m_RenderStats.triangles);
        ImGui::Text("Vertices: %u", m_RenderStats.vertices);
        ImGui::Text("Texture Binds: %u", m_RenderStats.textureBinds);
        ImGui::Text("Shader Switches: %u", m_RenderStats.shaderSwitches);
        
        if (m_RenderStats.gpuTime > 0.0f) {
            ImGui::Text("GPU Time: %.3f ms", m_RenderStats.gpuTime);
        }
    }
    
    ImGui::Separator();
    
    // Scene Section
    if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Entity Count: %u", m_EntityCount);
    }
    
    ImGui::Separator();
    
    // Memory Section
    if (ImGui::CollapsingHeader("Memory")) {
        ImGui::Text("Allocated: -- MB");
        ImGui::Text("GPU Memory: -- MB");
        ImGui::Text("Textures: -- MB");
    }
    
    ImGui::End();
}

} // namespace Gini
