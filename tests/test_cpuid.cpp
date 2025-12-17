// This file is a part of Julia. License is MIT: https://julialang.org/license
//
// Unit tests for CPUID functionality (x86/x86_64 only)

#include <archspec/cpuid.hpp>
#include <archspec/detect.hpp>
#include <iostream>
#include <cassert>

using namespace archspec;

// Simple test framework
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "... "; \
    try { \
        test_##name(); \
        std::cout << "PASSED" << std::endl; \
        tests_passed++; \
    } catch (const std::exception& e) { \
        std::cout << "FAILED: " << e.what() << std::endl; \
        tests_failed++; \
    } catch (...) { \
        std::cout << "FAILED: unknown exception" << std::endl; \
        tests_failed++; \
    } \
} while(0)

#define SKIP_TEST(name, reason) do { \
    std::cout << "Skipping " #name ": " reason << std::endl; \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        throw std::runtime_error("Assertion failed: " #cond); \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        throw std::runtime_error("Assertion failed: " #a " == " #b); \
    } \
} while(0)

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
}

// Test vendor detection
TEST(vendor_detection) {
    if (!Cpuid::is_supported()) {
        std::cout << "(skipped - not x86) ";
        return;
    }
    
    Cpuid cpuid;
    std::string vendor = cpuid.vendor();
    
    ASSERT(!vendor.empty());
    ASSERT(vendor.length() == 12);  // Vendor string is always 12 chars
    
    // Should be one of the known vendors
    bool known_vendor = (vendor == "GenuineIntel" ||
                         vendor == "AuthenticAMD" ||
                         vendor == "GenuineTMx86" ||
                         vendor == "CentaurHauls" ||
                         vendor == "CyrixInstead" ||
                         vendor == "TransmetaCPU" ||
                         vendor == "GenuineTMx86" ||
                         vendor == "Geode by NSC" ||
                         vendor == "VIA VIA VIA " ||
                         vendor == "HygonGenuine" ||
                         // Hypervisors
                         vendor == "VMwareVMware" ||
                         vendor == "Microsoft Hv" ||
                         vendor == "KVMKVMKVM\0\0\0" ||
                         vendor == " lrpepyh vr\0" ||  // Parallels
                         vendor == "XenVMMXenVMM" ||
                         vendor == "prl hyperv  " ||
                         vendor == "bhyve bhyve ");
    
    std::cout << "(vendor: " << vendor << ") ";
    // Don't fail on unknown vendor - might be running in unusual environment
}

// Test highest function support
TEST(highest_function) {
    if (!Cpuid::is_supported()) {
        std::cout << "(skipped - not x86) ";
        return;
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
}

// Test feature detection
TEST(feature_detection) {
    if (!Cpuid::is_supported()) {
        std::cout << "(skipped - not x86) ";
        return;
    }
    
    Cpuid cpuid;
    const auto& features = cpuid.features();
    
    ASSERT(!features.empty());
    std::cout << "(detected " << features.size() << " features) ";
    
    // All modern x86_64 CPUs should have these basic features
    std::string machine = get_machine();
    if (machine == ARCH_X86_64) {
        ASSERT(features.count("fpu") || features.count("sse2"));  // At minimum
    }
}

// Test brand string
TEST(brand_string_cpuid) {
    if (!Cpuid::is_supported()) {
        std::cout << "(skipped - not x86) ";
        return;
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
}

// Test CPUID query
TEST(cpuid_query) {
    if (!Cpuid::is_supported()) {
        std::cout << "(skipped - not x86) ";
        return;
    }
    
    Cpuid cpuid;
    
    // Query EAX=0 - should return vendor and max basic function
    CpuidRegisters regs = cpuid.query(0, 0);
    ASSERT(regs.eax >= 1);  // Should support at least EAX=1
    
    // EBX, EDX, ECX form the vendor string
    ASSERT(regs.ebx != 0 || regs.ecx != 0 || regs.edx != 0);
    
    std::cout << "(EAX=0: max=" << regs.eax << ") ";
}

// Test feature consistency with detection
TEST(feature_consistency) {
    if (!Cpuid::is_supported()) {
        std::cout << "(skipped - not x86) ";
        return;
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
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;
    
    return tests_failed > 0 ? 1 : 0;
}

