#include "LoadingIndicator.h"
#include "Core/Logger.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace Gini {

u64 LoadingIndicator::BeginTask(const std::string& name, const std::string& description, bool indeterminate) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  
  LoadingTask task;
  task.id = m_NextTaskId++;
  task.name = name;
  task.description = description;
  task.progress = 0.0f;
  task.indeterminate = indeterminate;
  
  m_Tasks.push_back(task);
  GINI_INFO("Loading task started: ", name);
  return task.id;
}

void LoadingIndicator::UpdateProgress(u64 taskId, f32 progress) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  
  for (auto& task : m_Tasks) {
    if (task.id == taskId) {
      task.progress = std::clamp(progress, 0.0f, 1.0f);
      break;
    }
  }
}

void LoadingIndicator::UpdateDescription(u64 taskId, const std::string& description) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  
  for (auto& task : m_Tasks) {
    if (task.id == taskId) {
      task.description = description;
      break;
    }
  }
}

void LoadingIndicator::EndTask(u64 taskId) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  
  auto it = std::find_if(m_Tasks.begin(), m_Tasks.end(), 
    [taskId](const LoadingTask& t) { return t.id == taskId; });
  
  if (it != m_Tasks.end()) {
    GINI_INFO("Loading task completed: ", it->name);
    m_Tasks.erase(it);
  }
}

bool LoadingIndicator::HasActiveTasks() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return !m_Tasks.empty();
}

std::vector<LoadingTask> LoadingIndicator::GetActiveTasks() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_Tasks;
}

void LoadingIndicator::RenderOverlay() {
  if (!HasActiveTasks()) return;
  
  auto tasks = GetActiveTasks();
  if (tasks.empty()) return;
  
  ImGuiIO& io = ImGui::GetIO();
  
  // Position in bottom-right corner
  const f32 padding = 16.0f;
  const f32 panelWidth = 320.0f;
  const f32 taskHeight = 60.0f;
  const f32 panelHeight = std::min((f32)tasks.size() * taskHeight + 40.0f, 300.0f);
  
  ImVec2 windowPos(io.DisplaySize.x - panelWidth - padding, 
                   io.DisplaySize.y - panelHeight - padding);
  
  ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
  
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing |
                           ImGuiWindowFlags_NoBringToFrontOnFocus;
  
  // Semi-transparent dark background
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.12f, 0.95f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.5f, 0.8f, 0.5f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
  
  if (ImGui::Begin("##LoadingOverlay", nullptr, flags)) {
    // Header
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Use default font
    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Loading...");
    ImGui::PopFont();
    
    ImGui::Separator();
    ImGui::Spacing();
    
    // Render each task
    static f32 spinnerAngle = 0.0f;
    spinnerAngle += io.DeltaTime * 4.0f;
    if (spinnerAngle > 6.28318f) spinnerAngle -= 6.28318f;
    
    for (const auto& task : tasks) {
      ImGui::PushID(static_cast<int>(task.id));
      
      // Task name
      ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%s", task.name.c_str());
      
      // Description if present
      if (!task.description.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", task.description.c_str());
      }
      
      // Progress bar or spinner
      if (task.indeterminate) {
        // Animated indeterminate progress bar
        f32 animProgress = (std::sin(spinnerAngle * 2.0f) + 1.0f) * 0.5f;
        f32 barStart = animProgress * 0.6f;
        f32 barEnd = barStart + 0.3f;
        
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size(ImGui::GetContentRegionAvail().x, 4.0f);
        
        // Background
        ImGui::GetWindowDrawList()->AddRectFilled(
          pos, ImVec2(pos.x + size.x, pos.y + size.y),
          IM_COL32(40, 40, 50, 255), 2.0f);
        
        // Animated bar
        f32 startX = pos.x + barStart * size.x;
        f32 endX = pos.x + std::min(barEnd, 1.0f) * size.x;
        ImGui::GetWindowDrawList()->AddRectFilled(
          ImVec2(startX, pos.y), ImVec2(endX, pos.y + size.y),
          IM_COL32(80, 160, 255, 255), 2.0f);
        
        ImGui::Dummy(ImVec2(size.x, size.y + 4.0f));
      } else {
        // Standard progress bar
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.2f, 1.0f));
        ImGui::ProgressBar(task.progress, ImVec2(-1.0f, 4.0f), "");
        ImGui::PopStyleColor(2);
        
        // Percentage text
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "%.0f%%", task.progress * 100.0f);
      }
      
      ImGui::Spacing();
      ImGui::PopID();
    }
  }
  ImGui::End();
  
  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

} // namespace Gini
