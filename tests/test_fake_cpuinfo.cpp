// This file is a part of Julia. License is MIT: https://julialang.org/license
//
// Test CPU detection using fake /proc/cpuinfo files from archspec test data

#include "test_common.hpp"
#include <archspec/archspec.hpp>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

using namespace archspec;

// Parse cpuinfo content (similar to detect_from_proc_cpuinfo but takes string input)
DetectedCpuInfo parse_cpuinfo_content(const std::string& content, const std::string& arch) {
    DetectedCpuInfo info;

    std::map<std::string, std::string> data;
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            if (!data.empty())
                break;
            continue;
        }

        // Trim key
        std::string key = line.substr(0, colon);
        size_t start = key.find_first_not_of(" \t");
        size_t end = key.find_last_not_of(" \t");
        if (start != std::string::npos) {
            key = key.substr(start, end - start + 1);
        }

        // Trim value
        std::string value = line.substr(colon + 1);
        start = value.find_first_not_of(" \t");
        end = value.find_last_not_of(" \t\r\n");
        if (start != std::string::npos) {
            value = value.substr(start, end - start + 1);
        } else {
            value = "";
        }

        data[key] = value;
    }

    if (arch == "x86_64" || arch == "i686" || arch == "i386") {
        info.vendor = data.count("vendor_id") ? data["vendor_id"] : "generic";

        // Parse flags
        if (data.count("flags")) {
            std::istringstream flags_stream(data["flags"]);
            std::string flag;
            while (flags_stream >> flag) {
                info.features.insert(flag);
            }
        }

        // ssse3 implies sse3 (Linux reports sse3 as "pni")
        if (info.features.count("ssse3")) {
            info.features.insert("sse3");
        }
    } else if (arch == "aarch64") {
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
            std::istringstream feat_stream(data["Features"]);
            std::string feat;
            while (feat_stream >> feat) {
                info.features.insert(feat);
            }
        }

        info.cpu_part = data.count("CPU part") ? data["CPU part"] : "";
    } else if (arch == "ppc64le" || arch == "ppc64") {
        // Parse POWER generation
        if (data.count("cpu")) {
            std::string cpu_str = data["cpu"];
            size_t pos = cpu_str.find("POWER");
            if (pos != std::string::npos) {
                pos += 5; // Skip "POWER"
                std::string gen_str;
                while (pos < cpu_str.size() && std::isdigit(cpu_str[pos])) {
                    gen_str += cpu_str[pos++];
                }
                if (!gen_str.empty()) {
                    char* end = nullptr;
                    long val = std::strtol(gen_str.c_str(), &end, 10);
                    if (end != gen_str.c_str()) {
                        info.generation = static_cast<int>(val);
                    }
                }
            }
        }
    }

    return info;
}

// Detect host from cpuinfo content
std::string detect_from_content(const std::string& content, const std::string& arch) {
    DetectedCpuInfo info = parse_cpuinfo_content(content, arch);
    auto candidates = compatible_microarchitectures(info, arch);

    if (candidates.empty()) {
        return arch;
    }

    // Sorting criteria: more ancestors and more features is better
    auto sorting_fn = [](const Microarchitecture* a, const Microarchitecture* b) {
        size_t a_depth = a->ancestors().size();
        size_t b_depth = b->ancestors().size();
        if (a_depth != b_depth)
            return a_depth < b_depth;
        return a->features().size() < b->features().size();
    };

    // Find best generic candidate
    std::vector<const Microarchitecture*> generic_candidates;
    for (const auto* c : candidates) {
        if (c->vendor() == "generic") {
            generic_candidates.push_back(c);
        }
    }

    const Microarchitecture* best_generic = nullptr;
    if (!generic_candidates.empty()) {
        best_generic =
            *std::max_element(generic_candidates.begin(), generic_candidates.end(), sorting_fn);
    }

    // Filter by CPU part for AArch64
    if (!info.cpu_part.empty()) {
        std::vector<const Microarchitecture*> cpu_part_matches;
        for (const auto* c : candidates) {
            if (c->cpu_part() == info.cpu_part) {
                cpu_part_matches.push_back(c);
            }
        }
        if (!cpu_part_matches.empty()) {
            candidates = cpu_part_matches;
        }
    }

    // Filter candidates to be descendants of best generic
    if (best_generic) {
        std::vector<const Microarchitecture*> filtered;
        for (const auto* c : candidates) {
            if (*c > *best_generic) {
                filtered.push_back(c);
            }
        }
        if (!filtered.empty()) {
            candidates = filtered;
        }
    }

    if (candidates.empty()) {
        return best_generic ? best_generic->name() : arch;
    }

    const Microarchitecture* best =
        *std::max_element(candidates.begin(), candidates.end(), sorting_fn);
    return best->name();
}

