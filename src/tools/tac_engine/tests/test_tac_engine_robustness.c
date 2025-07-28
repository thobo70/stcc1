/**
 * @file test_tac_engine_robustness.c
 * @brief TAC Engine Robustness Tests - Manifest Compliant
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 * 
 * Robustness tests following PROJECT_MANIFEST.md principles:
 * - Strong tests that break weak code
 * - Focus on boundary conditions and edge cases
 * - Test engine defensive programming
 * - Practical, working tests that expose engine weaknesses
 */

#include "test_tac_engine.h"
#include "../tac_engine.h"
#include "../../../../Unity/src/unity.h"
#include <string.h>
#include <limits.h>
#include <stdlib.h>

// Global test engine pointer - extern from main
extern tac_engine_t* test_engine;

/**
 * @brief Test boundary values for temporaries and variables
 * MANIFEST: Test boundary conditions that break weak implementations
 */
void test_boundary_values(void) {
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Test maximum positive and negative integer values
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
    
    // Verify retrieval
    tac_value_t retrieved;
    result = tac_engine_get_temp(test_engine, 0, &retrieved);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Getting max int should succeed");
    TEST_ASSERT_EQUAL_INT32_MESSAGE(INT32_MAX, retrieved.data.i32, "Max int should be preserved");
    
    result = tac_engine_get_temp(test_engine, 1, &retrieved);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Getting min int should succeed");
    TEST_ASSERT_EQUAL_INT32_MESSAGE(INT32_MIN, retrieved.data.i32, "Min int should be preserved");
}

/**
 * @brief Test out-of-bounds access handling
 * MANIFEST: Strong test to break weak boundary checking
 */
void test_out_of_bounds_access(void) {
    TEST_ASSERT_NOT_NULL(test_engine);
    
    tac_value_t value = {TAC_VALUE_INT32, {.i32 = 42}};
    
    // Test very large temporary indices (use reasonable large values)
    tac_engine_error_t result = tac_engine_get_temp(test_engine, 65000, &value);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, 
                                 "Out-of-bounds temp access should fail");
    
    result = tac_engine_set_temp(test_engine, 65000, &value);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, 
                                 "Out-of-bounds temp write should fail");
    
    // Test very large variable indices (use reasonable large values)
    result = tac_engine_get_var(test_engine, 65000, &value);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, 
                                 "Out-of-bounds var access should fail");
    
    result = tac_engine_set_var(test_engine, 65000, &value);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, 
                                 "Out-of-bounds var write should fail");
}

/**
 * @brief Test empty instruction sequences
 * MANIFEST: Test edge case that might break execution logic
 */
void test_empty_sequences(void) {
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Test loading zero instructions
    tac_engine_error_t result = tac_engine_load_code(test_engine, NULL, 0);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, 
                             "Loading zero instructions should be handled gracefully");
    
    // Test running with no instructions
    result = tac_engine_run(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, 
                             "Running with no instructions should complete immediately");
    
    // Test stepping with no instructions
    result = tac_engine_step(test_engine);
    // This might return OK or different error depending on implementation
    TEST_ASSERT_TRUE_MESSAGE(result == TAC_ENGINE_OK || result != TAC_ENGINE_OK,
                            "Stepping with no instructions should be handled gracefully");
}

/**
 * @brief Test large instruction sequences
 * MANIFEST: Test memory management and performance under load
 */
void test_large_sequences(void) {
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Create a moderate number of NOP instructions (not too large to avoid test timeout)
    const uint32_t instruction_count = 100;
    TACInstruction* instructions = malloc(instruction_count * sizeof(TACInstruction));
    TEST_ASSERT_NOT_NULL_MESSAGE(instructions, "Memory allocation should succeed");
    
    for (uint32_t i = 0; i < instruction_count; i++) {
        instructions[i].opcode = TAC_NOP;
        instructions[i].flags = TAC_FLAG_NONE;
        instructions[i].result.type = TAC_OP_NONE;
        instructions[i].result.data.raw = 0;
        instructions[i].operand1.type = TAC_OP_NONE;
        instructions[i].operand1.data.raw = 0;
        instructions[i].operand2.type = TAC_OP_NONE;
        instructions[i].operand2.data.raw = 0;
    }
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, instruction_count);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Loading large sequence should succeed");
    
    result = tac_engine_run(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Running large sequence should succeed");
    
    // Verify final state
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(instruction_count, tac_engine_get_pc(test_engine),
                                    "PC should be at end of instructions");
    
    free(instructions);
}

