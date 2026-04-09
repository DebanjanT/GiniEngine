#include "Profiler.h"
#include "../Renderer/RendererAdapter.h"
#include "Logger.h"
#include <algorithm>
#include <fstream>
#include <iomanip>

namespace Gini {

Profiler &Profiler::GetInstance() {
  static Profiler instance;
  return instance;
}

Profiler::Profiler() {
  m_FrameStartTime = std::chrono::high_resolution_clock::now();
}

void Profiler::BeginScope(const std::string &name) {
  if (!m_Enabled)
    return;

  std::lock_guard<std::mutex> lock(m_Mutex);
  m_ScopeStartTimes[name] = std::chrono::high_resolution_clock::now();
}

void Profiler::EndScope(const std::string &name) {
  if (!m_Enabled)
    return;

  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_ScopeStartTimes.find(name);
  if (it != m_ScopeStartTimes.end()) {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration<f64, std::milli>(endTime - it->second).count();

    ScopeData &data = m_ScopeData[name];
    data.TotalTime += duration;
    data.CallCount++;
    data.MinTime = std::min(data.MinTime, duration);
    data.MaxTime = std::max(data.MaxTime, duration);

    m_ScopeStartTimes.erase(it);
  }
}

const Profiler::ScopeData &
Profiler::GetScopeData(const std::string &name) const {
  static const ScopeData emptyData;
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_ScopeData.find(name);
  return (it != m_ScopeData.end()) ? it->second : emptyData;
}

std::vector<std::string> Profiler::GetScopeNames() const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  std::vector<std::string> names;
  names.reserve(m_ScopeData.size());
  for (const auto &pair : m_ScopeData) {
    names.push_back(pair.first);
  }
  return names;
}

void Profiler::BeginFrame() {
  if (!m_Enabled)
    return;
  m_FrameStartTime = std::chrono::high_resolution_clock::now();
}

void Profiler::EndFrame() {
  if (!m_Enabled)
    return;

  auto endTime = std::chrono::high_resolution_clock::now();
  m_FrameTime =
      std::chrono::duration<f64, std::milli>(endTime - m_FrameStartTime)
          .count();

  m_FrameCount++;
  m_TotalFrameTime += m_FrameTime;

  // Update FPS every second
  if (m_TotalFrameTime >= 1000.0) {
    m_FPS = (m_FrameCount / m_TotalFrameTime) * 1000.0;
    m_FrameCount = 0;
    m_TotalFrameTime = 0.0;
  }
}

void Profiler::BeginGPUTimer(const std::string &name) {
  // TODO: Implement GPU timing when Diligent Engine is available
  // For now, this is a placeholder
}

void Profiler::EndGPUTimer(const std::string &name) {
  // TODO: Implement GPU timing when Diligent Engine is available
  // For now, this is a placeholder
}

f64 Profiler::GetGPUTime(const std::string &name) const {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_GPUTimers.find(name);
  return (it != m_GPUTimers.end()) ? it->second : 0.0;
}

void Profiler::TrackMemoryAllocation(u64 size) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_CurrentMemoryUsage += size;
  m_PeakMemoryUsage = std::max(m_PeakMemoryUsage, m_CurrentMemoryUsage);
}

void Profiler::TrackMemoryDeallocation(u64 size) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_CurrentMemoryUsage = std::max(m_CurrentMemoryUsage, size) - size;
}

void Profiler::Reset() {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_ScopeData.clear();
  m_ScopeStartTimes.clear();
  m_GPUTimers.clear();
  m_FrameCount = 0;
  m_TotalFrameTime = 0.0;
  m_CurrentMemoryUsage = 0;
  m_PeakMemoryUsage = 0;
}

void Profiler::PrintStatistics() const {
  std::lock_guard<std::mutex> lock(m_Mutex);

  GINI_INFO("=== Performance Statistics ===");
  GINI_INFO("FPS: {:.2f}", m_FPS);
  GINI_INFO("Frame Time: {:.2f} ms", m_FrameTime);
  GINI_INFO("Current Memory Usage: {} MB",
            m_CurrentMemoryUsage / (1024 * 1024));
  GINI_INFO("Peak Memory Usage: {} MB", m_PeakMemoryUsage / (1024 * 1024));
  GINI_INFO("=== Scope Timings ===");

  // Sort scopes by total time
  std::vector<std::pair<std::string, ScopeData>> sortedScopes(
      m_ScopeData.begin(), m_ScopeData.end());
  std::sort(sortedScopes.begin(), sortedScopes.end(),
            [](const auto &a, const auto &b) {
              return a.second.TotalTime > b.second.TotalTime;
            });

  for (const auto &pair : sortedScopes) {
    const auto &data = pair.second;
    GINI_INFO("{}: Total: {:.2f} ms, Calls: {}, Avg: {:.2f} ms, Min: {:.2f} "
              "ms, Max: {:.2f} ms",
              pair.first, data.TotalTime, data.CallCount, data.AverageTime(),
              data.MinTime, data.MaxTime);
  }
}

