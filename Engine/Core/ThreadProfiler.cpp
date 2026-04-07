#include "ThreadProfiler.h"

namespace Gini {

void ThreadProfiler::RegisterThread(const std::string &name,
                                    std::thread::id id) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto &stats = m_Threads[name];
  stats.name = name;
  stats.threadId = id;
  stats.alive = true;
}

void ThreadProfiler::UnregisterThread(const std::string &name) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Threads.find(name);
  if (it != m_Threads.end()) {
    it->second.alive = false;
  }
}

void ThreadProfiler::SetThreadAlive(const std::string &name, bool alive) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Threads.find(name);
  if (it != m_Threads.end()) {
    it->second.alive = alive;
  }
}

void ThreadProfiler::SetQueueDepth(const std::string &name, u32 depth) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Threads.find(name);
  if (it != m_Threads.end()) {
    it->second.queueDepth = depth;
  }
}

void ThreadProfiler::RecordTaskCompleted(const std::string &name,
                                         f64 durationMs) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Threads.find(name);
  if (it != m_Threads.end()) {
    auto &s = it->second;
    s.tasksCompleted++;
    s.lastTaskDurationMs = durationMs;
    s.totalBusyMs += durationMs;
    if (durationMs > s.peakTaskDurationMs) {
      s.peakTaskDurationMs = durationMs;
    }
    if (s.tasksCompleted > 0) {
      s.avgTaskDurationMs = s.totalBusyMs / static_cast<f64>(s.tasksCompleted);
    }
  }
}

void ThreadProfiler::RecordScopedTimer(const std::string &label, f64 durationMs,
                                       std::thread::id tid) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (m_RecentTimers.size() >= TIMER_HISTORY) {
    m_RecentTimers.erase(m_RecentTimers.begin());
  }
  m_RecentTimers.push_back({label, durationMs, tid});
}

void ThreadProfiler::BeginFrame() {
  auto now = std::chrono::steady_clock::now();
  if (m_LastFrameStart.time_since_epoch().count() > 0) {
    m_FrameTimeMs =
        std::chrono::duration<f64, std::milli>(now - m_LastFrameStart).count();
  }
  m_LastFrameStart = now;
}

std::vector<ThreadStats> ThreadProfiler::GetAllThreadStats() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  std::vector<ThreadStats> result;
  result.reserve(m_Threads.size());
  for (const auto &[name, stats] : m_Threads) {
    result.push_back(stats);
  }
  return result;
}

std::vector<ScopedTimerResult> ThreadProfiler::GetRecentTimers() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  return m_RecentTimers;
}

} // namespace Gini
