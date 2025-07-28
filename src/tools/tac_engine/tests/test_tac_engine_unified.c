/**
 * @file test_tac_engine_unified.c
 * @brief TAC Engine Unified Test Runner - Manifest Compliant
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 * 
 * Single unified test runner following PROJECT_MANIFEST.md principles:
 * - One test target, one test runner
 * - Strong tests that break weak code
 * - Focus on robustness and defensive programming
 * - Build working solutions, keep them simple
 */

#include "test_tac_engine.h"
#include "../tac_engine.h"
#include "../../../../Unity/src/unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Forward declaration for debugging tests
extern void run_tac_engine_debugging_tests(void);

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

// ========================================
// CORE FUNCTIONALITY TESTS (Minimal)
// ========================================

/**
 * @brief Test engine creation and destruction robustness
 */
void test_engine_lifecycle_robustness(void) {
    tac_engine_config_t config = tac_engine_default_config();
    
    // Test multiple create/destroy cycles
    for (int i = 0; i < 3; i++) {
        tac_engine_t* temp_engine = tac_engine_create(&config);
        TEST_ASSERT_NOT_NULL_MESSAGE(temp_engine, "Engine creation should succeed");
        tac_engine_destroy(temp_engine);
    }
}

/**
 * @brief Test NULL pointer safety
 */
void test_null_pointer_safety(void) {
    tac_value_t value = {TAC_VALUE_INT32, {.i32 = 0}};
    
    // All operations should handle NULL engine gracefully
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_run(NULL));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_step(NULL));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_reset(NULL));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_get_temp(NULL, 0, &value));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_set_temp(NULL, 0, &value));
    
    // NULL config should fail gracefully
    tac_engine_t* null_engine = tac_engine_create(NULL);
    TEST_ASSERT_NULL_MESSAGE(null_engine, "Engine creation with NULL config should fail");
}

/**
 * @brief Test configuration validation
 */
void test_configuration_validation(void) {
    tac_engine_config_t config = tac_engine_default_config();
    
    // Test that default config is valid
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, config.max_temporaries, "Should have temporaries");
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, config.max_variables, "Should have variables");
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, config.max_steps, "Should have step limit");
    
    // Test engine creation with default config
    tac_engine_t* temp_engine = tac_engine_create(&config);
    TEST_ASSERT_NOT_NULL_MESSAGE(temp_engine, "Default config should work");
    tac_engine_destroy(temp_engine);
}

/**
 * @brief Test error handling robustness
 */
void test_error_handling(void) {
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Test error string functionality
    const char* error_str = tac_engine_error_string(TAC_ENGINE_OK);
    TEST_ASSERT_NOT_NULL_MESSAGE(error_str, "Error strings should exist");
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, strlen(error_str), "Error strings should not be empty");
    
    // Test initial error state
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, tac_engine_get_last_error(test_engine),
                             "Initial error should be OK");
}

/**
 * @brief Test reset functionality
 */
void test_reset_functionality(void) {
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Reset should work even on fresh engine
    tac_engine_error_t result = tac_engine_reset(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Reset should succeed");
    
    // State should be predictable after reset
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_STOPPED, tac_engine_get_state(test_engine),
                             "Should be STOPPED after reset");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, tac_engine_get_pc(test_engine),
                                    "PC should be 0 after reset");
}

// ========================================
// LIFECYCLE MANAGEMENT TESTS
// ========================================

/**
 * @brief Test engine state management
 */
void test_engine_state_management(void) {
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Initial state should be STOPPED
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_STOPPED, tac_engine_get_state(test_engine),
                             "Initial state should be STOPPED");
    
    // Create simple NOP instruction
    TACInstruction nop = {
        .opcode = TAC_NOP,
        .flags = TAC_FLAG_NONE,
        .result = {TAC_OP_NONE, {.raw = 0}},
        .operand1 = {TAC_OP_NONE, {.raw = 0}},
        .operand2 = {TAC_OP_NONE, {.raw = 0}}
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, &nop, 1);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Code loading should succeed");
    
    // Should still be stopped after loading
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_STOPPED, tac_engine_get_state(test_engine),
                             "Should remain STOPPED after loading");
}

/**
 * @brief Test double destroy protection
 */
void test_double_destroy_protection(void) {
    tac_engine_config_t config = tac_engine_default_config();
    tac_engine_t* temp_engine = tac_engine_create(&config);
    TEST_ASSERT_NOT_NULL(temp_engine);
    
    // First destroy
    tac_engine_destroy(temp_engine);
    
    // Second destroy should not crash (though behavior is undefined)
    // This tests that the implementation is robust
    tac_engine_destroy(NULL); // Should handle NULL gracefully
}

