#include "utils/logger.hpp"
#include <iostream>
#include <iomanip>
#include <ctime>

Logger::Logger() 
    : m_logLevel(LogLevel::INFO)
    , m_consoleOutput(true) {
}

Logger::~Logger() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    m_logFile.open(filename, std::ios::app);
    if (!m_logFile.is_open()) {
        throw std::runtime_error("Failed to open log file: " + filename);
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logLevel = level;
}

void Logger::writeLog(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string timestamp = getTimestamp();
    std::string levelStr = levelToString(level);
    std::string formattedMessage = 
        timestamp + " [" + levelStr + "] " + message + "\n";

    // Write to file if opened
    if (m_logFile.is_open()) {
        m_logFile << formattedMessage;
        m_logFile.flush();
    }

    // Write to console if enabled
    if (m_consoleOutput) {
        if (level >= LogLevel::WARNING) {
            std::cerr << formattedMessage;
        } else {
            std::cout << formattedMessage;
        }
    }
}

void Logger::logLatency(const std::string& operation, 
                       std::chrono::microseconds duration) {
    std::ostringstream ss;
    ss << "Latency - " << operation << ": " 
       << duration.count() << "μs";
    info(ss.str());
}

std::string Logger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto now_tm = std::localtime(&now_c);
    
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream ss;
    ss << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
    
    return ss.str();
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}