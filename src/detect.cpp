// This file is a part of Julia. License is MIT: https://julialang.org/license

#include "archspec/detect.hpp"
#include "archspec/cpuid.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>
#include <regex>

// Platform-specific includes
#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/machine.h>
#endif

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
#include <sys/utsname.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
// PROCESSOR_ARCHITECTURE_ARM64 may not be defined in older SDKs/MinGW
#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 12
#endif
#endif

#if defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

namespace archspec {

namespace {

[[maybe_unused]] std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream(s);
    while (std::getline(stream, token, delimiter)) {
        size_t start = token.find_first_not_of(" \t\r\n");
        size_t end = token.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos)
            tokens.push_back(token.substr(start, end - start + 1));
        else if (start != std::string::npos)
            tokens.push_back(token.substr(start));
    }
    return tokens;
}

[[maybe_unused]] std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

} // anonymous namespace

std::string get_machine() {
#if defined(_WIN32) || defined(_WIN64)
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);
    switch (sysInfo.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
        return ARCH_X86_64;
    case PROCESSOR_ARCHITECTURE_ARM64:
        return ARCH_AARCH64;
    case PROCESSOR_ARCHITECTURE_INTEL:
        return "i686";
    default:
        return "unknown";
    }
#elif defined(__APPLE__)
    // Check brand string first (Rosetta may report wrong arch)
    char brand[256] = {0};
    size_t size = sizeof(brand);
    if (sysctlbyname("machdep.cpu.brand_string", brand, &size, nullptr, 0) == 0) {
        if (std::string(brand).find("Apple") != std::string::npos)
            return ARCH_AARCH64;
    }
    struct utsname uts;
    if (uname(&uts) == 0) {
        std::string machine = uts.machine;
        if (machine == "arm64")
            return ARCH_AARCH64;
        return machine;
    }
    return "unknown";
#else
    struct utsname uts;
    if (uname(&uts) == 0) {
        std::string machine = uts.machine;
        if (machine == "arm64")
            return ARCH_AARCH64;
        if (machine == "amd64")
            return ARCH_X86_64;
        return machine;
    }
    return "unknown";
#endif
}

#if defined(__linux__)
// Helper to trim string (Linux only)
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

DetectedCpuInfo detect_from_proc_cpuinfo() {
    DetectedCpuInfo info;

    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        return info;
    }

    std::map<std::string, std::string> data;
    std::string line;
    while (std::getline(cpuinfo, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            // Empty line - we've read one CPU's info
            if (!data.empty())
                break;
            continue;
        }

        std::string key = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));
        data[key] = value;
    }

    std::string arch = get_machine();

    if (arch == ARCH_X86_64 || arch == "i686" || arch == "i386") {
        info.vendor = data.count("vendor_id") ? data["vendor_id"] : "generic";

        // Parse flags
        if (data.count("flags")) {
            for (const auto& flag : split(data["flags"], ' ')) {
                if (!flag.empty()) {
                    info.features.insert(flag);
                }
            }
        }

        // ssse3 implies sse3; on Linux sse3 is reported as "pni" in /proc/cpuinfo,
        // so add sse3 when ssse3 is present (matching archspec Python behavior)
        if (info.features.count("ssse3")) {
            info.features.insert("sse3");
        }
    } else if (arch == ARCH_AARCH64) {
        // Get vendor from CPU implementer
        if (data.count("CPU implementer")) {
            const auto& db = MicroarchitectureDatabase::instance();
            const auto& vendors = db.arm_vendors();
            auto it = vendors.find(data["CPU implementer"]);
            if (it != vendors.end()) {
                info.vendor = it->second;
            } else {
                info.vendor = data["CPU implementer"];
            }
        } else {
            info.vendor = "generic";
        }

        // Parse features
        if (data.count("Features")) {
            for (const auto& flag : split(data["Features"], ' ')) {
                if (!flag.empty()) {
                    info.features.insert(flag);
                }
            }
        }

        // CPU part
        info.cpu_part = data.count("CPU part") ? data["CPU part"] : "";
    } else if (arch == ARCH_PPC64LE || arch == ARCH_PPC64) {
        // Parse POWER generation
        if (data.count("cpu")) {
            std::regex power_re("POWER(\\d+)");
            std::smatch match;
            std::string cpu_str = data["cpu"];
            if (std::regex_search(cpu_str, match, power_re)) {
                std::string gen_str = match[1].str();
                char* end = nullptr;
                long val = std::strtol(gen_str.c_str(), &end, 10);
                if (end != gen_str.c_str()) {
                    info.generation = static_cast<int>(val);
                }
            }
        }
    } else if (arch == ARCH_RISCV64) {
        if (data.count("uarch")) {
            std::string uarch = data["uarch"];
            if (uarch == "sifive,u74-mc") {
                uarch = "u74mc";
            }
            info.name = uarch;
        } else {
            info.name = ARCH_RISCV64;
        }
    }

    return info;
}
#endif

