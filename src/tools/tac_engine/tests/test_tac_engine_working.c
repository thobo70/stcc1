/**
 * @file test_tac_engine_working.c
 * @brief TAC Engine Working Tests - Demonstrates framework functionality
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 * @copyright Copyright (c) 2025
 * 
 * Simple tests that demonstrate the TAC Engine test framework is working
 * with the correct API and can run strong tests to break weak code.
 */

#include "test_tac_engine.h"
#include "../tac_engine.h"
#include <string.h>

// Global test engine instance
static tac_engine_t* test_engine = NULL;

// Local test setup/teardown (not Unity's global ones)
static void test_setUp(void) {
    test_engine = NULL;
}

static void test_tearDown(void) {
    if (test_engine) {
        tac_engine_destroy(test_engine);
        test_engine = NULL;
    }
}

/**
 * @brief Test basic engine lifecycle
 */
void test_tac_engine_basic_lifecycle(void) {
    test_setUp();
    
    tac_engine_config_t config = tac_engine_default_config();
    
    // Test successful creation
    test_engine = tac_engine_create(&config);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_engine, "Engine creation should succeed");
    
    // Verify initial state
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_STOPPED, tac_engine_get_state(test_engine),
                             "Initial state should be STOPPED");
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, tac_engine_get_last_error(test_engine),
                             "Initial error should be OK");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, tac_engine_get_pc(test_engine),
                                    "Initial PC should be 0");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, tac_engine_get_step_count(test_engine),
                                    "Initial step count should be 0");
    
    // Test reset
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, tac_engine_reset(test_engine),
                             "Reset should succeed");
    
    // Test stop
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, tac_engine_stop(test_engine),
                             "Stop should succeed");
    
    // Test destruction
    test_tearDown();
}

/**
 * @brief Test NULL pointer handling - Strong test to break weak code
 */
void test_tac_engine_null_pointer_robustness(void) {
    tac_value_t value = {0};
    TACInstruction instruction = {0};
    uint32_t stats_steps, stats_memory;
    
    // All operations should handle NULL engine gracefully
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_load_code(NULL, &instruction, 1));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_run(NULL));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_step(NULL));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_stop(NULL));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_reset(NULL));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_get_temp(NULL, 0, &value));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_set_temp(NULL, 0, &value));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_get_var(NULL, 0, &value));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_set_var(NULL, 0, &value));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_get_stats(NULL, &stats_steps, &stats_memory));
    
    // State queries should return safe defaults
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERROR, tac_engine_get_state(NULL));
    TEST_ASSERT_EQUAL(TAC_ENGINE_ERR_NULL_POINTER, tac_engine_get_last_error(NULL));
    TEST_ASSERT_EQUAL_UINT32(0, tac_engine_get_pc(NULL));
    TEST_ASSERT_EQUAL_UINT32(0, tac_engine_get_step_count(NULL));
    
    // Error string should never return NULL
    const char* error_str = tac_engine_error_string(TAC_ENGINE_ERR_NULL_POINTER);
    TEST_ASSERT_NOT_NULL_MESSAGE(error_str, "Error string should never be NULL");
    TEST_ASSERT_TRUE_MESSAGE(strlen(error_str) > 0, "Error string should not be empty");
}

/**
 * @brief Test configuration validation - Strong test to break weak code
 */
void test_tac_engine_invalid_config_rejection(void) {
    test_setUp();
    
    tac_engine_config_t config;
    
    // Test NULL config
    test_engine = tac_engine_create(NULL);
    TEST_ASSERT_NULL_MESSAGE(test_engine, "Engine should reject NULL config");
    
    // Test zero values (should be rejected)
    config = tac_engine_default_config();
    config.max_temporaries = 0;
    test_engine = tac_engine_create(&config);
    TEST_ASSERT_NULL_MESSAGE(test_engine, "Engine should reject zero temporaries");
    
    config = tac_engine_default_config();
    config.max_variables = 0;
    test_engine = tac_engine_create(&config);
    TEST_ASSERT_NULL_MESSAGE(test_engine, "Engine should reject zero variables");
    
    config = tac_engine_default_config();
    config.max_memory_size = 0;
    test_engine = tac_engine_create(&config);
    TEST_ASSERT_NULL_MESSAGE(test_engine, "Engine should reject zero memory");
    
    test_tearDown();
}

