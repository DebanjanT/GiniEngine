#pragma once

#include "Types.h"
#include <chrono>

namespace Gini {

class Timer {
public:
    Timer() { Reset(); }
    
    void Reset() {
        m_Start = std::chrono::high_resolution_clock::now();
    }
    
    f64 Elapsed() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<f64>(now - m_Start).count();
    }
    
    f64 ElapsedMillis() const {
        return Elapsed() * 1000.0;
    }
    
private:
    std::chrono::high_resolution_clock::time_point m_Start;
};

class DeltaTime {
public:
    DeltaTime() = default;
    
    void Update() {
        auto now = std::chrono::high_resolution_clock::now();
        m_DeltaTime = std::chrono::duration<f64>(now - m_LastFrame).count();
        m_LastFrame = now;
        m_TotalTime += m_DeltaTime;
        m_FrameCount++;
    }
    
    f64 Get() const { return m_DeltaTime; }
    f32 GetF() const { return static_cast<f32>(m_DeltaTime); }
    f64 GetTotal() const { return m_TotalTime; }
    u64 GetFrameCount() const { return m_FrameCount; }
    f64 GetFPS() const { return m_DeltaTime > 0 ? 1.0 / m_DeltaTime : 0.0; }
    
private:
    std::chrono::high_resolution_clock::time_point m_LastFrame = 
        std::chrono::high_resolution_clock::now();
    f64 m_DeltaTime = 0.0;
    f64 m_TotalTime = 0.0;
    u64 m_FrameCount = 0;
};

// Fixed timestep for deterministic simulation (critical for RTS)
class FixedTimestep {
public:
    FixedTimestep(f64 tickRate = 60.0) 
        : m_TickRate(tickRate), m_TickDuration(1.0 / tickRate) {}
    
    void Update(f64 deltaTime) {
        m_Accumulator += deltaTime;
    }
    
    bool ShouldTick() {
        if (m_Accumulator >= m_TickDuration) {
            m_Accumulator -= m_TickDuration;
            m_TickCount++;
            return true;
        }
        return false;
    }
    
    f64 GetTickDuration() const { return m_TickDuration; }
    f64 GetAlpha() const { return m_Accumulator / m_TickDuration; }
    u64 GetTickCount() const { return m_TickCount; }
    
    void SetTickRate(f64 rate) {
        m_TickRate = rate;
        m_TickDuration = 1.0 / rate;
    }
    
private:
    f64 m_TickRate;
    f64 m_TickDuration;
    f64 m_Accumulator = 0.0;
    u64 m_TickCount = 0;
};

} // namespace Gini
