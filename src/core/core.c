#include "noty_core.h"

#include <windows.h>

const char* noty_core_version_string(void) {
    return "0.1.0";
}

uint32_t noty_core_logical_cpu_count(void) {
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    if (si.dwNumberOfProcessors == 0) {
        return 1;
    }
    return (uint32_t)si.dwNumberOfProcessors;
}

uint64_t noty_core_total_physical_memory(void) {
    MEMORYSTATUSEX ms;
    ZeroMemory(&ms, sizeof(ms));
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms)) {
        return 0;
    }
    return (uint64_t)ms.ullTotalPhys;
}

uint64_t noty_core_available_physical_memory(void) {
    MEMORYSTATUSEX ms;
    ZeroMemory(&ms, sizeof(ms));
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms)) {
        return 0;
    }
    return (uint64_t)ms.ullAvailPhys;
}