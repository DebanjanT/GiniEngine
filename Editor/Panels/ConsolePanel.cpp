#include "ConsolePanel.h"
#include <ctime>
#include <imgui.h>

namespace Gini {

ConsolePanel *ConsolePanel::s_Instance = nullptr;

ConsolePanel::ConsolePanel() : EditorPanel("Console") { s_Instance = this; }

ConsolePanel &ConsolePanel::Get() { return *s_Instance; }

void ConsolePanel::AddLog(LogMessage::Level level, const std::string &message) {
  std::lock_guard<std::mutex> lock(m_Mutex);

  LogMessage msg;
  msg.level = level;
  msg.message = message;

  // Get timestamp
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);
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
  if (!m_Visible)
    return;

  ImGui::Begin(m_Name.c_str(), &m_Visible);

  // Toolbar row
  ImVec4 offColor(0.0f, 0.0f, 0.0f, 0.0f);
  ImVec4 offHover(0.24f, 0.24f, 0.27f, 1.0f);

  auto FilterToggle = [&](const char* label, bool& flag, ImVec4 onColor) {
    ImGui::PushStyleColor(ImGuiCol_Button, flag ? onColor : offColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, flag ? onColor : offHover);
    ImGui::PushStyleColor(ImGuiCol_Text, flag ? ImVec4(1,1,1,1) : ImVec4(0.55f,0.55f,0.58f,1));
    if (ImGui::SmallButton(label)) flag = !flag;
    ImGui::PopStyleColor(3);
    ImGui::SameLine(0, 4);
  };

  if (ImGui::SmallButton("Clear")) Clear();
  ImGui::SameLine(0, 12);
  ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
  ImGui::SameLine(0, 16);

  FilterToggle("TRC", m_ShowTrace,  ImVec4(0.35f, 0.35f, 0.40f, 1.0f));
  FilterToggle("DBG", m_ShowDebug,  ImVec4(0.25f, 0.50f, 0.30f, 1.0f));
  FilterToggle("INF", m_ShowInfo,   ImVec4(0.18f, 0.45f, 0.62f, 1.0f));
  FilterToggle("WRN", m_ShowWarn,   ImVec4(0.65f, 0.50f, 0.18f, 1.0f));
  FilterToggle("ERR", m_ShowError,  ImVec4(0.65f, 0.22f, 0.22f, 1.0f));

  ImGui::SameLine(0, 12);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  ImGui::InputTextWithHint("##filter", "Filter...", m_FilterBuffer, sizeof(m_FilterBuffer));
  ImGui::PopStyleVar();

  ImGui::Separator();

  // Log messages
  ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false,
                    ImGuiWindowFlags_HorizontalScrollbar);

  std::lock_guard<std::mutex> lock(m_Mutex);

  for (const auto &msg : m_Messages) {
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

    if (!show)
      continue;

    // Filter by text
    if (m_FilterBuffer[0] != '\0') {
      if (msg.message.find(m_FilterBuffer) == std::string::npos) {
        continue;
      }
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.42f, 0.42f, 0.45f, 1.0f));
    ImGui::TextUnformatted(msg.timestamp.c_str());
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(msg.message.c_str());
    ImGui::PopStyleColor();
  }

  if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
    ImGui::SetScrollHereY(1.0f);
  }

  ImGui::EndChild();
  ImGui::End();
}

} // namespace Gini