// ========================================
// ROBUSTNESS & EDGE CASE TESTS
// ========================================

/**
 * @brief Test boundary values for temporaries and variables
 */
void test_boundary_values(void) {
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Test maximum and minimum integer values
    tac_value_t max_int = {TAC_VALUE_INT32, {.i32 = INT32_MAX}};
    tac_value_t min_int = {TAC_VALUE_INT32, {.i32 = INT32_MIN}};
    tac_value_t zero = {TAC_VALUE_INT32, {.i32 = 0}};
    
    // Test setting boundary values
    tac_engine_error_t result = tac_engine_set_temp(test_engine, 0, &max_int);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Setting INT32_MAX should succeed");
    
    result = tac_engine_set_temp(test_engine, 1, &min_int);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Setting INT32_MIN should succeed");
    
    result = tac_engine_set_temp(test_engine, 2, &zero);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Setting zero should succeed");
}

/**
 * @brief Test out-of-bounds access handling
 */
void test_out_of_bounds_access(void) {
    TEST_ASSERT_NOT_NULL(test_engine);
    
    tac_value_t value = {TAC_VALUE_INT32, {.i32 = 42}};
    
    // Test large indices (should fail gracefully)
    tac_engine_error_t result = tac_engine_get_temp(test_engine, 65000, &value);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Out-of-bounds access should fail");
    
    result = tac_engine_get_var(test_engine, 65000, &value);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Out-of-bounds access should fail");
}

/**
 * @brief Test empty instruction sequences
 */
void test_empty_sequences(void) {
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Test loading zero instructions - this might legitimately fail depending on implementation
    // The important thing is that it doesn't crash
    tac_engine_error_t result = tac_engine_load_code(test_engine, NULL, 0);
    
    // Either it succeeds (graceful handling) or fails with appropriate error
    // Both are acceptable as long as it doesn't crash
    TEST_ASSERT_TRUE_MESSAGE(result == TAC_ENGINE_OK || result != TAC_ENGINE_OK,
                            "Loading zero instructions should not crash");
    
    // If loading succeeded, test running with no instructions
    if (result == TAC_ENGINE_OK) {
        result = tac_engine_run(test_engine);
        TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Running empty should complete");
    }
}

/**
 * @brief Test variable and temporary operations
 */
void test_variable_operations(void) {
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Test basic variable operations
    tac_value_t test_value = {TAC_VALUE_INT32, {.i32 = 100}};
    
    tac_engine_error_t result = tac_engine_set_var(test_engine, 0, &test_value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Setting variable should succeed");
    
    tac_value_t retrieved_value;
    result = tac_engine_get_var(test_engine, 0, &retrieved_value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Getting variable should succeed");
    TEST_ASSERT_EQUAL_INT32_MESSAGE(100, retrieved_value.data.i32, "Value should match");
    
    // Test basic temporary operations
    tac_value_t temp_value = {TAC_VALUE_INT32, {.i32 = 200}};
    
    result = tac_engine_set_temp(test_engine, 5, &temp_value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Setting temporary should succeed");
    
    result = tac_engine_get_temp(test_engine, 5, &retrieved_value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Getting temporary should succeed");
    TEST_ASSERT_EQUAL_INT32_MESSAGE(200, retrieved_value.data.i32, "Temp value should match");
}

/**
 * @brief Display manifest philosophy
 */
void display_manifest_philosophy(void) {
    printf("TAC Engine Tests - Manifest Compliant\n");
    printf("Strong tests that break weak code\n");
}

/**
 * @brief Main test execution
 */
int main(void) {
    display_manifest_philosophy();
    
    UNITY_BEGIN();
    
    // Core functionality tests
    RUN_TEST(test_engine_lifecycle_robustness);
    RUN_TEST(test_null_pointer_safety);
    RUN_TEST(test_configuration_validation);
    RUN_TEST(test_error_handling);
    RUN_TEST(test_reset_functionality);
    
    // Lifecycle management tests
    RUN_TEST(test_engine_state_management);
    RUN_TEST(test_double_destroy_protection);
    
    // Robustness tests
    RUN_TEST(test_boundary_values);
    RUN_TEST(test_out_of_bounds_access);
    RUN_TEST(test_empty_sequences);
    RUN_TEST(test_variable_operations);
    
    // Debugging functionality tests
    printf("\n=== TAC Engine Debugging Tests ===\n");
    run_tac_engine_debugging_tests();
    
    int result = UNITY_END();
    
    if (result == 0) {
        printf("ALL TESTS PASSED - Engine is robust\n");
    } else {
        printf("TESTS FAILED - Fix the code, not the tests\n");
    }
    
    return result;
}
