#pragma once
#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <chrono>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    static Logger& getInstance();

    void setLogFile(const std::string& filename);
    void setLogLevel(LogLevel level);
    
    template<typename... Args>
    void debug(Args... args) {
        log(LogLevel::DEBUG, args...);
    }

    template<typename... Args>
    void info(Args... args) {
        log(LogLevel::INFO, args...);
    }

    template<typename... Args>
    void warning(Args... args) {
        log(LogLevel::WARNING, args...);
    }

    template<typename... Args>
    void error(Args... args) {
        log(LogLevel::ERROR, args...);
    }

    template<typename... Args>
    void critical(Args... args) {
        log(LogLevel::CRITICAL, args...);
    }

    // For latency measurements
    void logLatency(const std::string& operation, 
                   std::chrono::microseconds duration);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    template<typename T>
    void logOneArg(std::ostringstream& ss, T&& arg) {
        ss << std::forward<T>(arg);
    }

    template<typename T, typename... Args>
    void logOneArg(std::ostringstream& ss, T&& arg, Args&&... args) {
        ss << std::forward<T>(arg);
        logOneArg(ss, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void log(LogLevel level, Args&&... args) {
        if (level >= m_logLevel) {
            std::ostringstream ss;
            logOneArg(ss, std::forward<Args>(args)...);
            writeLog(level, ss.str());
        }
    }

    void writeLog(LogLevel level, const std::string& message);
    std::string getTimestamp() const;
    std::string levelToString(LogLevel level) const;

    LogLevel m_logLevel;
    std::ofstream m_logFile;
    std::mutex m_mutex;
    bool m_consoleOutput;
};