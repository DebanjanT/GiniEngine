#pragma once

#include "Core/Types.h"
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>

namespace Gini {

struct LoadingTask {
  std::string name;
  std::string description;
  f32 progress = 0.0f;  // 0.0 to 1.0
  bool indeterminate = false;  // If true, shows spinning indicator instead of progress bar
  u64 id = 0;
};

class LoadingIndicator {
public:
  static LoadingIndicator& Get() {
    static LoadingIndicator instance;
    return instance;
  }
  
  // Start a new loading task, returns task ID
  u64 BeginTask(const std::string& name, const std::string& description = "", bool indeterminate = false);
  
  // Update task progress (0.0 to 1.0)
  void UpdateProgress(u64 taskId, f32 progress);
  
  // Update task description
  void UpdateDescription(u64 taskId, const std::string& description);
  
  // End a loading task
  void EndTask(u64 taskId);
  
  // Check if any tasks are active
  bool HasActiveTasks() const;
  
  // Get all active tasks (for rendering)
  std::vector<LoadingTask> GetActiveTasks() const;
  
  // Render the loading overlay (call from ImGui render)
  void RenderOverlay();
  
  // Helper for scoped tasks
  class ScopedTask {
  public:
    ScopedTask(const std::string& name, const std::string& description = "", bool indeterminate = false)
      : m_TaskId(LoadingIndicator::Get().BeginTask(name, description, indeterminate)) {}
    ~ScopedTask() { LoadingIndicator::Get().EndTask(m_TaskId); }
    
    void UpdateProgress(f32 progress) { LoadingIndicator::Get().UpdateProgress(m_TaskId, progress); }
    void UpdateDescription(const std::string& desc) { LoadingIndicator::Get().UpdateDescription(m_TaskId, desc); }
    u64 GetId() const { return m_TaskId; }
    
  private:
    u64 m_TaskId;
  };

private:
  LoadingIndicator() = default;
  ~LoadingIndicator() = default;
  LoadingIndicator(const LoadingIndicator&) = delete;
  LoadingIndicator& operator=(const LoadingIndicator&) = delete;
  
  mutable std::mutex m_Mutex;
  std::vector<LoadingTask> m_Tasks;
  std::atomic<u64> m_NextTaskId{1};
};

// Convenience macro for scoped loading tasks
#define GINI_LOADING_TASK(name, desc) \
  Gini::LoadingIndicator::ScopedTask _loadingTask##__LINE__(name, desc)

#define GINI_LOADING_TASK_INDETERMINATE(name, desc) \
  Gini::LoadingIndicator::ScopedTask _loadingTask##__LINE__(name, desc, true)

} // namespace Gini
