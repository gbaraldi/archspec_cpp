// This file is a part of Julia. License is MIT: https://julialang.org/license
//
// Unit tests for microarchitecture database

#include <archspec/microarchitecture.hpp>
#include <iostream>
#include <cassert>
#include <cstring>

using namespace archspec;

// Simple test framework
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name)                                             \
    do {                                                           \
        std::cout << "Running " #name "... ";                      \
        try {                                                      \
            test_##name();                                         \
            std::cout << "PASSED" << std::endl;                    \
            tests_passed++;                                        \
        } catch (const std::exception& e) {                        \
            std::cout << "FAILED: " << e.what() << std::endl;      \
            tests_failed++;                                        \
        } catch (...) {                                            \
            std::cout << "FAILED: unknown exception" << std::endl; \
            tests_failed++;                                        \
        }                                                          \
    } while (0)

#define ASSERT(cond)                                              \
    do {                                                          \
        if (!(cond)) {                                            \
            throw std::runtime_error("Assertion failed: " #cond); \
        }                                                         \
    } while (0)

#define ASSERT_EQ(a, b)                                                  \
    do {                                                                 \
        if ((a) != (b)) {                                                \
            throw std::runtime_error("Assertion failed: " #a " == " #b); \
        }                                                                \
    } while (0)

// Test that database loads correctly
TEST(database_loads) {
    const auto& db = MicroarchitectureDatabase::instance();
    auto names = db.all_names();
    ASSERT(names.size() > 0);
    std::cout << "(found " << names.size() << " targets) ";
}

// Test getting specific targets
TEST(get_x86_64) {
    const auto* target = get_target("x86_64");
    ASSERT(target != nullptr);
    ASSERT_EQ(target->name(), "x86_64");
    ASSERT_EQ(target->vendor(), "generic");
}

TEST(get_haswell) {
    const auto* target = get_target("haswell");
    ASSERT(target != nullptr);
    ASSERT_EQ(target->name(), "haswell");
    ASSERT_EQ(target->vendor(), "GenuineIntel");
    ASSERT(target->features().count("avx2") > 0);
    ASSERT(target->features().count("fma") > 0);
}

TEST(get_zen3) {
    const auto* target = get_target("zen3");
    ASSERT(target != nullptr);
    ASSERT_EQ(target->name(), "zen3");
    ASSERT_EQ(target->vendor(), "AuthenticAMD");
    ASSERT(target->features().count("avx2") > 0);
}

TEST(get_aarch64) {
    const auto* target = get_target("aarch64");
    ASSERT(target != nullptr);
    ASSERT_EQ(target->name(), "aarch64");
    ASSERT_EQ(target->vendor(), "generic");
}

TEST(get_apple_m1) {
    const auto* target = get_target("m1");
    ASSERT(target != nullptr);
    ASSERT_EQ(target->name(), "m1");
    ASSERT_EQ(target->vendor(), "Apple");
}

TEST(get_nonexistent) {
    const auto* target = get_target("nonexistent_cpu_12345");
    ASSERT(target == nullptr);
}

// Test ancestry
TEST(ancestors_haswell) {
    const auto* target = get_target("haswell");
    ASSERT(target != nullptr);

    auto ancestors = target->ancestors();
    ASSERT(ancestors.size() > 0);

    // haswell should descend from x86_64
    bool has_x86_64 = false;
    for (const auto& a : ancestors) {
        if (a == "x86_64") {
            has_x86_64 = true;
            break;
        }
    }
    ASSERT(has_x86_64);
}

TEST(ancestors_zen4) {
    const auto* target = get_target("zen4");
    ASSERT(target != nullptr);

    auto ancestors = target->ancestors();

    // zen4 should descend from zen3
    bool has_zen3 = false;
    for (const auto& a : ancestors) {
        if (a == "zen3") {
            has_zen3 = true;
            break;
        }
    }
    ASSERT(has_zen3);
}

// Test family
TEST(family_haswell) {
    const auto* target = get_target("haswell");
    ASSERT(target != nullptr);
    ASSERT_EQ(target->family(), "x86_64");
}

TEST(family_m1) {
    const auto* target = get_target("m1");
    ASSERT(target != nullptr);
    ASSERT_EQ(target->family(), "aarch64");
}

TEST(family_power9le) {
    const auto* target = get_target("power9le");
    ASSERT(target != nullptr);
    ASSERT_EQ(target->family(), "ppc64le");
}

// Test generic
TEST(generic_skylake) {
    const auto* target = get_target("skylake");
    ASSERT(target != nullptr);
    std::string gen = target->generic();
    // Should be x86_64_v3 or similar generic target
    ASSERT(!gen.empty());
}

// Test feature checking
TEST(has_feature_avx2) {
    const auto* target = get_target("haswell");
    ASSERT(target != nullptr);
    ASSERT(target->has_feature("avx2"));
    ASSERT(target->has_feature("avx"));
    ASSERT(target->has_feature("sse4_1"));
}

TEST(has_feature_alias) {
    const auto* target = get_target("haswell");
    ASSERT(target != nullptr);
    // sse4.1 is an alias for sse4_1
    ASSERT(target->has_feature("sse4.1"));
}

// Test comparison operators
TEST(comparison_subset) {
    const auto* x86_64 = get_target("x86_64");
    const auto* haswell = get_target("haswell");
    ASSERT(x86_64 != nullptr);
    ASSERT(haswell != nullptr);

    // x86_64 < haswell (x86_64 is ancestor of haswell)
    ASSERT(*x86_64 < *haswell);
    ASSERT(*x86_64 <= *haswell);
    ASSERT(!(*haswell < *x86_64));
    ASSERT(*haswell > *x86_64);
}

TEST(comparison_equality) {
    const auto* haswell1 = get_target("haswell");
    const auto* haswell2 = get_target("haswell");
    ASSERT(haswell1 != nullptr);
    ASSERT(haswell2 != nullptr);

    // Same target should be equal
    ASSERT(*haswell1 == *haswell2);
    ASSERT(!(*haswell1 != *haswell2));
    ASSERT(*haswell1 <= *haswell2);
    ASSERT(*haswell1 >= *haswell2);
}

// Test compiler flags
TEST(optimization_flags_gcc) {
    const auto* target = get_target("haswell");
    ASSERT(target != nullptr);

    std::string flags = target->optimization_flags("gcc", "9.0");
    ASSERT(!flags.empty());
    // Should contain -march=haswell
    ASSERT(flags.find("haswell") != std::string::npos);
}

TEST(optimization_flags_clang) {
    const auto* target = get_target("skylake");
    ASSERT(target != nullptr);

    std::string flags = target->optimization_flags("clang", "10.0");
    ASSERT(!flags.empty());
}

// Test generic microarchitecture creation
TEST(generic_microarchitecture) {
    auto generic = generic_microarchitecture("test_arch");
    ASSERT_EQ(generic.name(), "test_arch");
    ASSERT_EQ(generic.vendor(), "generic");
    ASSERT(generic.features().empty());
    ASSERT(generic.parent_names().empty());
}

// Test database iteration
TEST(iterate_all_targets) {
    const auto& db = MicroarchitectureDatabase::instance();
    int count = 0;
    for (const auto& [name, target] : db.all()) {
        ASSERT(!name.empty());
        ASSERT(target.valid());
        count++;
    }
    ASSERT(count > 0);
    std::cout << "(iterated " << count << " targets) ";
}

// Test Power generation
TEST(power_generation) {
    const auto* power9 = get_target("power9le");
    ASSERT(power9 != nullptr);
    ASSERT_EQ(power9->generation(), 9);

    const auto* power10 = get_target("power10le");
    ASSERT(power10 != nullptr);
    ASSERT_EQ(power10->generation(), 10);
}

// Test ARM CPU part
TEST(arm_cpu_part) {
    const auto* n1 = get_target("neoverse_n1");
    ASSERT(n1 != nullptr);
    ASSERT(!n1->cpu_part().empty());
}

int main() {
    std::cout << "=== archspec_cpp Microarchitecture Tests ===" << std::endl;
    std::cout << std::endl;

    // Database tests
    RUN_TEST(database_loads);
    RUN_TEST(get_x86_64);
    RUN_TEST(get_haswell);
    RUN_TEST(get_zen3);
    RUN_TEST(get_aarch64);
    RUN_TEST(get_apple_m1);
    RUN_TEST(get_nonexistent);

    // Ancestry tests
    RUN_TEST(ancestors_haswell);
    RUN_TEST(ancestors_zen4);

    // Family tests
    RUN_TEST(family_haswell);
    RUN_TEST(family_m1);
    RUN_TEST(family_power9le);

    // Generic tests
    RUN_TEST(generic_skylake);

    // Feature tests
    RUN_TEST(has_feature_avx2);
    RUN_TEST(has_feature_alias);

    // Comparison tests
    RUN_TEST(comparison_subset);
    RUN_TEST(comparison_equality);

    // Compiler flags tests
    RUN_TEST(optimization_flags_gcc);
    RUN_TEST(optimization_flags_clang);

    // Other tests
    RUN_TEST(generic_microarchitecture);
    RUN_TEST(iterate_all_targets);
    RUN_TEST(power_generation);
    RUN_TEST(arm_cpu_part);

    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
