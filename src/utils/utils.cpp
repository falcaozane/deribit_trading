#include "utils/utils.hpp"
#include "utils/logger.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <thread>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <pthread.h>

#ifdef __linux__
    #include <sys/sysinfo.h>
    #include <sys/mman.h>
    #include <sys/syscall.h>
#elif defined(_WIN32)
    #include <windows.h>
    #include <psapi.h>
#endif

namespace utils {

// Time utilities
int64_t getCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string formatTimestamp(int64_t timestamp) {
    auto timePoint = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(timestamp)
    );
    auto timeT = std::chrono::system_clock::to_time_t(timePoint);
    auto tm = *std::localtime(&timeT);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    auto ms = timestamp % 1000;
    oss << '.' << std::setfill('0') << std::setw(3) << ms;
    
    return oss.str();
}

int64_t parseTimestamp(const std::string& timestamp) {
    std::tm tm = {};
    std::istringstream ss(timestamp);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    if (ss.fail()) {
        throw std::runtime_error("Failed to parse timestamp");
    }
    
    auto timePoint = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        timePoint.time_since_epoch()
    ).count();
}

// String utilities
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string trim(const std::string& str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) {
        start++;
    }
    
    auto end = str.end();
    do {
        end--;
    } while (end > start && std::isspace(*end));
    
    return std::string(start, end + 1);
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() &&
           str.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Number utilities
bool isValidPrice(double price) {
    return !std::isnan(price) && !std::isinf(price) && price > 0.0;
}

bool isValidQuantity(double quantity) {
    return !std::isnan(quantity) && !std::isinf(quantity) && quantity > 0.0;
}

double roundPrice(double price, int decimals) {
    double factor = std::pow(10.0, decimals);
    return std::round(price * factor) / factor;
}

double roundQuantity(double quantity, int decimals) {
    double factor = std::pow(10.0, decimals);
    return std::round(quantity * factor) / factor;
}

// ScopedTimer implementation
ScopedTimer::ScopedTimer(const std::string& operation)
    : m_operation(operation)
    , m_start(std::chrono::high_resolution_clock::now()) {
}

ScopedTimer::~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
    Logger::getInstance().logLatency(m_operation, duration);
}

// ThreadUtils implementation
void ThreadUtils::setThreadPriority(int priority) {
#ifdef __linux__
    struct sched_param param;
    param.sched_priority = priority;
    
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
        Logger::getInstance().warning("Failed to set thread priority");
    }
#elif defined(_WIN32)
    if (!SetThreadPriority(GetCurrentThread(), priority)) {
        Logger::getInstance().warning("Failed to set thread priority");
    }
#endif
}

void ThreadUtils::setThreadAffinity(int cpuId) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpuId, &cpuset);
    
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        Logger::getInstance().warning("Failed to set thread affinity");
    }
#elif defined(_WIN32)
    DWORD_PTR mask = 1ULL << cpuId;
    if (!SetThreadAffinityMask(GetCurrentThread(), mask)) {
        Logger::getInstance().warning("Failed to set thread affinity");
    }
#endif
}

void ThreadUtils::setThreadName(const std::string& name) {
#ifdef __linux__
    pthread_setname_np(pthread_self(), name.c_str());
#endif
}

int ThreadUtils::getCurrentCPU() {
#ifdef __linux__
    return sched_getcpu();
#else
    return -1;
#endif
}

// MemoryUtils implementation
size_t MemoryUtils::getProcessMemoryUsage() {
#ifdef __linux__
    FILE* fp = fopen("/proc/self/statm", "r");
    if (!fp) return 0;
    
    long rss = 0;
    if (fscanf(fp, "%*s%ld", &rss) != 1) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    
    long page_size = sysconf(_SC_PAGE_SIZE);
    return static_cast<size_t>(rss) * static_cast<size_t>(page_size);
#elif defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
#else
    return 0;
#endif
}

size_t MemoryUtils::getSystemMemoryUsage() {
#ifdef __linux__
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        return (size_t)si.totalram - (size_t)si.freeram;
    }
    return 0;
#elif defined(_WIN32)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    }
    return 0;
#else
    return 0;
#endif
}

void MemoryUtils::lockMemory(void* addr, size_t size) {
#ifdef __linux__
    if (mlock(addr, size) != 0) {
        Logger::getInstance().warning("Failed to lock memory");
    }
#elif defined(_WIN32)
    if (!VirtualLock(addr, size)) {
        Logger::getInstance().warning("Failed to lock memory");
    }
#endif
}

void MemoryUtils::unlockMemory(void* addr, size_t size) {
#ifdef __linux__
    if (munlock(addr, size) != 0) {
        Logger::getInstance().warning("Failed to unlock memory");
    }
#elif defined(_WIN32)
    if (!VirtualUnlock(addr, size)) {
        Logger::getInstance().warning("Failed to unlock memory");
    }
#endif
}

} // namespace utils