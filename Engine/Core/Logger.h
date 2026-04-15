#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <functional>

namespace Gini {

enum class LogLevel {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

// Callback function type for log messages
using LogCallback = std::function<void(LogLevel, const std::string&)>;

class Logger {
public:
    static Logger& Get() {
        static Logger instance;
        return instance;
    }
    
    void SetLevel(LogLevel level) { m_Level = level; }
    void SetOutputFile(const std::string& path);
    void EnableConsole(bool enable) { m_ConsoleEnabled = enable; }
    
    void SetLogCallback(LogCallback callback) { m_LogCallback = callback; }
    
    template<typename... Args>
    void Log(LogLevel level, const char* file, int line, Args&&... args) {
        if (level < m_Level) return;
        
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        std::ostringstream ss;
        ss << GetTimestamp() << " ";
        ss << "[" << LevelToString(level) << "] ";
        ss << "[" << ExtractFilename(file) << ":" << line << "] ";
        (ss << ... << std::forward<Args>(args));
        ss << "\n";
        
        std::string message = ss.str();
        
        if (m_ConsoleEnabled) {
            std::cout << GetColorCode(level) << message << "\033[0m";
        }
        
        if (m_FileStream.is_open()) {
            m_FileStream << message;
            m_FileStream.flush();
        }
        
        // Call registered callback (e.g., for ConsolePanel)
        if (m_LogCallback) {
            m_LogCallback(level, message);
        }
    }
    
private:
    Logger() = default;
    ~Logger() {
        if (m_FileStream.is_open()) {
            m_FileStream.close();
        }
    }
    
    std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::ostringstream ss;
        ss << std::put_time(std::localtime(&time), "%H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    const char* LevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Trace: return "TRACE";
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info:  return "INFO ";
            case LogLevel::Warn:  return "WARN ";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Fatal: return "FATAL";
            default: return "?????";
        }
    }
    
    const char* GetColorCode(LogLevel level) {
        switch (level) {
            case LogLevel::Trace: return "\033[90m";      // Gray
            case LogLevel::Debug: return "\033[36m";      // Cyan
            case LogLevel::Info:  return "\033[32m";      // Green
            case LogLevel::Warn:  return "\033[33m";      // Yellow
            case LogLevel::Error: return "\033[31m";      // Red
            case LogLevel::Fatal: return "\033[35m";      // Magenta
            default: return "\033[0m";
        }
    }
    
    std::string ExtractFilename(const char* path) {
        std::string p(path);
        size_t pos = p.find_last_of("/\\");
        return (pos != std::string::npos) ? p.substr(pos + 1) : p;
    }
    
    LogLevel m_Level = LogLevel::Trace;
    bool m_ConsoleEnabled = true;
    std::ofstream m_FileStream;
    std::mutex m_Mutex;
    LogCallback m_LogCallback;
};

// Logging macros
#define GINI_TRACE(...) ::Gini::Logger::Get().Log(::Gini::LogLevel::Trace, __FILE__, __LINE__, __VA_ARGS__)
#define GINI_DEBUG(...) ::Gini::Logger::Get().Log(::Gini::LogLevel::Debug, __FILE__, __LINE__, __VA_ARGS__)
#define GINI_INFO(...)  ::Gini::Logger::Get().Log(::Gini::LogLevel::Info,  __FILE__, __LINE__, __VA_ARGS__)
#define GINI_WARN(...)  ::Gini::Logger::Get().Log(::Gini::LogLevel::Warn,  __FILE__, __LINE__, __VA_ARGS__)
#define GINI_ERROR(...) ::Gini::Logger::Get().Log(::Gini::LogLevel::Error, __FILE__, __LINE__, __VA_ARGS__)
#define GINI_FATAL(...) ::Gini::Logger::Get().Log(::Gini::LogLevel::Fatal, __FILE__, __LINE__, __VA_ARGS__)

// Assertions
#define GINI_ASSERT(condition, ...) \
    do { \
        if (!(condition)) { \
            GINI_FATAL("Assertion failed: ", #condition, " - ", __VA_ARGS__); \
            std::abort(); \
        } \
    } while(0)

#ifdef NDEBUG
    #define GINI_DEBUG_ASSERT(condition, ...)
#else
    #define GINI_DEBUG_ASSERT(condition, ...) GINI_ASSERT(condition, __VA_ARGS__)
#endif

} // namespace Gini
