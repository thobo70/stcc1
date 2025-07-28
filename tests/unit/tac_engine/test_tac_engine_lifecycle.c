/**
 * @file test_tac_engine_lifecycle.c
 * @brief TAC Engine Lifecycle Tests
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 * @copyright Copyright (c) 2025
 * 
 * Tests for TAC Engine creation, configuration, and destruction.
 * These tests verify the basic lifecycle of the engine and ensure
 * proper resource management.
 */

#include "test_tac_engine.h"
#include "../../../src/tools/tac_engine/tac_engine.h"
#include "../../test_common.h"
#include <string.h>
#include <limits.h>

// Global test engine instance
static tac_engine_t* test_engine = NULL;

void setUp(void) {
    test_engine = NULL;
}

void tearDown(void) {
    if (test_engine) {
        tac_engine_close(test_engine);
        test_engine = NULL;
    }
}

/**
 * @brief Test basic engine creation and destruction
 */
void test_tac_engine_create_destroy(void) {
    tac_engine_config_t config = TAC_ENGINE_DEFAULT_CONFIG;
    
    // Test successful creation
    test_engine = tac_engine_open(&config);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_engine, "Engine creation should succeed with valid config");
    
    // Verify initial state
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_STATE_STOPPED, tac_engine_get_state(test_engine),
                             "Initial engine state should be STOPPED");
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, tac_engine_get_last_error(test_engine),
                             "Initial error state should be OK");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, tac_engine_get_pc(test_engine),
                                    "Initial program counter should be 0");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, tac_engine_get_step_count(test_engine),
                                    "Initial step count should be 0");
    
    // Test destruction
    tac_engine_close(test_engine);
    test_engine = NULL; // Prevent double-free in tearDown
}

/**
 * @brief Test engine creation with NULL config
 */
void test_tac_engine_null_config(void) {
    test_engine = tac_engine_open(NULL);
    TEST_ASSERT_NULL_MESSAGE(test_engine, "Engine creation should fail with NULL config");
}

/**
 * @brief Test engine operations with NULL engine
 */
void test_tac_engine_null_engine_operations(void) {
    tac_value_t value = {0};
    TACInstruction instruction = {0};
    
    // All operations should handle NULL engine gracefully
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERROR_INVALID_PARAM, tac_engine_load_code(NULL, &instruction, 1));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERROR_INVALID_PARAM, tac_engine_start(NULL, 0));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERROR_INVALID_PARAM, tac_engine_step(NULL));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERROR_INVALID_PARAM, tac_engine_stop(NULL));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERROR_INVALID_PARAM, tac_engine_reset(NULL));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERROR_INVALID_PARAM, tac_engine_get_variable(NULL, 0, &value));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERROR_INVALID_PARAM, tac_engine_set_variable(NULL, 0, &value));
    
    // State queries should return safe defaults
    TEST_ASSERT_EQUAL(TAC_ENGINE_STATE_ERROR, tac_engine_get_state(NULL));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERROR_INVALID_PARAM, tac_engine_get_last_error(NULL));
    TEST_ASSERT_EQUAL_UINT32(0, tac_engine_get_pc(NULL));
    TEST_ASSERT_EQUAL_UINT32(0, tac_engine_get_step_count(NULL));
}

/**
 * @brief Test engine with invalid configurations
 */
void test_tac_engine_invalid_config(void) {
    tac_engine_config_t config;
    
    // Test with zero temporaries
    config = TAC_ENGINE_DEFAULT_CONFIG;
    config.max_temporaries = 0;
    test_engine = tac_engine_open(&config);
    TEST_ASSERT_NULL_MESSAGE(test_engine, "Engine should reject zero temporaries");
    
    // Test with zero variables
    config = TAC_ENGINE_DEFAULT_CONFIG;
    config.max_variables = 0;
    test_engine = tac_engine_open(&config);
    TEST_ASSERT_NULL_MESSAGE(test_engine, "Engine should reject zero variables");
    
    // Test with zero memory
    config = TAC_ENGINE_DEFAULT_CONFIG;
    config.max_memory_size = 0;
    test_engine = tac_engine_open(&config);
    TEST_ASSERT_NULL_MESSAGE(test_engine, "Engine should reject zero memory");
    
    // Test with excessive values (should handle gracefully or reject)
    config = TAC_ENGINE_DEFAULT_CONFIG;
    config.max_temporaries = UINT32_MAX;
    config.max_variables = UINT32_MAX;
    config.max_memory_size = UINT32_MAX;
    test_engine = tac_engine_open(&config);
    // This may succeed or fail depending on available memory - either is acceptable
    if (test_engine) {
        tac_engine_close(test_engine);
        test_engine = NULL;
    }
}