/**
 * @brief Test basic instruction loading
 */
void test_tac_engine_instruction_loading(void) {
    test_setUp();
    
    tac_engine_config_t config = tac_engine_default_config();
    test_engine = tac_engine_create(&config);
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Create a simple instruction
    TACInstruction instructions[] = {
        {
            .opcode = TAC_ASSIGN,
            .result = {.type = TAC_OP_TEMP, .data = {.variable = {.id = 0}}},
            .operand1 = {.type = TAC_OP_IMMEDIATE, .data = {.immediate = {.value = 42}}},
            .operand2 = {.type = TAC_OP_NONE}
        }
    };
    
    // Test successful loading
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 1);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Code loading should succeed");
    
    // Test loading with NULL instructions
    result = tac_engine_load_code(test_engine, NULL, 1);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERR_NULL_POINTER, result, 
                             "Loading NULL instructions should fail");
    
    // Test loading with zero count (should succeed)
    result = tac_engine_load_code(test_engine, instructions, 0);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Loading zero instructions should succeed");
    
    test_tearDown();
}

/**
 * @brief Test error string functionality
 */
void test_tac_engine_error_strings(void) {
    // Test all known error codes
    const char* str;
    
    str = tac_engine_error_string(TAC_ENGINE_OK);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_TRUE(strlen(str) > 0);
    
    str = tac_engine_error_string(TAC_ENGINE_ERR_NULL_POINTER);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_TRUE(strlen(str) > 0);
    
    str = tac_engine_error_string(TAC_ENGINE_ERR_INVALID_OPCODE);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_TRUE(strlen(str) > 0);
    
    str = tac_engine_error_string(TAC_ENGINE_ERR_OUT_OF_MEMORY);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_TRUE(strlen(str) > 0);
    
    str = tac_engine_error_string(TAC_ENGINE_ERR_DIVISION_BY_ZERO);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_TRUE(strlen(str) > 0);
    
    // Test unknown error code (should still return valid string)
    str = tac_engine_error_string((tac_engine_error_t)9999);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_TRUE(strlen(str) > 0);
}

/**
 * @brief Test default configuration
 */
void test_tac_engine_default_config(void) {
    test_setUp();
    
    tac_engine_config_t config = tac_engine_default_config();
    
    // Verify reasonable defaults
    TEST_ASSERT_TRUE_MESSAGE(config.max_temporaries > 0, "Default temporaries should be > 0");
    TEST_ASSERT_TRUE_MESSAGE(config.max_variables > 0, "Default variables should be > 0");
    TEST_ASSERT_TRUE_MESSAGE(config.max_memory_size > 0, "Default memory should be > 0");
    TEST_ASSERT_TRUE_MESSAGE(config.max_call_depth > 0, "Default call depth should be > 0");
    TEST_ASSERT_TRUE_MESSAGE(config.max_steps > 0, "Default max steps should be > 0");
    
    // Test that default config can create engine
    test_engine = tac_engine_create(&config);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_engine, "Default config should create valid engine");
    
    test_tearDown();
}

/**
 * @brief Test execution with empty code
 */
void test_tac_engine_empty_execution(void) {
    test_setUp();
    
    tac_engine_config_t config = tac_engine_default_config();
    test_engine = tac_engine_create(&config);
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Running with no code should work
    tac_engine_error_t result = tac_engine_run(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Running empty program should succeed");
    
    // Stepping with no code should work
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Stepping empty program should succeed");
    
    test_tearDown();
}

/**
 * @brief Run all working tests
 */
void run_tac_engine_working_tests(void) {
    printf("Running TAC Engine Working Tests...\n");
    
    RUN_TEST(test_tac_engine_basic_lifecycle);
    RUN_TEST(test_tac_engine_null_pointer_robustness);
    RUN_TEST(test_tac_engine_invalid_config_rejection);
    RUN_TEST(test_tac_engine_instruction_loading);
    RUN_TEST(test_tac_engine_error_strings);
    RUN_TEST(test_tac_engine_default_config);
    RUN_TEST(test_tac_engine_empty_execution);
    
    printf("TAC Engine Working Tests completed.\n");
}