#if defined(__FreeBSD__)
DetectedCpuInfo detect_from_proc_cpuinfo() {
    DetectedCpuInfo info;
    std::string arch = get_machine();

    if (arch == ARCH_X86_64 || arch == "i686") {
        // Use CPUID on FreeBSD for x86
        if (Cpuid::is_supported()) {
            Cpuid cpuid;
            info.vendor = cpuid.vendor();
            info.features = cpuid.features();
        }
    } else if (arch == ARCH_AARCH64) {
        // Try to get features from hw.optional sysctls
        info.vendor = "generic";
    }

    return info;
}
#endif

#if defined(__APPLE__)
namespace {

std::string sysctl_string(const char* name) {
    char buf[256] = {0};
    size_t size = sizeof(buf);
    if (sysctlbyname(name, buf, &size, nullptr, 0) == 0) {
        return std::string(buf);
    }
    return "";
}

} // anonymous namespace

DetectedCpuInfo detect_from_sysctl() {
    DetectedCpuInfo info;
    std::string arch = get_machine();

    if (arch == ARCH_X86_64) {
        info.vendor = sysctl_string("machdep.cpu.vendor");

        // Get features from multiple sysctl calls
        std::string features_str;
        features_str += " " + sysctl_string("machdep.cpu.features");
        features_str += " " + sysctl_string("machdep.cpu.leaf7_features");
        features_str += " " + sysctl_string("machdep.cpu.extfeatures");

        // Parse and convert Darwin flags to Linux equivalents
        const auto& db = MicroarchitectureDatabase::instance();
        const auto& darwin_conv = db.darwin_flag_conversions();

        std::set<std::string> raw_features;
        for (const auto& flag : split(to_lower(features_str), ' ')) {
            if (!flag.empty()) {
                raw_features.insert(flag);
            }
        }

        // Apply conversions
        for (const auto& [darwin_flags, linux_flags] : darwin_conv) {
            auto darwin_parts = split(darwin_flags, ' ');
            bool all_present = true;
            for (const auto& part : darwin_parts) {
                if (raw_features.count(part) == 0) {
                    all_present = false;
                    break;
                }
            }
            if (all_present) {
                for (const auto& linux_flag : split(linux_flags, ' ')) {
                    info.features.insert(linux_flag);
                }
            }
        }

        // Add raw features
        for (const auto& f : raw_features) {
            info.features.insert(f);
        }
    } else if (arch == ARCH_AARCH64) {
        info.vendor = "Apple";

        // Detect Apple Silicon model from brand string
        std::string brand = to_lower(sysctl_string("machdep.cpu.brand_string"));

        if (brand.find("m4") != std::string::npos) {
            info.name = "m4";
        } else if (brand.find("m3") != std::string::npos) {
            info.name = "m3";
        } else if (brand.find("m2") != std::string::npos) {
            info.name = "m2";
        } else if (brand.find("m1") != std::string::npos) {
            info.name = "m1";
        } else if (brand.find("apple") != std::string::npos) {
            info.name = "m1"; // Default to M1 for unknown Apple Silicon
        }
    }

    return info;
}
#endif

#if defined(_WIN32) || defined(_WIN64)
DetectedCpuInfo detect_from_cpuid() {
    DetectedCpuInfo info;
    std::string arch = get_machine();

    if (arch == ARCH_X86_64) {
        if (Cpuid::is_supported()) {
            Cpuid cpuid;
            info.vendor = cpuid.vendor();
            info.features = cpuid.features();
        }
    }
    // TODO: Add ARM64 Windows support if needed

    return info;
}
#endif

DetectedCpuInfo detect_cpu_info() {
#if defined(__linux__) || defined(__FreeBSD__)
    return detect_from_proc_cpuinfo();
#elif defined(__APPLE__)
    return detect_from_sysctl();
#elif defined(_WIN32) || defined(_WIN64)
    return detect_from_cpuid();
#else
    // Fallback - try CPUID for x86
    DetectedCpuInfo info;
    if (Cpuid::is_supported()) {
        Cpuid cpuid;
        info.vendor = cpuid.vendor();
        info.features = cpuid.features();
    }
    return info;
#endif
}

std::optional<std::string> brand_string() {
#if defined(__APPLE__)
    char brand[256] = {0};
    size_t size = sizeof(brand);
    if (sysctlbyname("machdep.cpu.brand_string", brand, &size, nullptr, 0) == 0) {
        return std::string(brand);
    }
    return std::nullopt;
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    if (Cpuid::is_supported()) {
        Cpuid cpuid;
        std::string brand = cpuid.brand_string();
        if (!brand.empty()) {
            return brand;
        }
    }
    return std::nullopt;
#else
    return std::nullopt;
#endif
}

