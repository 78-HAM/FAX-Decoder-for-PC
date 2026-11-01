#include "util/CpuMonitor.h"
#include <cstdio>

#ifdef _WIN32
#include <windows.h>

CpuMonitor::CpuMonitor() {}

float CpuMonitor::getUsage() {
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return 0.0f;

    auto ft2ull = [](const FILETIME& ft) -> uint64_t {
        return ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    };

    uint64_t idle = ft2ull(idleTime);
    uint64_t kernel = ft2ull(kernelTime);
    uint64_t user = ft2ull(userTime);

    if (prev_idle == 0) {
        prev_idle = idle; prev_kernel = kernel; prev_user = user;
        return 0.0f;
    }

    uint64_t idle_diff = idle - prev_idle;
    uint64_t kernel_diff = kernel - prev_kernel;
    uint64_t user_diff = user - prev_user;

    uint64_t total_diff = kernel_diff + user_diff;
    prev_idle = idle; prev_kernel = kernel; prev_user = user;

    if (total_diff == 0) return 0.0f;
    return 1.0f - (float)idle_diff / (float)total_diff;
}

#else
#include <fstream>
#include <string>

CpuMonitor::CpuMonitor() {}

float CpuMonitor::getUsage() {
    std::ifstream file("/proc/stat");
    std::string line;
    if (!std::getline(file, line)) return 0.0f;
    
    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
    if (std::sscanf(line.c_str(), "cpu  %lu %lu %lu %lu %lu %lu %lu %lu", 
        &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) < 4) return 0.0f;
    
    uint64_t total = user + nice + system + idle + iowait + irq + softirq + steal;
    if (prev_total == 0) {
        prev_total = total; prev_idle = idle;
        return 0.0f;
    }
    
    uint64_t total_diff = total - prev_total;
    uint64_t idle_diff = idle - prev_idle;
    prev_total = total; prev_idle = idle;
    
    if (total_diff == 0) return 0.0f;
    return 1.0f - (float)idle_diff / (float)total_diff;
}
#endif
