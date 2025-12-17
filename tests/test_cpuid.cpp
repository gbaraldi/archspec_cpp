// This file is a part of Julia. License is MIT: https://julialang.org/license
//
// Unit tests for CPUID functionality (x86/x86_64 only)

#include "test_common.hpp"
#include <archspec/cpuid.hpp>
#include <archspec/detect.hpp>

using namespace archspec;

// Test CPUID support detection
TEST(cpuid_support) {
    bool supported = Cpuid::is_supported();
    std::string machine = get_machine();

    if (machine == ARCH_X86_64 || machine == "i686" || machine == "i386") {
        ASSERT(supported);
        std::cout << "(supported on " << machine << ") ";
    } else {
        ASSERT(!supported);
        std::cout << "(not supported on " << machine << ") ";
    }
    TEST_PASS();
}

// Test vendor detection
TEST(vendor_detection) {
    if (!Cpuid::is_supported()) {
        std::cout << "(skipped - not x86) ";
        TEST_PASS();
    }

    Cpuid cpuid;
    std::string vendor = cpuid.vendor();

    ASSERT(!vendor.empty());
    ASSERT(vendor.length() == 12); // Vendor string is always 12 chars

    // Should be one of the known vendors
    bool known_vendor =
        (vendor == "GenuineIntel" || vendor == "AuthenticAMD" || vendor == "GenuineTMx86" ||
         vendor == "CentaurHauls" || vendor == "CyrixInstead" || vendor == "TransmetaCPU" ||
         vendor == "GenuineTMx86" || vendor == "Geode by NSC" || vendor == "VIA VIA VIA " ||
         vendor == "HygonGenuine" ||
         // Hypervisors
         vendor == "VMwareVMware" || vendor == "Microsoft Hv" || vendor == "KVMKVMKVM\0\0\0" ||
         vendor == " lrpepyh vr\0" || // Parallels
         vendor == "XenVMMXenVMM" || vendor == "prl hyperv  " || vendor == "bhyve bhyve ");

    std::cout << "(vendor: " << vendor;
    if (!known_vendor)
        std::cout << " [unknown]";
    std::cout << ") ";
    TEST_PASS();
}

// Test highest function support
TEST(highest_function) {
    if (!Cpuid::is_supported()) {
        std::cout << "(skipped - not x86) ";
        TEST_PASS();
    }

    Cpuid cpuid;
    uint32_t basic = cpuid.highest_basic_function();
    uint32_t extended = cpuid.highest_extended_function();

    // Basic should be at least 1 (most CPUs support much more)
    ASSERT(basic >= 1);

    // Extended should be at least 0x80000000 if any extended functions are supported
    if (extended > 0) {
        ASSERT(extended >= 0x80000000);
    }

    std::cout << "(basic: " << std::hex << basic << ", extended: " << extended << std::dec << ") ";
    TEST_PASS();
}

// Test feature detection
TEST(feature_detection) {
    if (!Cpuid::is_supported()) {
        std::cout << "(skipped - not x86) ";
        TEST_PASS();
    }

    Cpuid cpuid;
    const auto& features = cpuid.features();

    ASSERT(!features.empty());
    std::cout << "(detected " << features.size() << " features) ";

    // All modern x86_64 CPUs should have these basic features
    std::string machine = get_machine();
    if (machine == ARCH_X86_64) {
        ASSERT(features.count("fpu") || features.count("sse2")); // At minimum
    }
    TEST_PASS();
}

// Test brand string
TEST(brand_string_cpuid) {
    if (!Cpuid::is_supported()) {
        std::cout << "(skipped - not x86) ";
        TEST_PASS();
    }

    Cpuid cpuid;

    // Brand string requires extended functions
    if (cpuid.highest_extended_function() >= 0x80000004) {
        std::string brand = cpuid.brand_string();
        ASSERT(!brand.empty());
        std::cout << "(brand: " << brand << ") ";
    } else {
        std::cout << "(brand string not supported) ";
    }
    TEST_PASS();
}

// Test CPUID query
TEST(cpuid_query) {
    if (!Cpuid::is_supported()) {
        std::cout << "(skipped - not x86) ";
        TEST_PASS();
    }

    Cpuid cpuid;

    // Query EAX=0 - should return vendor and max basic function
    CpuidRegisters regs = cpuid.query(0, 0);
    ASSERT(regs.eax >= 1); // Should support at least EAX=1

    // EBX, EDX, ECX form the vendor string
    ASSERT(regs.ebx != 0 || regs.ecx != 0 || regs.edx != 0);

    std::cout << "(EAX=0: max=" << regs.eax << ") ";
    TEST_PASS();
}

// Test feature consistency with detection
TEST(feature_consistency) {
    if (!Cpuid::is_supported()) {
        std::cout << "(skipped - not x86) ";
        TEST_PASS();
    }

    Cpuid cpuid;
    DetectedCpuInfo info = detect_cpu_info();

    // On x86, CPUID features and detected features should overlap
    const auto& cpuid_features = cpuid.features();

    // Some features detected by CPUID should also be in detected info
    int common = 0;
    for (const auto& f : cpuid_features) {
        if (info.features.count(f)) {
            common++;
        }
    }

    std::cout << "(common features: " << common << "/" << cpuid_features.size() << ") ";

    // At least some features should be common
    ASSERT(common > 0 || cpuid_features.empty());
    TEST_PASS();
}

int main() {
    std::cout << "=== archspec_cpp CPUID Tests ===" << std::endl;
    std::cout << std::endl;

    RUN_TEST(cpuid_support);
    RUN_TEST(vendor_detection);
    RUN_TEST(highest_function);
    RUN_TEST(feature_detection);
    RUN_TEST(brand_string_cpuid);
    RUN_TEST(cpuid_query);
    RUN_TEST(feature_consistency);

    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << g_tests_passed << std::endl;
    std::cout << "Failed: " << g_tests_failed << std::endl;

    return g_tests_failed > 0 ? 1 : 0;
}
