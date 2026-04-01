#include "ConsolePanel.h"
#include <imgui.h>
#include <ctime>

namespace Gini {

ConsolePanel* ConsolePanel::s_Instance = nullptr;

ConsolePanel::ConsolePanel() : EditorPanel("Console") {
    s_Instance = this;
}

ConsolePanel& ConsolePanel::Get() {
    return *s_Instance;
}

void ConsolePanel::AddLog(LogMessage::Level level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    LogMessage msg;
    msg.level = level;
    msg.message = message;
    
    // Get timestamp
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    msg.timestamp = buffer;
    
    m_Messages.push_back(msg);
    
    // Limit message count
    if (m_Messages.size() > 1000) {
        m_Messages.erase(m_Messages.begin());
    }
}

void ConsolePanel::Clear() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Messages.clear();
}

void ConsolePanel::OnImGuiRender() {
    if (!m_Visible) return;
    
    ImGui::Begin(m_Name.c_str(), &m_Visible);
    
    // Toolbar
    if (ImGui::Button("Clear")) {
        Clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
    ImGui::SameLine();
    
    // Filter buttons
    ImGui::PushStyleColor(ImGuiCol_Button, m_ShowTrace ? ImVec4(0.3f, 0.3f, 0.3f, 1.0f) : ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    if (ImGui::Button("Trace")) m_ShowTrace = !m_ShowTrace;
    ImGui::PopStyleColor();
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, m_ShowDebug ? ImVec4(0.2f, 0.5f, 0.2f, 1.0f) : ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    if (ImGui::Button("Debug")) m_ShowDebug = !m_ShowDebug;
    ImGui::PopStyleColor();
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, m_ShowInfo ? ImVec4(0.2f, 0.4f, 0.8f, 1.0f) : ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    if (ImGui::Button("Info")) m_ShowInfo = !m_ShowInfo;
    ImGui::PopStyleColor();
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, m_ShowWarn ? ImVec4(0.8f, 0.6f, 0.2f, 1.0f) : ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    if (ImGui::Button("Warn")) m_ShowWarn = !m_ShowWarn;
    ImGui::PopStyleColor();
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, m_ShowError ? ImVec4(0.8f, 0.2f, 0.2f, 1.0f) : ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    if (ImGui::Button("Error")) m_ShowError = !m_ShowError;
    ImGui::PopStyleColor();
    
    ImGui::Separator();
    
    // Filter input
    ImGui::InputText("Filter", m_FilterBuffer, sizeof(m_FilterBuffer));
    
    ImGui::Separator();
    
    // Log messages
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    for (const auto& msg : m_Messages) {
        // Filter by level
        bool show = false;
        ImVec4 color;
        
        switch (msg.level) {
            case LogMessage::Level::Trace:
                show = m_ShowTrace;
                color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                break;
            case LogMessage::Level::Debug:
                show = m_ShowDebug;
                color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
                break;
            case LogMessage::Level::Info:
                show = m_ShowInfo;
                color = ImVec4(0.4f, 0.6f, 1.0f, 1.0f);
                break;
            case LogMessage::Level::Warn:
                show = m_ShowWarn;
                color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
                break;
            case LogMessage::Level::Error:
                show = m_ShowError;
                color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                break;
        }
        
        if (!show) continue;
        
        // Filter by text
        if (m_FilterBuffer[0] != '\0') {
            if (msg.message.find(m_FilterBuffer) == std::string::npos) {
                continue;
            }
        }
        
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(("[" + msg.timestamp + "] " + msg.message).c_str());
        ImGui::PopStyleColor();
    }
    
    if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    ImGui::End();
}

} // namespace Gini
