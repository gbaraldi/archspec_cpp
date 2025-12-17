// This file is a part of Julia. License is MIT: https://julialang.org/license
//
// Common test framework without exceptions

#ifndef ARCHSPEC_TEST_COMMON_HPP
#define ARCHSPEC_TEST_COMMON_HPP

#include <iostream>
#include <optional>
#include <string>

// Simple test framework without exceptions
// Tests return std::optional<std::string> - nullopt on success, error message on failure
inline int g_tests_passed = 0;
inline int g_tests_failed = 0;

using TestResult = std::optional<std::string>;

#define TEST(name) TestResult test_##name()

#define RUN_TEST(name)                                    \
    do {                                                  \
        std::cout << "Running " #name "... ";             \
        TestResult result = test_##name();                \
        if (!result.has_value()) {                        \
            std::cout << "PASSED" << std::endl;           \
            g_tests_passed++;                             \
        } else {                                          \
            std::cout << "FAILED: " << *result << std::endl; \
            g_tests_failed++;                             \
        }                                                 \
    } while (0)

#define TEST_PASS() return std::nullopt
#define TEST_FAIL(msg) return std::string(msg)

#define ASSERT(cond)                                \
    do {                                            \
        if (!(cond)) {                              \
            return "Assertion failed: " #cond;      \
        }                                           \
    } while (0)

#define ASSERT_EQ(a, b)                                  \
    do {                                                 \
        if ((a) != (b)) {                                \
            return "Assertion failed: " #a " == " #b;    \
        }                                                \
    } while (0)

#endif // ARCHSPEC_TEST_COMMON_HPP
