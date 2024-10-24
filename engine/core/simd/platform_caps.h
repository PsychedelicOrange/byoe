#ifndef PLATFORM_DEFS_H
#define PLATFORM_DEFS_H

#include "../logging/log.h"

#if defined(_MSC_VER)
    #include <intrin.h>
#elif defined(__clang__) || defined(__GNUC__)
    #if defined(__APPLE__)
        #include <sys/types.h>
    #else
        #include <x86intrin.h>
    #endif
#else
    #error "Unknown compiler"
#endif

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__linux__)
    #include <sys/sysinfo.h>
    #include <sys/utsname.h>
    #include <unistd.h>
#elif defined(__APPLE__)
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <mach/mach.h>
#endif

// Enum to represent different instruction sets
typedef enum SIMD {
    SIMD_NONE = 0,
    SIMD_SSE = 1 << 0,
    SIMD_SSE2 = 1 << 1,
    SIMD_SSE3 = 1 << 2,
    SIMD_SSSE3 = 1 << 3,
    SIMD_SSE4_1 = 1 << 4,
    SIMD_SSE4_2 = 1 << 5,
    SIMD_AVX = 1 << 6,
    SIMD_AVX2 = 1 << 7,
    SIMD_AVX512 = 1 << 8,
    SIMD_NEON = 1 << 9,   // ARM NEON support
    SIMD_ASIMD = 1 << 10  // ARMv8 ASIMD
} SIMD;

// Function to detect supported instruction sets
static inline int cpu_detect_instruction_set(void) {
    int supported = SIMD_NONE;

#if defined(_MSC_VER)
    // Windows - MSVC intrinsics
    int info[4];
    __cpuid(info, 0);  // Query for max CPUID
    if (info[0] >= 1) {
        __cpuid(info, 1);
        if (info[3] & (1 << 25)) supported |= SIMD_SSE;
        if (info[3] & (1 << 26)) supported |= SIMD_SSE2;
        if (info[2] & (1 << 0))  supported |= SIMD_SSE3;
        if (info[2] & (1 << 9))  supported |= SIMD_SSSE3;
        if (info[2] & (1 << 19)) supported |= SIMD_SSE4_1;
        if (info[2] & (1 << 20)) supported |= SIMD_SSE4_2;
        if (info[2] & (1 << 28)) supported |= SIMD_AVX;
    }
    __cpuid(info, 7);  // Extended features
    if (info[1] & (1 << 5))  supported |= SIMD_AVX2;
    if (info[1] & (1 << 16)) supported |= SIMD_AVX512;

#elif defined(__clang__) || defined(__GNUC__)
    // Linux/macOS/other platforms - GCC/Clang intrinsics
    #if defined(__x86_64__) || defined(__i386__)
        #if defined(__SSE__)
            supported |= SIMD_SSE;
        #endif
        #if defined(__SSE2__)
            supported |= SIMD_SSE2;
        #endif
        #if defined(__SSE3__)
            supported |= SIMD_SSE3;
        #endif
        #if defined(__SSSE3__)
            supported |= SIMD_SSSE3;
        #endif
        #if defined(__SSE4_1__)
            supported |= SIMD_SSE4_1;
        #endif
        #if defined(__SSE4_2__)
            supported |= SIMD_SSE4_2;
        #endif
        #if defined(__AVX__)
            supported |= SIMD_AVX;
        #endif
        #if defined(__AVX2__)
            supported |= SIMD_AVX2;
        #endif
        #if defined(__AVX512F__)
            supported |= SIMD_AVX512;
        #endif
    #elif defined(__aarch64__) || defined(__ARM_NEON__)
        // ARM NEON (common in mobile/Apple Silicon)
        supported |= SIMD_NEON;
        #if defined(__ARM_NEON__)
            supported |= SIMD_ASIMD;
        #endif
    #endif
#endif

    return supported;
}

void cpu_caps_print_info(void) {
    #if defined(_WIN32)
    // Windows-specific CPU info
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    LOG_INFO("Number of processors: %u\n", sysinfo.dwNumberOfProcessors);

    // Processor architecture
    if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
        LOG_INFO("Processor architecture: x64 (AMD or Intel)\n");
    } else if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM) {
        LOG_INFO("Processor architecture: ARM\n");
    } else if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        LOG_INFO("Processor architecture: x86 (Intel)\n");
    }

    // Processor name
    HKEY hKeyProcessor;
    char cpuName[256] = { 0 };
    DWORD bufferSize = sizeof(cpuName);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKeyProcessor) == ERROR_SUCCESS) {
        RegQueryValueExA(hKeyProcessor, "ProcessorNameString", NULL, NULL, (LPBYTE)cpuName, &bufferSize);
        RegCloseKey(hKeyProcessor);
    }
    LOG_INFO("Processor Name: %s\n", cpuName);

    #elif defined(__linux__)
    // Linux-specific CPU info
    FILE* cpuInfoFile = fopen("/proc/cpuinfo", "r");
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), cpuInfoFile)) {
        if (strncmp(buffer, "model name", 10) == 0) {
            LOG_INFO("CPU Model: %s", strchr(buffer, ':') + 2);
        }
        if (strncmp(buffer, "cpu MHz", 7) == 0) {
            LOG_INFO("CPU Frequency: %s", strchr(buffer, ':') + 2);
        }
    }
    fclose(cpuInfoFile);

    struct sysinfo sysInfo;
    sysinfo(&sysInfo);
    LOG_INFO("Uptime: %ld seconds\n", sysInfo.uptime);

    #elif defined(__APPLE__)
    // https://gist.github.com/HalCanary/59d7c9c3c408ddfd55c477a992281827
    // macOS-specific CPU info
    char cpuBrand[512];
    size_t size = sizeof(cpuBrand);
    sysctlbyname("machdep.cpu.brand_string", &cpuBrand, &size, NULL, 0);
    LOG_INFO("Processor: %s\n", cpuBrand);

    int frequency;
    size = sizeof(frequency);
    sysctlbyname("hw.cpufrequency", &frequency, &size, NULL, 0);
    LOG_INFO("CPU Frequency: %d MHz\n", frequency);

    int numCores;
    sysctlbyname("hw.physicalcpu", &numCores, &size, NULL, 0);
    LOG_INFO("Number of cores: %d\n", numCores);

    #endif
}

void os_caps_print_info(void) {
    #if defined(_WIN32)
    // Windows OS info
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    LOG_INFO("Operating System: Windows %lu.%lu\n", osvi.dwMajorVersion, osvi.dwMinorVersion);
    
    #elif defined(__linux__)
    // Linux OS info
    struct utsname buffer;
    if (uname(&buffer) == 0) {
        LOG_INFO("Operating System: %s %s\n", buffer.sysname, buffer.release);
    }

    #elif defined(__APPLE__)
    // macOS info
    char osVersion[256];
    size_t size = sizeof(osVersion);
    sysctlbyname("kern.osproductversion", &osVersion, &size, NULL, 0);
    LOG_INFO("Operating System: macOS %s\n", osVersion);
    #endif
}


#endif // PLATFORM_DEFS_H
