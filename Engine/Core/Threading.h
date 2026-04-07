#pragma once

#include "Types.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace Gini {

class WorkerThread {
public:
  WorkerThread() = default;
  ~WorkerThread();

  bool Start(const std::string &name);
  void Stop();

  void Submit(std::function<void()> task);

  template <typename Fn> auto SubmitAndWait(Fn &&fn) -> decltype(fn()) {
    using ReturnT = decltype(fn());
    auto task = std::make_shared<std::packaged_task<ReturnT()>>(
        std::forward<Fn>(fn));
    std::future<ReturnT> future = task->get_future();
    Submit([task]() { (*task)(); });
    return future.get();
  }

  bool IsRunning() const { return m_Running.load(); }

private:
  void ThreadMain();

  std::string m_Name;
  std::thread m_Thread;
  std::mutex m_Mutex;
  std::condition_variable m_CV;
  std::queue<std::function<void()>> m_Tasks;
  std::atomic<bool> m_Running{false};
};

class RenderThread {
public:
  bool Start();
  void Stop();

  void Submit(std::function<void()> task);

  template <typename Fn> auto SubmitAndWait(Fn &&fn) -> decltype(fn()) {
    return m_Worker.SubmitAndWait(std::forward<Fn>(fn));
  }

  bool IsRunning() const { return m_Worker.IsRunning(); }

private:
  WorkerThread m_Worker;
};

} // namespace Gini
