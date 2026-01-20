/**
 * @file test_framework.h
 * @brief Simple test framework for Vista
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Colors for output */
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_RESET   "\x1b[0m"

/* Test macros */
#define TEST(name) static void test_##name(void)

#define RUN_TEST(name) do { \
    printf("  Running: %s... ", #name); \
    tests_run++; \
    test_##name(); \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf(COLOR_RED "FAILED" COLOR_RESET "\n"); \
        printf("    Assertion failed: %s\n", #condition); \
        printf("    At %s:%d\n", __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf(COLOR_RED "FAILED" COLOR_RESET "\n"); \
        printf("    Expected: %d, Got: %d\n", (int)(expected), (int)(actual)); \
        printf("    At %s:%d\n", __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_STR_EQ(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf(COLOR_RED "FAILED" COLOR_RESET "\n"); \
        printf("    Expected: \"%s\"\n", (expected)); \
        printf("    Got:      \"%s\"\n", (actual)); \
        printf("    At %s:%d\n", __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_STR_CONTAINS(haystack, needle) do { \
    if (strstr((haystack), (needle)) == NULL) { \
        printf(COLOR_RED "FAILED" COLOR_RESET "\n"); \
        printf("    String \"%s\" does not contain \"%s\"\n", (haystack), (needle)); \
        printf("    At %s:%d\n", __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_TRUE(condition) ASSERT(condition)
#define ASSERT_FALSE(condition) ASSERT(!(condition))

#define TEST_PASS() do { \
    printf(COLOR_GREEN "PASSED" COLOR_RESET "\n"); \
    tests_passed++; \
} while(0)

#define TEST_SUITE_BEGIN(name) do { \
    printf("\n" COLOR_YELLOW "=== %s ===" COLOR_RESET "\n", name); \
} while(0)

#define TEST_SUITE_END() do { \
    printf("\n----------------------------------------\n"); \
    printf("Results: "); \
    if (tests_failed == 0) { \
        printf(COLOR_GREEN "%d/%d tests passed" COLOR_RESET "\n", tests_passed, tests_run); \
    } else { \
        printf(COLOR_RED "%d/%d tests passed (%d failed)" COLOR_RESET "\n", \
               tests_passed, tests_run, tests_failed); \
    } \
    printf("----------------------------------------\n"); \
} while(0)

#define RETURN_TEST_RESULT() return (tests_failed == 0 ? 0 : 1)

#endif /* TEST_FRAMEWORK_H */
