#pragma once
#include <string>
#include <chrono>
#include <vector>
#include <optional>

namespace utils {

// Time utilities
int64_t getCurrentTimestamp();
std::string formatTimestamp(int64_t timestamp);
int64_t parseTimestamp(const std::string& timestamp);

// String utilities
std::vector<std::string> split(const std::string& str, char delimiter);
std::string trim(const std::string& str);
bool startsWith(const std::string& str, const std::string& prefix);
bool endsWith(const std::string& str, const std::string& suffix);

// Number utilities
bool isValidPrice(double price);
bool isValidQuantity(double quantity);
double roundPrice(double price, int decimals);
double roundQuantity(double quantity, int decimals);

// Performance measurement
class ScopedTimer {
public:
    ScopedTimer(const std::string& operation);
    ~ScopedTimer();

private:
    std::string m_operation;
    std::chrono::high_resolution_clock::time_point m_start;
};

// Thread utilities
class ThreadUtils {
public:
    static void setThreadPriority(int priority);
    static void setThreadAffinity(int cpuId);
    static void setThreadName(const std::string& name);
    static int getCurrentCPU();
};

// Memory utilities
class MemoryUtils {
public:
    static size_t getProcessMemoryUsage();
    static size_t getSystemMemoryUsage();
    static void lockMemory(void* addr, size_t size);
    static void unlockMemory(void* addr, size_t size);
};

} // namespace utils