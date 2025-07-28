/**
 * @file test_tac_engine_main.c
 * @brief TAC Engine Test Suite Main Runner
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 * @copyright Copyright (c) 2025
 * 
 * Main test runner for the TAC Engine test suite.
 * This comprehensive test suite is designed to break weak implementations
 * and expose edge cases, following PROJECT_MANIFEST.md principles.
 */

#include "test_tac_engine.h"
#include "../../../Unity/src/unity.h"
#include <stdio.h>
#include <stdlib.h>

// Test suite runners (defined in separate files)
extern void run_tac_engine_lifecycle_tests(void);
extern void run_tac_engine_execution_tests(void);
extern void run_tac_engine_debugging_tests(void);
extern void run_tac_engine_edge_case_tests(void);
extern void run_tac_engine_stress_tests(void);

// Unity setup/teardown
void setUp(void) {
    // Global setup if needed
}

void tearDown(void) {
    // Global teardown if needed
}

/**
 * @brief Print test suite header
 */
static void print_test_header(void) {
    printf("\n");
    printf("========================================\n");
    printf("    TAC ENGINE COMPREHENSIVE TEST SUITE\n");
    printf("========================================\n");
    printf("\n");
    printf("Following PROJECT_MANIFEST.md principles:\n");
    printf("- NEVER weaken a test to make it pass\n");
    printf("- FIX the code, not the test\n");
    printf("- Create strong tests that break weak code\n");
    printf("- Test edge cases aggressively\n");
    printf("\n");
    printf("Test Categories:\n");
    printf("1. Lifecycle Tests - Engine creation/destruction\n");
    printf("2. Execution Tests - TAC instruction execution\n");
    printf("3. Debugging Tests - Breakpoints, hooks, tracing\n");
    printf("4. Edge Case Tests - Boundary conditions, invalid inputs\n");
    printf("5. Stress Tests - Resource exhaustion, malformed data\n");
    printf("\n");
}

/**
 * @brief Print test results summary
 */
static void print_test_summary(void) {
    printf("\n");
    printf("========================================\n");
    printf("         TEST SUITE COMPLETED\n");
    printf("========================================\n");
    
    // Unity provides Unity.TestFailures for failure count
    if (Unity.TestFailures == 0) {
        printf("✅ ALL TESTS PASSED!\n");
        printf("The TAC Engine implementation is robust.\n");
    } else {
        printf("❌ %d TEST(S) FAILED!\n", Unity.TestFailures);
        printf("The implementation needs fixes.\n");
        printf("Remember: FIX THE CODE, NOT THE TEST!\n");
    }
    
    printf("\nTotal Tests Run: %d\n", Unity.NumberOfTests);
    printf("Failures: %d\n", Unity.TestFailures);
    printf("Ignored: %d\n", Unity.TestIgnores);
    printf("\n");
}

/**
 * @brief Run specific test category based on command line argument
 */
static int run_specific_tests(const char* category) {
    UNITY_BEGIN();
    
    if (strcmp(category, "lifecycle") == 0) {
        printf("Running only Lifecycle Tests...\n");
        run_tac_engine_lifecycle_tests();
    } else if (strcmp(category, "execution") == 0) {
        printf("Running only Execution Tests...\n");
        run_tac_engine_execution_tests();
    } else if (strcmp(category, "debugging") == 0) {
        printf("Running only Debugging Tests...\n");
        run_tac_engine_debugging_tests();
    } else if (strcmp(category, "edge") == 0) {
        printf("Running only Edge Case Tests...\n");
        run_tac_engine_edge_case_tests();
    } else if (strcmp(category, "stress") == 0) {
        printf("Running only Stress Tests...\n");
        run_tac_engine_stress_tests();
    } else {
        printf("Unknown test category: %s\n", category);
        printf("Valid categories: lifecycle, execution, debugging, edge, stress\n");
        return 1;
    }
    
    return UNITY_END();
}

/**
 * @brief Run all test suites
 */
static int run_all_tests(void) {
    UNITY_BEGIN();
    
    // Run all test suites in order
    run_tac_engine_lifecycle_tests();
    run_tac_engine_execution_tests();
    run_tac_engine_debugging_tests();
    run_tac_engine_edge_case_tests();
    run_tac_engine_stress_tests();
    
    return UNITY_END();
}