namespace compatibility {

static bool is_in_family(const Microarchitecture& target, const std::string& family_name) {
    if (target.name() == family_name)
        return true;
    for (const auto& ancestor : target.ancestors()) {
        if (ancestor == family_name)
            return true;
    }
    return false;
}

static bool vendor_matches(const Microarchitecture& target, const std::string& info_vendor) {
    return target.vendor() == "generic" || target.vendor() == info_vendor;
}

static bool has_required_features(const Microarchitecture& target,
                                  const std::set<std::string>& available_features) {
    for (const auto& feature : target.features()) {
        if (available_features.count(feature) == 0)
            return false;
    }
    return true;
}

bool check_x86_64(const DetectedCpuInfo& info, const Microarchitecture& target) {
    return is_in_family(target, ARCH_X86_64) && vendor_matches(target, info.vendor) &&
           has_required_features(target, info.features);
}

bool check_aarch64(const DetectedCpuInfo& info, const Microarchitecture& target) {
    if (target.vendor() == "generic" && target.name() != ARCH_AARCH64)
        return false;
    if (!is_in_family(target, ARCH_AARCH64))
        return false;
    if (!vendor_matches(target, info.vendor))
        return false;

#if defined(__APPLE__)
    if (!info.name.empty()) {
        const auto* model = MicroarchitectureDatabase::instance().get(info.name);
        if (model) {
            if (target.name() == info.name)
                return true;
            for (const auto& ancestor : model->ancestors()) {
                if (ancestor == target.name())
                    return true;
            }
        }
    }
#else
    if (!has_required_features(target, info.features))
        return false;
#endif
    return true;
}

bool check_ppc64(const DetectedCpuInfo& info, const Microarchitecture& target) {
    std::string arch = get_machine();
    return is_in_family(target, arch) && target.generation() <= info.generation;
}

bool check_riscv64(const DetectedCpuInfo& info, const Microarchitecture& target) {
    return is_in_family(target, ARCH_RISCV64) &&
           (target.name() == info.name || target.vendor() == "generic");
}

} // namespace compatibility

std::vector<const Microarchitecture*> compatible_microarchitectures(const DetectedCpuInfo& info,
                                                                    const std::string& arch) {
    std::vector<const Microarchitecture*> result;
    const auto& db = MicroarchitectureDatabase::instance();

    std::function<bool(const DetectedCpuInfo&, const Microarchitecture&)> checker;
    if (arch == ARCH_X86_64 || arch == "i686" || arch == "i386")
        checker = compatibility::check_x86_64;
    else if (arch == ARCH_AARCH64)
        checker = compatibility::check_aarch64;
    else if (arch == ARCH_PPC64 || arch == ARCH_PPC64LE)
        checker = compatibility::check_ppc64;
    else if (arch == ARCH_RISCV64)
        checker = compatibility::check_riscv64;
    else {
        if (const auto* generic = db.get(arch))
            result.push_back(generic);
        return result;
    }

    for (const auto& [name, target] : db.all()) {
        if (checker(info, target))
            result.push_back(&target);
    }

    if (result.empty()) {
        if (const auto* generic = db.get(arch))
            result.push_back(generic);
    }

    return result;
}

std::vector<const Microarchitecture*> compatible_microarchitectures(const DetectedCpuInfo& info) {
    return compatible_microarchitectures(info, get_machine());
}

Microarchitecture host() {
    DetectedCpuInfo info = detect_cpu_info();
    auto candidates = compatible_microarchitectures(info);

    if (candidates.empty())
        return generic_microarchitecture(get_machine());

    std::vector<const Microarchitecture*> generic_candidates;
    for (const auto* c : candidates) {
        if (c->vendor() == "generic")
            generic_candidates.push_back(c);
    }

    const Microarchitecture* best_generic = nullptr;
    if (!generic_candidates.empty()) {
        best_generic = *std::max_element(generic_candidates.begin(), generic_candidates.end(),
                                         compare_microarch_specificity);
    }

    if (!info.cpu_part.empty()) {
        std::vector<const Microarchitecture*> cpu_part_matches;
        for (const auto* c : candidates) {
            if (c->cpu_part() == info.cpu_part)
                cpu_part_matches.push_back(c);
        }
        if (!cpu_part_matches.empty())
            candidates = cpu_part_matches;
    }

    if (best_generic) {
        std::vector<const Microarchitecture*> filtered;
        for (const auto* c : candidates) {
            if (*c > *best_generic)
                filtered.push_back(c);
        }
        if (!filtered.empty())
            candidates = filtered;
    }

    if (candidates.empty())
        return best_generic ? *best_generic : generic_microarchitecture(get_machine());

    return **std::max_element(candidates.begin(), candidates.end(), compare_microarch_specificity);
}

} // namespace archspec
