/*
 * minunit.h - Minimal unit testing framework for C
 * https://github.com/siu/minunit
 */

#ifndef MINUNIT_H
#define MINUNIT_H

#include <stdio.h>

/* Assertions */
#define mu_assert(test, message) do { \
    if (!(test)) { \
        printf("FAILED: %s\n", message); \
        return message; \
    } \
} while (0)

#define mu_check(test) do { \
    if (!(test)) { \
        printf("FAILED: %s\n", #test); \
        return #test; \
    } \
} while (0)

/* Test runner */
static int tests_run = 0;
static int tests_passed = 0;

#define MU_TEST(name) static char* name()

#define MU_RUN_TEST(test) do { \
    char* message = test(); \
    tests_run++; \
    if (message) return message; \
    tests_passed++; \
} while (0)

#define MU_TEST_SUITE(name) static char* name()

#define MU_RUN_SUITE(suite) do { \
    char* message = suite(); \
    if (message) { \
        printf("\n❌ TEST SUITE FAILED: %s\n", message); \
        return 1; \
    } \
} while (0)

#define MU_REPORT() do { \
    printf("\n========================================\n"); \
    printf("Tests run: %d\n", tests_run); \
    printf("Tests passed: %d\n", tests_passed); \
    if (tests_run == tests_passed) { \
        printf("✅ ALL TESTS PASSED\n"); \
    } else { \
        printf("❌ %d TESTS FAILED\n", tests_run - tests_passed); \
    } \
    printf("========================================\n"); \
} while (0)

#define MU_EXIT_CODE (tests_run == tests_passed ? 0 : 1)

#endif /* MINUNIT_H */