/**
 * @brief Main test runner entry point
 */
int main(int argc, char* argv[]) {
    print_test_header();
    
    int result;
    
    if (argc > 1) {
        // Run specific test category
        result = run_specific_tests(argv[1]);
    } else {
        // Run all tests
        result = run_all_tests();
    }
    
    print_test_summary();
    
    // Return non-zero if any tests failed
    return result;
}

// =============================================================================
// UNITY INTEGRATION EXAMPLES FOR OTHER TEST FILES
// =============================================================================

/**
 * @brief Example of how to use TAC Engine in Unity tests
 * 
 * This function demonstrates the recommended patterns for testing
 * TAC code execution using the Unity framework.
 */
void example_unity_test_patterns(void) {
    // This is an example function - not run as part of the test suite
    // It shows recommended patterns for Unity integration
    
#if 0  // Disabled - just for documentation
    
    // PATTERN 1: Basic TAC execution test
    void test_simple_arithmetic(void) {
        tac_engine_config_t config = TAC_ENGINE_DEFAULT_CONFIG;
        tac_engine_t* engine = tac_engine_open(&config);
        TEST_ASSERT_NOT_NULL(engine);
        
        TACInstruction instructions[] = {
            // t0 = 5 + 3
            {.op = TAC_OP_ASSIGN, .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
             .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 5}}}},
            {.op = TAC_OP_ASSIGN, .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
             .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 3}}}},
            {.op = TAC_OP_ADD, .result = {.type = TAC_OPERAND_TEMPORARY, .id = 2},
             .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
             .operand2 = {.type = TAC_OPERAND_TEMPORARY, .id = 1}}
        };
        
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, tac_engine_load_code(engine, instructions, 3));
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, tac_engine_start(engine, 0));
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, tac_engine_run(engine, 10));
        
        tac_value_t result;
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, tac_engine_get_temporary(engine, 2, &result));
        TEST_ASSERT_EQUAL_INT32(8, result.value.int_val);
        
        tac_engine_close(engine);
    }
    
    // PATTERN 2: Error condition testing
    void test_division_by_zero(void) {
        tac_engine_config_t config = TAC_ENGINE_DEFAULT_CONFIG;
        tac_engine_t* engine = tac_engine_open(&config);
        TEST_ASSERT_NOT_NULL(engine);
        
        TACInstruction instructions[] = {
            {.op = TAC_OP_ASSIGN, .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
             .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 42}}}},
            {.op = TAC_OP_ASSIGN, .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
             .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 0}}}},
            {.op = TAC_OP_DIV, .result = {.type = TAC_OPERAND_TEMPORARY, .id = 2},
             .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
             .operand2 = {.type = TAC_OPERAND_TEMPORARY, .id = 1}}
        };
        
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, tac_engine_load_code(engine, instructions, 3));
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, tac_engine_start(engine, 0));
        
        // First two instructions should succeed
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, tac_engine_step(engine));
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, tac_engine_step(engine));
        
        // Division should fail
        TEST_ASSERT_EQUAL(TAC_ENGINE_ERROR_DIVISION_BY_ZERO, tac_engine_step(engine));
        
        tac_engine_close(engine);
    }
    
    // PATTERN 3: Debugging feature testing
    void test_breakpoint_functionality(void) {
        tac_engine_config_t config = TAC_ENGINE_DEFAULT_CONFIG;
        tac_engine_t* engine = tac_engine_open(&config);
        TEST_ASSERT_NOT_NULL(engine);
        
        TACInstruction instructions[] = {
            {.op = TAC_OP_ASSIGN, .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
             .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 1}}}},
            {.op = TAC_OP_ASSIGN, .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
             .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 2}}}}
        };
        
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, tac_engine_load_code(engine, instructions, 2));
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, tac_engine_add_breakpoint(engine, 1));
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, tac_engine_start(engine, 0));
        
        // Run should hit breakpoint
        TEST_ASSERT_EQUAL(TAC_ENGINE_ERROR_BREAKPOINT_HIT, tac_engine_run(engine, 10));
        TEST_ASSERT_EQUAL(TAC_ENGINE_STATE_PAUSED, tac_engine_get_state(engine));
        TEST_ASSERT_EQUAL_UINT32(1, tac_engine_get_pc(engine));
        
        tac_engine_close(engine);
    }
    
#endif
}
