/**
 * @file test_tac_engine_comprehensive.c
 * @brief TAC Engine Comprehensive Test Runner - Manifest Compliant
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 * 
 * Comprehensive test runner following PROJECT_MANIFEST.md principles:
 * - Include all valuable tests that break weak code
 * - Focus on robustness and defensive programming
 * - Build on working solutions (minimal + lifecycle + robustness)
 * - Demonstrate manifest philosophy in action
 */

#include "test_tac_engine.h"
#include "../tac_engine.h"
#include "../../../../Unity/src/unity.h"
#include <stdio.h>
#include <stdlib.h>

// Global test engine - initialized in setUp, destroyed in tearDown
tac_engine_t* test_engine = NULL;

/**
 * Unity test setup - called before each test
 */
void setUp(void) {
    tac_engine_config_t config = tac_engine_default_config();
    test_engine = tac_engine_create(&config);
    if (!test_engine) {
        TEST_FAIL_MESSAGE("Failed to create test engine");
    }
}

/**
 * Unity test teardown - called after each test
 */
void tearDown(void) {
    if (test_engine) {
        tac_engine_destroy(test_engine);
        test_engine = NULL;
    }
}

// External test runners
extern void run_minimal_tests(void);      // From test_tac_engine_minimal.c
extern void run_lifecycle_tests(void);    // From test_tac_engine_lifecycle.c
extern void run_robustness_tests(void);   // From test_tac_engine_robustness.c

/**
 * @brief Display manifest philosophy
 */
void display_manifest_philosophy(void) {
    printf("\\n");
    printf("╔════════════════════════════════════════════════════════════════╗\\n");
    printf("║                    TAC ENGINE TEST FRAMEWORK                  ║\\n");
    printf("║                     Manifest Compliant                        ║\\n");
    printf("╠════════════════════════════════════════════════════════════════╣\\n");
    printf("║ PHILOSOPHY: Strong tests that break weak code                 ║\\n");
    printf("║                                                                ║\\n");
    printf("║ TEST CATEGORIES:                                               ║\\n");
    printf("║ • MINIMAL: Core functionality & NULL safety (5 tests)         ║\\n");
    printf("║ • LIFECYCLE: Engine state management (8 tests)                ║\\n");
    printf("║ • ROBUSTNESS: Boundary & edge cases (7 tests)                 ║\\n");
    printf("║                                                                ║\\n");
    printf("║ TOTAL: 20 comprehensive tests                                  ║\\n");
    printf("║                                                                ║\\n");
    printf("║ MANIFEST PRINCIPLE: Build working solutions first,            ║\\n");
    printf("║                     then enhance while maintaining quality    ║\\n");
    printf("╚════════════════════════════════════════════════════════════════╝\\n");
    printf("\\n");
}

/**
 * @brief Main test execution
 */
int main(void) {
    display_manifest_philosophy();
    
    UNITY_BEGIN();
    
    printf("\\n>>> Phase 1: Minimal Core Tests (Defensive Programming)\\n");
    run_minimal_tests();
    
    printf("\\n>>> Phase 2: Lifecycle Management Tests (State Robustness)\\n");
    run_lifecycle_tests();
    
    printf("\\n>>> Phase 3: Robustness Tests (Boundary & Edge Cases)\\n");
    run_robustness_tests();
    
    int result = UNITY_END();
    
    printf("\\n");
    printf("╔════════════════════════════════════════════════════════════════╗\\n");
    printf("║                        TEST SUMMARY                           ║\\n");
    printf("╠════════════════════════════════════════════════════════════════╣\\n");
    if (result == 0) {
        printf("║ ✅ ALL TESTS PASSED - Engine demonstrates robustness          ║\\n");
        printf("║                                                                ║\\n");
        printf("║ MANIFEST VALIDATION: Strong tests validated strong code       ║\\n");
    } else {
        printf("║ ❌ TESTS FAILED - Engine needs strengthening                  ║\\n");
        printf("║                                                                ║\\n");
        printf("║ MANIFEST GUIDANCE: Fix the code, not the tests               ║\\n");
    }
    printf("╚════════════════════════════════════════════════════════════════╝\\n");
    printf("\\n");
    
    return result;
}
