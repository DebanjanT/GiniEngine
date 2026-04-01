#include "Logger.h"

namespace Gini {

void Logger::SetOutputFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_FileStream.is_open()) {
        m_FileStream.close();
    }
    m_FileStream.open(path, std::ios::out | std::ios::app);
    if (!m_FileStream.is_open()) {
        std::cerr << "Failed to open log file: " << path << std::endl;
    }
}

} // namespace Gini
