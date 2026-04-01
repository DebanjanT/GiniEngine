#pragma once

#include "EditorPanel.h"

namespace Gini {

struct RenderStats {
    u32 drawCalls = 0;
    u32 triangles = 0;
    u32 vertices = 0;
    u32 textureBinds = 0;
    u32 shaderSwitches = 0;
    f32 frameTime = 0.0f;
    f32 fps = 0.0f;
    f32 gpuTime = 0.0f;
    
    void Reset() {
        drawCalls = 0;
        triangles = 0;
        vertices = 0;
        textureBinds = 0;
        shaderSwitches = 0;
    }
};

class StatsPanel : public EditorPanel {
public:
    StatsPanel();
    
    void OnImGuiRender() override;
    void OnUpdate(f32 deltaTime) override;
    
    void SetEntityCount(u32 count) { m_EntityCount = count; }
    void SetRenderStats(const RenderStats& stats) { m_RenderStats = stats; }
    
    static RenderStats& GetStats() { return s_Stats; }
    
private:
    RenderStats m_RenderStats;
    u32 m_EntityCount = 0;
    
    // Frame time history for graph
    static constexpr u32 FRAME_HISTORY_SIZE = 120;
    f32 m_FrameTimeHistory[FRAME_HISTORY_SIZE] = {};
    u32 m_FrameHistoryIndex = 0;
    
    f32 m_FpsUpdateTimer = 0.0f;
    f32 m_DisplayedFps = 0.0f;
    f32 m_DisplayedFrameTime = 0.0f;
    
    static RenderStats s_Stats;
};

} // namespace Gini