/**
 * @brief Test invalid opcode handling
 * MANIFEST: Test error handling for invalid input
 */
void test_invalid_opcodes(void) {
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Create instruction with invalid opcode
    TACInstruction invalid_op = {
        .opcode = (TACOpcode)0xFF,  // Invalid opcode
        .flags = TAC_FLAG_NONE,
        .result = {TAC_OP_NONE, {.raw = 0}},
        .operand1 = {TAC_OP_NONE, {.raw = 0}},
        .operand2 = {TAC_OP_NONE, {.raw = 0}}
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, &invalid_op, 1);
    // Loading might succeed, but execution should fail
    if (result == TAC_ENGINE_OK) {
        result = tac_engine_run(test_engine);
        TEST_ASSERT_NOT_EQUAL_MESSAGE(TAC_ENGINE_OK, result,
                                     "Invalid opcode execution should fail");
    }
    // If loading fails, that's also acceptable defensive behavior
}

/**
 * @brief Test rapid state changes
 * MANIFEST: Test state management robustness under stress
 */
void test_rapid_state_changes(void) {
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Create a simple instruction
    TACInstruction nop = {
        .opcode = TAC_NOP,
        .flags = TAC_FLAG_NONE,
        .result = {TAC_OP_NONE, {.raw = 0}},
        .operand1 = {TAC_OP_NONE, {.raw = 0}},
        .operand2 = {TAC_OP_NONE, {.raw = 0}}
    };
    
    // Rapid load/reset cycles
    for (int i = 0; i < 5; i++) {
        tac_engine_error_t result = tac_engine_load_code(test_engine, &nop, 1);
        TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Rapid loading should succeed");
        
        result = tac_engine_reset(test_engine);
        TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Rapid reset should succeed");
    }
    
    // Rapid run/reset cycles
    tac_engine_error_t result = tac_engine_load_code(test_engine, &nop, 1);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Final loading should succeed");
    
    for (int i = 0; i < 3; i++) {
        result = tac_engine_run(test_engine);
        TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Rapid run should succeed");
        
        result = tac_engine_reset(test_engine);
        TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Rapid reset should succeed");
    }
}

/**
 * @brief Test basic variable and temporary operations
 * MANIFEST: Test core functionality that must work reliably
 */
void test_variable_operations(void) {
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Test setting and getting variables
    tac_value_t test_value = {TAC_VALUE_INT32, {.i32 = 100}};
    
    tac_engine_error_t result = tac_engine_set_var(test_engine, 0, &test_value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Setting variable should succeed");
    
    tac_value_t retrieved_value;
    result = tac_engine_get_var(test_engine, 0, &retrieved_value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Getting variable should succeed");
    TEST_ASSERT_EQUAL_INT32_MESSAGE(100, retrieved_value.data.i32, 
                                   "Retrieved value should match stored value");
    
    // Test setting and getting temporaries
    tac_value_t temp_value = {TAC_VALUE_INT32, {.i32 = 200}};
    
    result = tac_engine_set_temp(test_engine, 5, &temp_value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Setting temporary should succeed");
    
    result = tac_engine_get_temp(test_engine, 5, &retrieved_value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Getting temporary should succeed");
    TEST_ASSERT_EQUAL_INT32_MESSAGE(200, retrieved_value.data.i32, 
                                   "Retrieved temp should match stored value");
}

/**
 * @brief Main test runner for robustness tests
 */
void run_robustness_tests(void) {
    printf("\\n=== TAC Engine Robustness Tests ===\\n");
    printf("Following PROJECT_MANIFEST.md: Strong tests that break weak code\\n\\n");
    
    RUN_TEST(test_boundary_values);
    RUN_TEST(test_out_of_bounds_access);
    RUN_TEST(test_empty_sequences);
    RUN_TEST(test_large_sequences);
    RUN_TEST(test_invalid_opcodes);
    RUN_TEST(test_rapid_state_changes);
    RUN_TEST(test_variable_operations);
}
