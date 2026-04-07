#pragma once

#include "Types.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Gini {

struct ThreadStats {
  std::string name;
  std::thread::id threadId;
  bool alive = false;
  u64 tasksCompleted = 0;
  u32 queueDepth = 0;
  f64 lastTaskDurationMs = 0.0;
  f64 avgTaskDurationMs = 0.0;
  f64 peakTaskDurationMs = 0.0;
  f64 totalBusyMs = 0.0;
  f64 idleFraction = 1.0;
};

struct ScopedTimerResult {
  std::string label;
  f64 durationMs = 0.0;
  std::thread::id threadId;
};

class ThreadProfiler {
public:
  static ThreadProfiler &Get() {
    static ThreadProfiler instance;
    return instance;
  }

  void RegisterThread(const std::string &name, std::thread::id id);
  void UnregisterThread(const std::string &name);

  void SetThreadAlive(const std::string &name, bool alive);
  void SetQueueDepth(const std::string &name, u32 depth);
  void RecordTaskCompleted(const std::string &name, f64 durationMs);

  void RecordScopedTimer(const std::string &label, f64 durationMs,
                         std::thread::id tid);

  void BeginFrame();

  std::vector<ThreadStats> GetAllThreadStats() const;
  std::vector<ScopedTimerResult> GetRecentTimers() const;

  f64 GetFrameTimeMs() const { return m_FrameTimeMs; }
  f64 GetRenderTimeMs() const { return m_RenderTimeMs.load(); }
  f64 GetUpdateTimeMs() const { return m_UpdateTimeMs.load(); }

  void SetRenderTimeMs(f64 ms) { m_RenderTimeMs.store(ms); }
  void SetUpdateTimeMs(f64 ms) { m_UpdateTimeMs.store(ms); }

  static constexpr u32 TIMER_HISTORY = 128;

private:
  ThreadProfiler() = default;

  mutable std::mutex m_Mutex;
  std::unordered_map<std::string, ThreadStats> m_Threads;
  std::vector<ScopedTimerResult> m_RecentTimers;

  std::chrono::steady_clock::time_point m_LastFrameStart{};
  f64 m_FrameTimeMs = 0.0;
  std::atomic<f64> m_RenderTimeMs{0.0};
  std::atomic<f64> m_UpdateTimeMs{0.0};
};

class ScopedTimer {
public:
  ScopedTimer(const std::string &label)
      : m_Label(label), m_Start(std::chrono::steady_clock::now()),
        m_ThreadId(std::this_thread::get_id()) {}

  ~ScopedTimer() {
    auto end = std::chrono::steady_clock::now();
    f64 ms = std::chrono::duration<f64, std::milli>(end - m_Start).count();
    ThreadProfiler::Get().RecordScopedTimer(m_Label, ms, m_ThreadId);
  }

private:
  std::string m_Label;
  std::chrono::steady_clock::time_point m_Start;
  std::thread::id m_ThreadId;
};

#define GINI_PROFILE_SCOPE(name) ::Gini::ScopedTimer _timer_##__LINE__(name)

} // namespace Gini
