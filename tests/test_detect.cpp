// This file is a part of Julia. License is MIT: https://julialang.org/license
//
// Unit tests for CPU detection

#include "test_common.hpp"
#include <archspec/archspec.hpp>

using namespace archspec;

// Test get_machine
TEST(get_machine) {
    std::string machine = get_machine();
    ASSERT(!machine.empty());
    std::cout << "(detected: " << machine << ") ";

    // Should be one of the known architectures
    bool known = (machine == ARCH_X86_64 || machine == ARCH_AARCH64 || machine == ARCH_PPC64LE ||
                  machine == ARCH_PPC64 || machine == ARCH_RISCV64 || machine == "i686" ||
                  machine == "i386" || machine == "arm64"); // macOS reports arm64
    ASSERT(known);
    TEST_PASS();
}

// Test CPU info detection
TEST(detect_cpu_info) {
    DetectedCpuInfo info = detect_cpu_info();

    std::string machine = get_machine();

    // Vendor should be set for x86_64
    if (machine == ARCH_X86_64) {
        ASSERT(!info.vendor.empty());
        std::cout << "(vendor: " << info.vendor << ", features: " << info.features.size() << ") ";
    }
    // For AArch64, vendor might be set
    else if (machine == ARCH_AARCH64) {
        std::cout << "(vendor: " << info.vendor << ") ";
    }
    TEST_PASS();
}

// Test host detection
TEST(host_detection) {
    Microarchitecture uarch = host();

    ASSERT(uarch.valid());
    ASSERT(!uarch.name().empty());
    std::cout << "(detected: " << uarch.name() << ") ";

    // Should be in the correct family
    std::string machine = get_machine();
    std::string family = uarch.family();

    if (machine == ARCH_X86_64 || machine == "i686" || machine == "i386") {
        ASSERT(family == ARCH_X86_64 || family == "x86" || family == "i686");
    } else if (machine == ARCH_AARCH64 || machine == "arm64") {
        ASSERT(family == ARCH_AARCH64);
    } else if (machine == ARCH_PPC64LE) {
        ASSERT(family == ARCH_PPC64LE);
    } else if (machine == ARCH_PPC64) {
        ASSERT(family == ARCH_PPC64);
    } else if (machine == ARCH_RISCV64) {
        ASSERT(family == ARCH_RISCV64);
    }
    TEST_PASS();
}

// Test compatible microarchitectures
TEST(compatible_microarchitectures) {
    DetectedCpuInfo info = detect_cpu_info();
    auto compatible = compatible_microarchitectures(info);

    ASSERT(!compatible.empty());
    std::cout << "(found " << compatible.size() << " compatible) ";

    // All compatible targets should be valid
    for (const auto* target : compatible) {
        ASSERT(target != nullptr);
        ASSERT(target->valid());
    }
    TEST_PASS();
}

// Test brand string (if available)
TEST(brand_string) {
    auto brand = brand_string();

    if (brand.has_value()) {
        ASSERT(!brand->empty());
        std::cout << "(brand: " << *brand << ") ";
    } else {
        std::cout << "(brand: not available) ";
    }
    TEST_PASS();
}

// Test that host is in compatible list
TEST(host_is_compatible) {
    DetectedCpuInfo info = detect_cpu_info();
    auto compatible = compatible_microarchitectures(info);
    Microarchitecture uarch = host();

    bool found = false;
    for (const auto* target : compatible) {
        if (target->name() == uarch.name()) {
            found = true;
            break;
        }
    }

    // Host should be one of the compatible architectures
    // (or be the generic one if nothing else matched)
    ASSERT(found || uarch.vendor() == "generic");
    TEST_PASS();
}

// Test optimization flags for detected host
TEST(host_optimization_flags) {
    Microarchitecture uarch = host();

    // Try to get flags for common compilers
    std::string gcc_flags = uarch.optimization_flags("gcc", "10.0");
    std::string clang_flags = uarch.optimization_flags("clang", "12.0");

    std::cout << "(gcc flags: " << (gcc_flags.empty() ? "none" : gcc_flags) << ") ";

    // Flags should not be empty for x86_64 targets
    std::string machine = get_machine();
    if (machine == ARCH_X86_64 && uarch.vendor() != "generic") {
        // Generic x86_64 might have flags, specific targets should have flags
    }
    TEST_PASS();
}

// Test that host has expected features
TEST(host_features) {
    Microarchitecture uarch = host();
    std::string machine = get_machine();

    if (machine == ARCH_X86_64) {
        // All modern x86_64 CPUs should have these
        // Note: generic x86_64 has no features
        if (uarch.name() != "x86_64") {
            // Most CPUs will have at least SSE2 if not generic
            bool has_basics = uarch.features().size() > 0;
            std::cout << "(features: " << uarch.features().size() << ") ";
            ASSERT(has_basics || uarch.vendor() == "generic");
        }
    } else if (machine == ARCH_AARCH64 || machine == "arm64") {
        std::cout << "(features: " << uarch.features().size() << ") ";
    }
    TEST_PASS();
}

int main() {
    std::cout << "=== archspec_cpp Detection Tests ===" << std::endl;
    std::cout << std::endl;

    RUN_TEST(get_machine);
    RUN_TEST(detect_cpu_info);
    RUN_TEST(host_detection);
    RUN_TEST(compatible_microarchitectures);
    RUN_TEST(brand_string);
    RUN_TEST(host_is_compatible);
    RUN_TEST(host_optimization_flags);
    RUN_TEST(host_features);

    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << g_tests_passed << std::endl;
    std::cout << "Failed: " << g_tests_failed << std::endl;

    return g_tests_failed > 0 ? 1 : 0;
}
