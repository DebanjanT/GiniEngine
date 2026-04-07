#include "ThreadAnalysisPanel.h"
#include "Core/Logger.h"
#include <imgui.h>
#include <sstream>
#include <thread>

namespace Gini {

ThreadAnalysisPanel::ThreadAnalysisPanel() : EditorPanel("Thread Analysis") {}

void ThreadAnalysisPanel::OnUpdate(f32 deltaTime) {
  m_RefreshTimer += deltaTime;
  if (m_RefreshTimer < m_RefreshInterval) {
    return;
  }
  m_RefreshTimer = 0.0f;

  auto &profiler = ThreadProfiler::Get();
  m_CachedStats = profiler.GetAllThreadStats();
  m_CachedTimers = profiler.GetRecentTimers();
  m_CachedFrameTime = profiler.GetFrameTimeMs();
  m_CachedRenderTime = profiler.GetRenderTimeMs();
  m_CachedUpdateTime = profiler.GetUpdateTimeMs();

  m_FrameTimeHistory[m_HistoryIndex] = static_cast<f32>(m_CachedFrameTime);
  m_RenderTimeHistory[m_HistoryIndex] = static_cast<f32>(m_CachedRenderTime);
  m_UpdateTimeHistory[m_HistoryIndex] = static_cast<f32>(m_CachedUpdateTime);
  m_HistoryIndex = (m_HistoryIndex + 1) % HISTORY_SIZE;
}

void ThreadAnalysisPanel::OnImGuiRender() {
  if (!m_Visible) {
    return;
  }

  ImGui::Begin(m_Name.c_str(), &m_Visible);

  DrawFrameTimeline();
  ImGui::Separator();
  DrawTimingBreakdown();
  ImGui::Separator();
  DrawThreadList();
  ImGui::Separator();
  DrawScopedTimers();

  ImGui::End();
}

void ThreadAnalysisPanel::DrawFrameTimeline() {
  if (!ImGui::CollapsingHeader("Frame Timeline",
                               ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }

  f32 fps =
      (m_CachedFrameTime > 0.0) ? static_cast<f32>(1000.0 / m_CachedFrameTime) : 0.0f;

  ImVec4 fpsColor;
  if (fps >= 55.0f) {
    fpsColor = ImVec4(0.2f, 0.9f, 0.2f, 1.0f);
  } else if (fps >= 30.0f) {
    fpsColor = ImVec4(0.9f, 0.9f, 0.2f, 1.0f);
  } else {
    fpsColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
  }

  ImGui::TextColored(fpsColor, "FPS: %.1f", fps);
  ImGui::SameLine(150);
  ImGui::Text("Frame: %.2f ms", m_CachedFrameTime);
  ImGui::SameLine(320);
  ImGui::Text("Update: %.2f ms", m_CachedUpdateTime);
  ImGui::SameLine(490);
  ImGui::Text("Render: %.2f ms", m_CachedRenderTime);

  ImGui::PlotLines("##FrameTime", m_FrameTimeHistory, HISTORY_SIZE,
                   m_HistoryIndex, "Frame Time (ms)", 0.0f, 33.3f,
                   ImVec2(0, 60));

  ImGui::PlotLines("##RenderTime", m_RenderTimeHistory, HISTORY_SIZE,
                   m_HistoryIndex, "Render (ms)", 0.0f, 33.3f,
                   ImVec2(0, 40));

  ImGui::PlotLines("##UpdateTime", m_UpdateTimeHistory, HISTORY_SIZE,
                   m_HistoryIndex, "Update (ms)", 0.0f, 33.3f,
                   ImVec2(0, 40));
}

void ThreadAnalysisPanel::DrawTimingBreakdown() {
  if (!ImGui::CollapsingHeader("CPU / GPU Breakdown",
                               ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }

  f32 total = static_cast<f32>(m_CachedFrameTime);
  f32 update = static_cast<f32>(m_CachedUpdateTime);
  f32 render = static_cast<f32>(m_CachedRenderTime);
  f32 other = total - update - render;
  if (other < 0.0f) {
    other = 0.0f;
  }

  if (total > 0.0f) {
    f32 updateFrac = update / total;
    f32 renderFrac = render / total;
    f32 otherFrac = other / total;

    ImGui::Text("Budget breakdown (%.1f ms total):", total);

    ImVec4 updateColor(0.3f, 0.6f, 1.0f, 1.0f);
    ImVec4 renderColor(0.3f, 0.9f, 0.3f, 1.0f);
    ImVec4 otherColor(0.6f, 0.6f, 0.6f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                          ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
    ImGui::ProgressBar(updateFrac, ImVec2(-1, 0),
                       ("Update: " + std::to_string(static_cast<int>(update * 100.0f / total)) + "%").c_str());
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                          ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
    ImGui::ProgressBar(renderFrac, ImVec2(-1, 0),
                       ("Render: " + std::to_string(static_cast<int>(render * 100.0f / total)) + "%").c_str());
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                          ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    ImGui::ProgressBar(otherFrac, ImVec2(-1, 0),
                       ("Other: " + std::to_string(static_cast<int>(other * 100.0f / total)) + "%").c_str());
    ImGui::PopStyleColor();
  } else {
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                       "Waiting for frame data...");
  }
}

void ThreadAnalysisPanel::DrawThreadList() {
  if (!ImGui::CollapsingHeader("Active Threads",
                               ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }

  ImGui::Columns(7, "ThreadColumns", true);
  ImGui::SetColumnWidth(0, 140);
  ImGui::SetColumnWidth(1, 60);
  ImGui::SetColumnWidth(2, 80);
  ImGui::SetColumnWidth(3, 80);
  ImGui::SetColumnWidth(4, 90);
  ImGui::SetColumnWidth(5, 90);
  ImGui::SetColumnWidth(6, 90);

  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Thread");
  ImGui::NextColumn();
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Status");
  ImGui::NextColumn();
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Queue");
  ImGui::NextColumn();
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Tasks");
  ImGui::NextColumn();
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Last (ms)");
  ImGui::NextColumn();
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Avg (ms)");
  ImGui::NextColumn();
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Peak (ms)");
  ImGui::NextColumn();
  ImGui::Separator();

  for (const auto &stats : m_CachedStats) {
    ImGui::Text("%s", stats.name.c_str());
    ImGui::NextColumn();

    if (stats.alive) {
      ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "ALIVE");
    } else {
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "IDLE");
    }
    ImGui::NextColumn();

    if (stats.queueDepth > 0) {
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%u",
                         stats.queueDepth);
    } else {
      ImGui::Text("0");
    }
    ImGui::NextColumn();

    ImGui::Text("%s", std::to_string(stats.tasksCompleted).c_str());
    ImGui::NextColumn();

    ImGui::Text("%.2f", stats.lastTaskDurationMs);
    ImGui::NextColumn();

    ImGui::Text("%.2f", stats.avgTaskDurationMs);
    ImGui::NextColumn();

    ImGui::Text("%.2f", stats.peakTaskDurationMs);
    ImGui::NextColumn();
  }

  if (m_CachedStats.empty()) {
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No threads registered");
    ImGui::NextColumn();
    for (int i = 0; i < 6; i++) {
      ImGui::NextColumn();
    }
  }

  ImGui::Columns(1);

  u32 hwThreads = std::thread::hardware_concurrency();
  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                     "Hardware threads: %u | Active engine threads: %zu",
                     hwThreads, m_CachedStats.size());
}

