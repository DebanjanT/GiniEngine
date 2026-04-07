#pragma once

#include "EditorPanel.h"
#include "Core/ThreadProfiler.h"

namespace Gini {

class ThreadAnalysisPanel : public EditorPanel {
public:
  ThreadAnalysisPanel();
  ~ThreadAnalysisPanel() = default;

  void OnImGuiRender() override;
  void OnUpdate(f32 deltaTime) override;

private:
  void DrawFrameTimeline();
  void DrawThreadList();
  void DrawTimingBreakdown();
  void DrawScopedTimers();

  static constexpr u32 HISTORY_SIZE = 240;

  f32 m_FrameTimeHistory[HISTORY_SIZE] = {};
  f32 m_RenderTimeHistory[HISTORY_SIZE] = {};
  f32 m_UpdateTimeHistory[HISTORY_SIZE] = {};
  u32 m_HistoryIndex = 0;

  f32 m_RefreshTimer = 0.0f;
  f32 m_RefreshInterval = 0.1f;

  std::vector<ThreadStats> m_CachedStats;
  std::vector<ScopedTimerResult> m_CachedTimers;

  f64 m_CachedFrameTime = 0.0;
  f64 m_CachedRenderTime = 0.0;
  f64 m_CachedUpdateTime = 0.0;
};

} // namespace Gini
