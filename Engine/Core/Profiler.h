#pragma once

#include "Types.h"
#include <string>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace Gini {

// Performance profiling system for renderer performance monitoring
class Profiler {
public:
    static Profiler& GetInstance();
    
    // Profiling scopes
    void BeginScope(const std::string& name);
    void EndScope(const std::string& name);
    
    // Performance metrics
    struct ScopeData {
        f64 TotalTime = 0.0;
        u64 CallCount = 0;
        f64 MinTime = 1e10;
        f64 MaxTime = 0.0;
        f64 AverageTime() const { return CallCount > 0 ? TotalTime / CallCount : 0.0; }
    };
    
    // Get performance data
    const ScopeData& GetScopeData(const std::string& name) const;
    std::vector<std::string> GetScopeNames() const;
    
    // Frame timing
    void BeginFrame();
    void EndFrame();
    f64 GetFrameTime() const { return m_FrameTime; }
    f64 GetFPS() const { return m_FPS; }
    
    // GPU timing (when available)
    void BeginGPUTimer(const std::string& name);
    void EndGPUTimer(const std::string& name);
    f64 GetGPUTime(const std::string& name) const;
    
    // Memory tracking
    void TrackMemoryAllocation(u64 size);
    void TrackMemoryDeallocation(u64 size);
    u64 GetCurrentMemoryUsage() const { return m_CurrentMemoryUsage; }
    u64 GetPeakMemoryUsage() const { return m_PeakMemoryUsage; }
    
    // Statistics
    void Reset();
    void PrintStatistics() const;
    void ExportStatistics(const std::string& filename) const;
    
    // Enable/disable profiling
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

private:
    Profiler();
    
    bool m_Enabled = true;
    std::unordered_map<std::string, ScopeData> m_ScopeData;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> m_ScopeStartTimes;
    
    // Frame timing
    std::chrono::high_resolution_clock::time_point m_FrameStartTime;
    f64 m_FrameTime = 0.0;
    f64 m_FPS = 0.0;
    u32 m_FrameCount = 0;
    f64 m_TotalFrameTime = 0.0;
    
    // GPU timing (placeholder for future implementation)
    std::unordered_map<std::string, f64> m_GPUTimers;
    
    // Memory tracking
    u64 m_CurrentMemoryUsage = 0;
    u64 m_PeakMemoryUsage = 0;
    
    // Thread safety
    mutable std::mutex m_Mutex;
};

// RAII scope timer for automatic profiling
class ScopeTimer {
public:
    ScopeTimer(const std::string& name);
    ~ScopeTimer();
    
private:
    std::string m_Name;
};

// RAII GPU timer for automatic GPU profiling
class GPUTimer {
public:
    GPUTimer(const std::string& name);
    ~GPUTimer();
    
private:
    std::string m_Name;
};

// Performance benchmarking utilities
class PerformanceBenchmark {
public:
    static void RunBenchmark(const std::string& name, std::function<void()> benchmark, u32 iterations = 100);
    static void CompareRenderers(const std::string& testName, std::function<void()> openglTest, std::function<void()> diligentTest);
    static void ProfileRenderingPipeline(const std::string& scenePath);
    
    // Memory profiling
    static void ProfileMemoryUsage();
    static void DetectMemoryLeaks();
    
    // Rendering performance
    static void MeasureDrawCallOverhead(u32 drawCallCount);
    static void MeasureTextureUploadPerformance(u32 textureCount);
    static void MeasureShaderCompilationPerformance(const std::vector<std::string>& shaderPaths);
};

// Performance optimization suggestions
class PerformanceOptimizer {
public:
    struct OptimizationSuggestion {
        std::string Category;
        std::string Description;
        std::string Priority; // "High", "Medium", "Low"
        f64 EstimatedImprovement; // Percentage
    };
    
    static std::vector<OptimizationSuggestion> AnalyzePerformance();
    static void ApplyOptimizations();
    static void GenerateReport(const std::string& filename);
    
private:
    static void AnalyzeRenderStateChanges();
    static void AnalyzeDrawCalls();
    static void AnalyzeTextureUsage();
    static void AnalyzeShaderComplexity();
    static void AnalyzeMemoryUsage();
};

} // namespace Gini
