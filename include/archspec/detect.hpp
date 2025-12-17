// This file is a part of Julia. License is MIT: https://julialang.org/license

#ifndef ARCHSPEC_DETECT_HPP
#define ARCHSPEC_DETECT_HPP

#include "microarchitecture.hpp"
#include <string>
#include <set>
#include <optional>

namespace archspec {

// Architecture constants
constexpr const char* ARCH_X86_64 = "x86_64";
constexpr const char* ARCH_AARCH64 = "aarch64";
constexpr const char* ARCH_PPC64LE = "ppc64le";
constexpr const char* ARCH_PPC64 = "ppc64";
constexpr const char* ARCH_RISCV64 = "riscv64";

/**
 * Information detected about the host CPU
 */
struct DetectedCpuInfo {
    std::string name;               // Detected CPU name (if available)
    std::string vendor;             // CPU vendor
    std::set<std::string> features; // Detected CPU features
    int generation = 0;             // Power generation (POWER CPUs only)
    std::string cpu_part;           // CPU part number (ARM only)
};

/**
 * Get the machine architecture string
 * Returns "x86_64", "aarch64", "ppc64le", etc.
 */
std::string get_machine();

/**
 * Detect CPU information from the host
 * Uses /proc/cpuinfo on Linux, sysctl on macOS/BSD, CPUID on Windows
 */
DetectedCpuInfo detect_cpu_info();

/**
 * Get the host microarchitecture
 * This is the main entry point for CPU detection
 */
Microarchitecture host();

/**
 * Get the CPU brand string (if available)
 */
std::optional<std::string> brand_string();

/**
 * Get a list of all microarchitectures compatible with the detected CPU
 */
std::vector<const Microarchitecture*> compatible_microarchitectures(const DetectedCpuInfo& info);

// Platform-specific detection functions

#if defined(__linux__) || defined(__FreeBSD__)
/**
 * Parse /proc/cpuinfo (Linux) or similar (FreeBSD)
 */
DetectedCpuInfo detect_from_proc_cpuinfo();
#endif

#if defined(__APPLE__)
/**
 * Detect CPU info using sysctl on macOS
 */
DetectedCpuInfo detect_from_sysctl();
#endif

#if defined(_WIN32) || defined(_WIN64)
/**
 * Detect CPU info using CPUID on Windows
 */
DetectedCpuInfo detect_from_cpuid();
#endif

// Compatibility check functions for different architectures
namespace compatibility {

bool check_x86_64(const DetectedCpuInfo& info, const Microarchitecture& target);
bool check_aarch64(const DetectedCpuInfo& info, const Microarchitecture& target);
bool check_ppc64(const DetectedCpuInfo& info, const Microarchitecture& target);
bool check_riscv64(const DetectedCpuInfo& info, const Microarchitecture& target);

} // namespace compatibility

} // namespace archspec

#endif // ARCHSPEC_DETECT_HPP