void ThreadAnalysisPanel::DrawScopedTimers() {
  if (!ImGui::CollapsingHeader("Recent Scoped Timers")) {
    return;
  }

  ImGui::Columns(3, "TimerColumns", true);
  ImGui::SetColumnWidth(0, 200);
  ImGui::SetColumnWidth(1, 100);
  ImGui::SetColumnWidth(2, 150);

  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Label");
  ImGui::NextColumn();
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Duration (ms)");
  ImGui::NextColumn();
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Thread ID");
  ImGui::NextColumn();
  ImGui::Separator();

  u32 shown = 0;
  for (auto it = m_CachedTimers.rbegin();
       it != m_CachedTimers.rend() && shown < 20; ++it, ++shown) {
    ImGui::Text("%s", it->label.c_str());
    ImGui::NextColumn();

    ImVec4 color;
    if (it->durationMs < 1.0) {
      color = ImVec4(0.2f, 0.9f, 0.2f, 1.0f);
    } else if (it->durationMs < 5.0) {
      color = ImVec4(0.9f, 0.9f, 0.2f, 1.0f);
    } else {
      color = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
    }
    ImGui::TextColored(color, "%.3f", it->durationMs);
    ImGui::NextColumn();

    std::ostringstream oss;
    oss << it->threadId;
    ImGui::Text("%s", oss.str().c_str());
    ImGui::NextColumn();
  }

  if (m_CachedTimers.empty()) {
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                       "No scoped timers recorded yet");
    ImGui::NextColumn();
    ImGui::NextColumn();
    ImGui::NextColumn();
  }

  ImGui::Columns(1);
  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                     "Use GINI_PROFILE_SCOPE(\"name\") to instrument code");
}

} // namespace Gini