void Profiler::ExportStatistics(const std::string &filename) const {
  std::lock_guard<std::mutex> lock(m_Mutex);

  std::ofstream file(filename);
  if (!file.is_open()) {
    GINI_ERROR("Failed to open file for statistics export: {}", filename);
    return;
  }

  file << "=== Performance Statistics ===\n";
  file << "FPS: " << std::fixed << std::setprecision(2) << m_FPS << "\n";
  file << "Frame Time: " << std::fixed << std::setprecision(2) << m_FrameTime
       << " ms\n";
  file << "Current Memory Usage: " << (m_CurrentMemoryUsage / (1024 * 1024))
       << " MB\n";
  file << "Peak Memory Usage: " << (m_PeakMemoryUsage / (1024 * 1024))
       << " MB\n";
  file << "=== Scope Timings ===\n";

  // Sort scopes by total time
  std::vector<std::pair<std::string, ScopeData>> sortedScopes(
      m_ScopeData.begin(), m_ScopeData.end());
  std::sort(sortedScopes.begin(), sortedScopes.end(),
            [](const auto &a, const auto &b) {
              return a.second.TotalTime > b.second.TotalTime;
            });

  for (const auto &pair : sortedScopes) {
    const auto &data = pair.second;
    file << pair.first << ": Total: " << std::fixed << std::setprecision(2)
         << data.TotalTime << " ms, Calls: " << data.CallCount
         << ", Avg: " << data.AverageTime() << " ms, Min: " << data.MinTime
         << " ms, Max: " << data.MaxTime << " ms\n";
  }

  file.close();
  GINI_INFO("Statistics exported to: {}", filename);
}

// ScopeTimer implementation
ScopeTimer::ScopeTimer(const std::string &name) : m_Name(name) {
  Profiler::GetInstance().BeginScope(m_Name);
}

ScopeTimer::~ScopeTimer() { Profiler::GetInstance().EndScope(m_Name); }

// GPUTimer implementation
GPUTimer::GPUTimer(const std::string &name) : m_Name(name) {
  Profiler::GetInstance().BeginGPUTimer(m_Name);
}

GPUTimer::~GPUTimer() { Profiler::GetInstance().EndGPUTimer(m_Name); }

// PerformanceBenchmark implementation
void PerformanceBenchmark::RunBenchmark(const std::string &name,
                                        std::function<void()> benchmark,
                                        u32 iterations) {
  GINI_INFO("=== Running Benchmark: {} ===", name);

  // Warm-up run
  benchmark();

  // Timed runs
  auto &profiler = Profiler::GetInstance();
  profiler.BeginScope(name);

  auto startTime = std::chrono::high_resolution_clock::now();
  for (u32 i = 0; i < iterations; i++) {
    benchmark();
  }
  auto endTime = std::chrono::high_resolution_clock::now();

  profiler.EndScope(name);

  auto duration =
      std::chrono::duration<f64, std::milli>(endTime - startTime).count();
  f64 averageTime = duration / iterations;

  GINI_INFO("Benchmark {}: Total: {:.2f} ms, Iterations: {}, Avg: {:.2f} ms",
            name, duration, iterations, averageTime);
}

void PerformanceBenchmark::CompareRenderers(
    const std::string &testName, std::function<void()> openglTest,
    std::function<void()> diligentTest) {
  GINI_INFO("=== Comparing Renderers: {} ===", testName);

  // OpenGL test
  GINI_INFO("Running OpenGL test...");
  auto openglStart = std::chrono::high_resolution_clock::now();
  openglTest();
  auto openglEnd = std::chrono::high_resolution_clock::now();
  auto openglTime =
      std::chrono::duration<f64, std::milli>(openglEnd - openglStart).count();

  GINI_INFO("OpenGL Time: {:.2f} ms", openglTime);

  // Diligent Engine test (if available)
  if (RendererAdapter::GetInstance().IsDiligentEngineAvailable()) {
    GINI_INFO("Running Diligent Engine test...");
    auto diligentStart = std::chrono::high_resolution_clock::now();
    diligentTest();
    auto diligentEnd = std::chrono::high_resolution_clock::now();
    auto diligentTime =
        std::chrono::duration<f64, std::milli>(diligentEnd - diligentStart)
            .count();

    GINI_INFO("Diligent Engine Time: {:.2f} ms", diligentTime);
    GINI_INFO("Speedup: {:.2f}x", openglTime / diligentTime);
  } else {
    GINI_INFO("Diligent Engine not available for comparison");
  }
}