/**
 * @brief Test multiple engine instances
 */
void test_tac_engine_multiple_instances(void) {
    tac_engine_config_t config = TAC_ENGINE_DEFAULT_CONFIG;
    tac_engine_t* engine1 = NULL;
    tac_engine_t* engine2 = NULL;
    tac_engine_t* engine3 = NULL;
    
    // Create multiple engines
    engine1 = tac_engine_open(&config);
    engine2 = tac_engine_open(&config);
    engine3 = tac_engine_open(&config);
    
    TEST_ASSERT_NOT_NULL_MESSAGE(engine1, "First engine should be created successfully");
    TEST_ASSERT_NOT_NULL_MESSAGE(engine2, "Second engine should be created successfully");
    TEST_ASSERT_NOT_NULL_MESSAGE(engine3, "Third engine should be created successfully");
    
    // Verify they are independent
    TEST_ASSERT_NOT_EQUAL_MESSAGE(engine1, engine2, "Engines should be different instances");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(engine1, engine3, "Engines should be different instances");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(engine2, engine3, "Engines should be different instances");
    
    // Test that operations on one don't affect others
    tac_engine_error_t result1 = tac_engine_start(engine1, 0);
    tac_engine_error_t result2 = tac_engine_start(engine2, 0);
    
    // Both should fail (no code loaded) but independently
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERROR_INVALID_STATE, result1);
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERROR_INVALID_STATE, result2);
    
    // Cleanup
    tac_engine_close(engine1);
    tac_engine_close(engine2);
    tac_engine_close(engine3);
}

/**
 * @brief Test engine destruction edge cases
 */
void test_tac_engine_destruction_edge_cases(void) {
    // Double destruction should be safe
    tac_engine_close(NULL);
    
    // Create and destroy multiple times
    for (int i = 0; i < 3; i++) {
        tac_engine_config_t config = TAC_ENGINE_DEFAULT_CONFIG;
        tac_engine_t* engine = tac_engine_open(&config);
        TEST_ASSERT_NOT_NULL(engine);
        tac_engine_close(engine);
    }
}

/**
 * @brief Test engine with minimal configuration
 */
void test_tac_engine_minimal_config(void) {
    tac_engine_config_t config = {
        .max_temporaries = 1,
        .max_variables = 1,
        .max_memory_size = 1024,
        .max_call_depth = 1,
        .max_steps = 0,
        .enable_tracing = false,
        .max_trace_entries = 0
    };
    
    test_engine = tac_engine_open(&config);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_engine, "Engine should work with minimal config");
    
    // Verify it actually works with minimal resources
    TEST_ASSERT_EQUAL(TAC_ENGINE_STATE_STOPPED, tac_engine_get_state(test_engine));
    
    tac_engine_close(test_engine);
    test_engine = NULL;
}

/**
 * @brief Run all lifecycle tests
 */
void run_tac_engine_lifecycle_tests(void) {
    printf("Running TAC Engine Lifecycle Tests...\n");
    
    RUN_TEST(test_tac_engine_create_destroy);
    RUN_TEST(test_tac_engine_null_config);
    RUN_TEST(test_tac_engine_null_engine_operations);
    RUN_TEST(test_tac_engine_invalid_config);
    RUN_TEST(test_tac_engine_multiple_instances);
    RUN_TEST(test_tac_engine_destruction_edge_cases);
    RUN_TEST(test_tac_engine_minimal_config);
    
    printf("TAC Engine Lifecycle Tests completed.\n");
}
