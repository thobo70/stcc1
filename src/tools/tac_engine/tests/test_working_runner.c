/**
 * @file test_working_runner.c
 * @brief Simple test runner for working TAC Engine tests
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 * @copyright Copyright (c) 2025
 */

#include "test_tac_engine.h"
#include "../../../Unity/src/unity.h"
#include <stdio.h>

// Unity setup/teardown
void setUp(void) {
    // Global setup if needed
}

void tearDown(void) {
    // Global teardown if needed
}

// Include the working test functions
extern void run_tac_engine_working_tests(void);

int main(void) {
    printf("\n========================================\n");
    printf("    TAC ENGINE WORKING TEST SUITE\n");
    printf("========================================\n");
    printf("Demonstrating functional test framework\n");
    printf("with strong tests that break weak code.\n");
    printf("========================================\n\n");
    
    UNITY_BEGIN();
    
    run_tac_engine_working_tests();
    
    int result = UNITY_END();
    
    printf("\n========================================\n");
    if (result == 0) {
        printf("✅ ALL WORKING TESTS PASSED!\n");
        printf("TAC Engine test framework is functional\n");
    } else {
        printf("❌ SOME TESTS FAILED\n");
        printf("Framework needs fixes\n");
    }
    printf("========================================\n\n");
    
    return result;
}