// Read file content
std::string read_file_content(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Extract expected target from filename (e.g., "linux-ubuntu20.04-zen3" -> "zen3")
std::string extract_expected_target(const std::string& filename) {
    size_t last_dash = filename.rfind('-');
    if (last_dash != std::string::npos) {
        return filename.substr(last_dash + 1);
    }
    return "";
}

// Extract architecture from filename
std::string extract_arch_from_filename(const std::string& filename) {
    if (filename.find("linux-") == 0 || filename.find("bgq-") == 0) {
        // Check for power
        if (filename.find("power") != std::string::npos) {
            if (filename.find("le") != std::string::npos)
                return "ppc64le";
            return "ppc64";
        }
        // Check for ARM
        if (filename.find("cortex") != std::string::npos ||
            filename.find("neoverse") != std::string::npos ||
            filename.find("thunderx") != std::string::npos ||
            filename.find("a64fx") != std::string::npos ||
            filename.find("-m1") != std::string::npos ||
            filename.find("-m2") != std::string::npos ||
            filename.find("-m3") != std::string::npos ||
            filename.find("-m4") != std::string::npos) {
            return "aarch64";
        }
        // Default to x86_64 for Linux
        return "x86_64";
    }
    if (filename.find("darwin-") == 0) {
        // macOS - skip these as they use sysctl not cpuinfo
        return "skip";
    }
    return "x86_64";
}

TEST(fake_cpuinfo_zen3) {
    std::string content =
        read_file_content("extern/archspec/archspec/json/tests/targets/linux-ubuntu20.04-zen3");
    ASSERT(!content.empty());

    std::string detected = detect_from_content(content, "x86_64");
    std::cout << "(detected: " << detected << ", expected: zen3) ";
    ASSERT_EQ(detected, "zen3");
    TEST_PASS();
}

TEST(fake_cpuinfo_haswell) {
    std::string content =
        read_file_content("extern/archspec/archspec/json/tests/targets/linux-rhel7-haswell");
    ASSERT(!content.empty());

    std::string detected = detect_from_content(content, "x86_64");
    std::cout << "(detected: " << detected << ", expected: haswell) ";
    ASSERT_EQ(detected, "haswell");
    TEST_PASS();
}

TEST(fake_cpuinfo_broadwell) {
    std::string content =
        read_file_content("extern/archspec/archspec/json/tests/targets/linux-rhel7-broadwell");
    ASSERT(!content.empty());

    std::string detected = detect_from_content(content, "x86_64");
    std::cout << "(detected: " << detected << ", expected: broadwell) ";
    ASSERT_EQ(detected, "broadwell");
    TEST_PASS();
}

TEST(fake_cpuinfo_cascadelake) {
    std::string content =
        read_file_content("extern/archspec/archspec/json/tests/targets/linux-centos7-cascadelake");
    ASSERT(!content.empty());

    std::string detected = detect_from_content(content, "x86_64");
    std::cout << "(detected: " << detected << ", expected: cascadelake) ";
    ASSERT_EQ(detected, "cascadelake");
    TEST_PASS();
}

TEST(fake_cpuinfo_skylake_avx512) {
    std::string content =
        read_file_content("extern/archspec/archspec/json/tests/targets/linux-rhel7-skylake_avx512");
    ASSERT(!content.empty());

    std::string detected = detect_from_content(content, "x86_64");
    std::cout << "(detected: " << detected << ", expected: skylake_avx512) ";
    ASSERT_EQ(detected, "skylake_avx512");
    TEST_PASS();
}

TEST(fake_cpuinfo_piledriver) {
    std::string content =
        read_file_content("extern/archspec/archspec/json/tests/targets/linux-rhel6-piledriver");
    ASSERT(!content.empty());

    std::string detected = detect_from_content(content, "x86_64");
    std::cout << "(detected: " << detected << ", expected: piledriver) ";
    ASSERT_EQ(detected, "piledriver");
    TEST_PASS();
}

TEST(fake_cpuinfo_zen4) {
    std::string content =
        read_file_content("extern/archspec/archspec/json/tests/targets/linux-rocky8.5-zen4");
    ASSERT(!content.empty());

    std::string detected = detect_from_content(content, "x86_64");
    std::cout << "(detected: " << detected << ", expected: zen4) ";
    ASSERT_EQ(detected, "zen4");
    TEST_PASS();
}

// Note: ARM and Power tests would require architecture simulation support
// since compatible_microarchitectures() uses get_machine() internally.
// These are left as placeholders for future cross-architecture testing support.

TEST(fake_cpuinfo_zen) {
    std::string content =
        read_file_content("extern/archspec/archspec/json/tests/targets/linux-rhel7-zen");
    ASSERT(!content.empty());

    std::string detected = detect_from_content(content, "x86_64");
    std::cout << "(detected: " << detected << ", expected: zen) ";
    ASSERT_EQ(detected, "zen");
    TEST_PASS();
}

TEST(fake_cpuinfo_zen5) {
    std::string content =
        read_file_content("extern/archspec/archspec/json/tests/targets/linux-rocky9-zen5");
    ASSERT(!content.empty());

    std::string detected = detect_from_content(content, "x86_64");
    std::cout << "(detected: " << detected << ", expected: zen5) ";
    ASSERT_EQ(detected, "zen5");
    TEST_PASS();
}

int main() {
    std::cout << "=== archspec_cpp Fake cpuinfo Tests ===" << std::endl;
    std::cout << std::endl;

    // x86_64 tests (Intel)
    RUN_TEST(fake_cpuinfo_haswell);
    RUN_TEST(fake_cpuinfo_broadwell);
    RUN_TEST(fake_cpuinfo_skylake_avx512);
    RUN_TEST(fake_cpuinfo_cascadelake);

    // x86_64 tests (AMD)
    RUN_TEST(fake_cpuinfo_piledriver);
    RUN_TEST(fake_cpuinfo_zen);
    RUN_TEST(fake_cpuinfo_zen3);
    RUN_TEST(fake_cpuinfo_zen4);
    RUN_TEST(fake_cpuinfo_zen5);

    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << g_tests_passed << std::endl;
    std::cout << "Failed: " << g_tests_failed << std::endl;

    return g_tests_failed > 0 ? 1 : 0;
}
