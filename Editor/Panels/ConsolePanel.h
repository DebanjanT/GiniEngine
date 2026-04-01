#pragma once

#include "EditorPanel.h"
#include <vector>
#include <string>
#include <mutex>

namespace Gini {

struct LogMessage {
    enum class Level { Trace, Debug, Info, Warn, Error };
    Level level;
    std::string message;
    std::string timestamp;
};

class ConsolePanel : public EditorPanel {
public:
    ConsolePanel();
    
    void OnImGuiRender() override;
    
    void AddLog(LogMessage::Level level, const std::string& message);
    void Clear();
    
    static ConsolePanel& Get();
    
private:
    std::vector<LogMessage> m_Messages;
    std::mutex m_Mutex;
    bool m_AutoScroll = true;
    bool m_ShowTrace = true;
    bool m_ShowDebug = true;
    bool m_ShowInfo = true;
    bool m_ShowWarn = true;
    bool m_ShowError = true;
    char m_FilterBuffer[256] = {};
    
    static ConsolePanel* s_Instance;
};

} // namespace Gini