void PerformanceBenchmark::ProfileRenderingPipeline(
    const std::string &scenePath) {
  GINI_INFO("=== Profiling Rendering Pipeline: {} ===", scenePath);

  // This would load and render a scene while profiling each stage
  // For now, this is a placeholder for future implementation
  GINI_INFO("Scene profiling deferred until scene loading is implemented");
}

void PerformanceBenchmark::ProfileMemoryUsage() {
  auto &profiler = Profiler::GetInstance();
  GINI_INFO("=== Memory Usage Profile ===");
  GINI_INFO("Current Memory Usage: {} MB",
            profiler.GetCurrentMemoryUsage() / (1024 * 1024));
  GINI_INFO("Peak Memory Usage: {} MB",
            profiler.GetPeakMemoryUsage() / (1024 * 1024));
}

void PerformanceBenchmark::DetectMemoryLeaks() {
  // This would check for memory leaks using allocation tracking
  // For now, this is a placeholder for future implementation
  GINI_INFO("Memory leak detection deferred until allocation tracking is "
            "implemented");
}

void PerformanceBenchmark::MeasureDrawCallOverhead(u32 drawCallCount) {
  GINI_INFO("=== Measuring Draw Call Overhead ===");
  GINI_INFO("Draw Call Count: {}", drawCallCount);

  // This would measure the overhead of many draw calls
  // For now, this is a placeholder for future implementation
  GINI_INFO(
      "Draw call overhead measurement deferred until rendering is implemented");
}

void PerformanceBenchmark::MeasureTextureUploadPerformance(u32 textureCount) {
  GINI_INFO("=== Measuring Texture Upload Performance ===");
  GINI_INFO("Texture Count: {}", textureCount);

  // This would measure texture upload performance
  // For now, this is a placeholder for future implementation
  GINI_INFO("Texture upload performance measurement deferred until texture "
            "system is implemented");
}

void PerformanceBenchmark::MeasureShaderCompilationPerformance(
    const std::vector<std::string> &shaderPaths) {
  GINI_INFO("=== Measuring Shader Compilation Performance ===");
  GINI_INFO("Shader Count: {}", shaderPaths.size());

  // This would measure shader compilation performance
  // For now, this is a placeholder for future implementation
  GINI_INFO("Shader compilation performance measurement deferred until shader "
            "system is implemented");
}

// PerformanceOptimizer implementation
std::vector<PerformanceOptimizer::OptimizationSuggestion>
PerformanceOptimizer::AnalyzePerformance() {
  std::vector<OptimizationSuggestion> suggestions;

  // Analyze render state changes
  AnalyzeRenderStateChanges();

  // Analyze draw calls
  AnalyzeDrawCalls();

  // Analyze texture usage
  AnalyzeTextureUsage();

  // Analyze shader complexity
  AnalyzeShaderComplexity();

  // Analyze memory usage
  AnalyzeMemoryUsage();

  return suggestions;
}

void PerformanceOptimizer::ApplyOptimizations() {
  GINI_INFO("=== Applying Performance Optimizations ===");
  // This would apply detected optimizations
  GINI_INFO("Optimization application deferred until analysis is implemented");
}

void PerformanceOptimizer::GenerateReport(const std::string &filename) {
  GINI_INFO("=== Generating Performance Optimization Report ===");

  auto suggestions = AnalyzePerformance();

  std::ofstream file(filename);
  if (!file.is_open()) {
    GINI_ERROR("Failed to open file for optimization report: {}", filename);
    return;
  }

  file << "=== Performance Optimization Report ===\n";
  for (const auto &suggestion : suggestions) {
    file << "Category: " << suggestion.Category << "\n";
    file << "Priority: " << suggestion.Priority << "\n";
    file << "Description: " << suggestion.Description << "\n";
    file << "Estimated Improvement: " << suggestion.EstimatedImprovement
         << "%\n";
    file << "---\n";
  }

  file.close();
  GINI_INFO("Optimization report generated: {}", filename);
}

void PerformanceOptimizer::AnalyzeRenderStateChanges() {
  GINI_INFO("Analyzing render state changes...");
  // This would analyze render state changes and suggest batching
}

void PerformanceOptimizer::AnalyzeDrawCalls() {
  GINI_INFO("Analyzing draw calls...");
  // This would analyze draw call patterns and suggest instancing
}

void PerformanceOptimizer::AnalyzeTextureUsage() {
  GINI_INFO("Analyzing texture usage...");
  // This would analyze texture usage and suggest atlasing
}

void PerformanceOptimizer::AnalyzeShaderComplexity() {
  GINI_INFO("Analyzing shader complexity...");
  // This would analyze shader complexity and suggest simplifications
}

void PerformanceOptimizer::AnalyzeMemoryUsage() {
  GINI_INFO("Analyzing memory usage...");
  // This would analyze memory usage patterns and suggest optimizations
}

} // namespace Gini
