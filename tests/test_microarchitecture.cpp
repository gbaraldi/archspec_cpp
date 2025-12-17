// This file is a part of Julia. License is MIT: https://julialang.org/license
//
// Unit tests for microarchitecture database

#include "test_common.hpp"
#include <archspec/microarchitecture.hpp>
#include <cstring>

using namespace archspec;

// Test that database loads correctly
TEST(database_loads) {
    const auto& db = MicroarchitectureDatabase::instance();
    auto names = db.all_names();
    ASSERT(names.size() > 0);
    std::cout << "(found " << names.size() << " targets) ";
    TEST_PASS();
}

// Test getting specific targets
TEST(get_x86_64) {
    auto target = get_target("x86_64");
    ASSERT(target.has_value());
    ASSERT_EQ(target->get().name(), "x86_64");
    ASSERT_EQ(target->get().vendor(), "generic");
    TEST_PASS();
}

TEST(get_haswell) {
    auto target = get_target("haswell");
    ASSERT(target.has_value());
    ASSERT_EQ(target->get().name(), "haswell");
    ASSERT_EQ(target->get().vendor(), "GenuineIntel");
    ASSERT(target->get().features().count("avx2") > 0);
    ASSERT(target->get().features().count("fma") > 0);
    TEST_PASS();
}

TEST(get_zen3) {
    auto target = get_target("zen3");
    ASSERT(target.has_value());
    ASSERT_EQ(target->get().name(), "zen3");
    ASSERT_EQ(target->get().vendor(), "AuthenticAMD");
    ASSERT(target->get().features().count("avx2") > 0);
    TEST_PASS();
}

TEST(get_aarch64) {
    auto target = get_target("aarch64");
    ASSERT(target.has_value());
    ASSERT_EQ(target->get().name(), "aarch64");
    ASSERT_EQ(target->get().vendor(), "generic");
    TEST_PASS();
}

TEST(get_apple_m1) {
    auto target = get_target("m1");
    ASSERT(target.has_value());
    ASSERT_EQ(target->get().name(), "m1");
    ASSERT_EQ(target->get().vendor(), "Apple");
    TEST_PASS();
}

TEST(get_nonexistent) {
    auto target = get_target("nonexistent_cpu_12345");
    ASSERT(!target.has_value());
    TEST_PASS();
}

// Test ancestry
TEST(ancestors_haswell) {
    auto target = get_target("haswell");
    ASSERT(target.has_value());

    auto ancestors = target->get().ancestors();
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
    TEST_PASS();
}

TEST(ancestors_zen4) {
    auto target = get_target("zen4");
    ASSERT(target.has_value());

    auto ancestors = target->get().ancestors();

    // zen4 should descend from zen3
    bool has_zen3 = false;
    for (const auto& a : ancestors) {
        if (a == "zen3") {
            has_zen3 = true;
            break;
        }
    }
    ASSERT(has_zen3);
    TEST_PASS();
}

// Test family
TEST(family_haswell) {
    auto target = get_target("haswell");
    ASSERT(target.has_value());
    ASSERT_EQ(target->get().family(), "x86_64");
    TEST_PASS();
}

TEST(family_m1) {
    auto target = get_target("m1");
    ASSERT(target.has_value());
    ASSERT_EQ(target->get().family(), "aarch64");
    TEST_PASS();
}

TEST(family_power9le) {
    auto target = get_target("power9le");
    ASSERT(target.has_value());
    ASSERT_EQ(target->get().family(), "ppc64le");
    TEST_PASS();
}

// Test generic
TEST(generic_skylake) {
    auto target = get_target("skylake");
    ASSERT(target.has_value());
    std::string gen = target->get().generic();
    // Should be x86_64_v3 or similar generic target
    ASSERT(!gen.empty());
    TEST_PASS();
}

// Test feature checking
TEST(has_feature_avx2) {
    auto target = get_target("haswell");
    ASSERT(target.has_value());
    ASSERT(target->get().has_feature("avx2"));
    ASSERT(target->get().has_feature("avx"));
    ASSERT(target->get().has_feature("sse4_1"));
    TEST_PASS();
}

TEST(has_feature_alias) {
    auto target = get_target("haswell");
    ASSERT(target.has_value());
    // sse4.1 is an alias for sse4_1
    ASSERT(target->get().has_feature("sse4.1"));
    TEST_PASS();
}

// Test comparison operators
TEST(comparison_subset) {
    auto x86_64 = get_target("x86_64");
    auto haswell = get_target("haswell");
    ASSERT(x86_64.has_value());
    ASSERT(haswell.has_value());

    // x86_64 < haswell (x86_64 is ancestor of haswell)
    ASSERT(x86_64->get() < haswell->get());
    ASSERT(x86_64->get() <= haswell->get());
    ASSERT(!(haswell->get() < x86_64->get()));
    ASSERT(haswell->get() > x86_64->get());
    TEST_PASS();
}

TEST(comparison_equality) {
    auto haswell1 = get_target("haswell");
    auto haswell2 = get_target("haswell");
    ASSERT(haswell1.has_value());
    ASSERT(haswell2.has_value());

    // Same target should be equal
    ASSERT(haswell1->get() == haswell2->get());
    ASSERT(!(haswell1->get() != haswell2->get()));
    ASSERT(haswell1->get() <= haswell2->get());
    ASSERT(haswell1->get() >= haswell2->get());
    TEST_PASS();
}

// Test compiler flags
TEST(optimization_flags_gcc) {
    auto target = get_target("haswell");
    ASSERT(target.has_value());

    std::string flags = target->get().optimization_flags("gcc", "9.0");
    ASSERT(!flags.empty());
    // Should contain -march=haswell
    ASSERT(flags.find("haswell") != std::string::npos);
    TEST_PASS();
}

TEST(optimization_flags_clang) {
    auto target = get_target("skylake");
    ASSERT(target.has_value());

    std::string flags = target->get().optimization_flags("clang", "10.0");
    ASSERT(!flags.empty());
    TEST_PASS();
}

// Test generic microarchitecture creation
TEST(generic_microarchitecture) {
    auto generic = generic_microarchitecture("test_arch");
    ASSERT_EQ(generic.name(), "test_arch");
    ASSERT_EQ(generic.vendor(), "generic");
    ASSERT(generic.features().empty());
    ASSERT(generic.parent_names().empty());
    TEST_PASS();
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
    TEST_PASS();
}

// Test Power generation
TEST(power_generation) {
    auto power9 = get_target("power9le");
    ASSERT(power9.has_value());
    ASSERT_EQ(power9->get().generation(), 9);

    auto power10 = get_target("power10le");
    ASSERT(power10.has_value());
    ASSERT_EQ(power10->get().generation(), 10);
    TEST_PASS();
}

// Test ARM CPU part
TEST(arm_cpu_part) {
    auto n1 = get_target("neoverse_n1");
    ASSERT(n1.has_value());
    ASSERT(!n1->get().cpu_part().empty());
    TEST_PASS();
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
    std::cout << "Passed: " << g_tests_passed << std::endl;
    std::cout << "Failed: " << g_tests_failed << std::endl;

    return g_tests_failed > 0 ? 1 : 0;
}
