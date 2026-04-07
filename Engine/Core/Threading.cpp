#include "Threading.h"
#include "Logger.h"
#include "ThreadProfiler.h"
#include <chrono>

namespace Gini {

WorkerThread::~WorkerThread() { Stop(); }

bool WorkerThread::Start(const std::string &name) {
  if (m_Running.load()) {
    return true;
  }

  m_Name = name;
  m_Running.store(true);
  m_Thread = std::thread([this]() {
    ThreadProfiler::Get().RegisterThread(m_Name, std::this_thread::get_id());
    ThreadMain();
    ThreadProfiler::Get().UnregisterThread(m_Name);
  });
  GINI_INFO("Started worker thread: ", m_Name);
  return true;
}

void WorkerThread::Stop() {
  if (!m_Running.load()) {
    return;
  }

  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Running.store(false);
  }
  m_CV.notify_all();

  if (m_Thread.joinable()) {
    m_Thread.join();
  }

  GINI_INFO("Stopped worker thread: ", m_Name);
}

void WorkerThread::Submit(std::function<void()> task) {
  {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Tasks.push(std::move(task));
  }
  m_CV.notify_one();
}

void WorkerThread::ThreadMain() {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(m_Mutex);
      m_CV.wait(lock,
                [this]() { return !m_Running.load() || !m_Tasks.empty(); });

      if (!m_Running.load() && m_Tasks.empty()) {
        break;
      }

      task = std::move(m_Tasks.front());
      m_Tasks.pop();
    }

    if (task) {
      auto start = std::chrono::steady_clock::now();
      task();
      auto end = std::chrono::steady_clock::now();
      f64 ms = std::chrono::duration<f64, std::milli>(end - start).count();
      ThreadProfiler::Get().RecordTaskCompleted(m_Name, ms);
    }

    {
      std::lock_guard<std::mutex> lock(m_Mutex);
      ThreadProfiler::Get().SetQueueDepth(
          m_Name, static_cast<u32>(m_Tasks.size()));
    }
  }
}

bool RenderThread::Start() { return m_Worker.Start("RenderThread"); }

void RenderThread::Stop() { m_Worker.Stop(); }

void RenderThread::Submit(std::function<void()> task) {
  m_Worker.Submit(std::move(task));
}

} // namespace Gini
