#pragma once
#include <cstdint>

class CpuMonitor {
public:
    CpuMonitor();
    float getUsage(); // 返回 0.0 ~ 1.0
private:
#ifdef _WIN32
    uint64_t prev_idle = 0, prev_kernel = 0, prev_user = 0;
#else
    uint64_t prev_total = 0, prev_idle = 0;
#endif
};
