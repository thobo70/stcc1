/**
 * @file test_tac_engine_debugging.c
 * @brief TAC Engine Debugging Features Tests
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 * @copyright Copyright (c) 2025
 * 
 * Tests for breakpoints, hooks, tracing, and other debugging features.
 */

#include "test_tac_engine.h"
#include "../../../src/tools/tac_engine/tac_engine.h"
#include "../../../src/ir/tac_types.h"
#include "../../test_common.h"

static tac_engine_t* test_engine = NULL;
static int hook_call_count = 0;
static tac_hook_type_t last_hook_type = 0;
static uint32_t last_hook_address = 0;

// Test hook callback
static bool test_hook_callback(tac_engine_t* engine,
                              tac_hook_type_t hook_type,
                              uint32_t address,
                              void* user_data) {
    hook_call_count++;
    last_hook_type = hook_type;
    last_hook_address = address;
    
    // Check user_data if provided
    if (user_data) {
        int* counter = (int*)user_data;
        (*counter)++;
    }
    
    return true; // Continue execution
}

// Hook that stops execution
static bool stop_hook_callback(tac_engine_t* engine,
                              tac_hook_type_t hook_type,
                              uint32_t address,
                              void* user_data) {
    hook_call_count++;
    return false; // Stop execution
}

void setUp(void) {
    tac_engine_config_t config = TAC_ENGINE_DEFAULT_CONFIG;
    config.enable_tracing = true;
    config.max_trace_entries = 100;
    test_engine = tac_engine_open(&config);
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Reset hook tracking
    hook_call_count = 0;
    last_hook_type = 0;
    last_hook_address = 0;
}

void tearDown(void) {
    if (test_engine) {
        tac_engine_close(test_engine);
        test_engine = NULL;
    }
}

/**
 * @brief Test basic breakpoint functionality
 */
void test_tac_engine_breakpoints(void) {
    TACInstruction instructions[] = {
        // 0: t0 = 1
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 1}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // 1: t1 = 2
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 2}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // 2: t2 = 3
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 2},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 3}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 3);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Set breakpoint at instruction 1
    result = tac_engine_add_breakpoint(test_engine, 1);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Adding breakpoint should succeed");
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // First step should execute normally
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "First step should succeed");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, tac_engine_get_pc(test_engine),
                                    "PC should advance to 1");
    
    // Second step should hit breakpoint
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_BREAKPOINT_HIT, result,
                             "Second step should hit breakpoint");
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_STATE_PAUSED, tac_engine_get_state(test_engine),
                             "Engine should be paused at breakpoint");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, tac_engine_get_pc(test_engine),
                                    "PC should still be at breakpoint");
    
    // Resume and continue
    result = tac_engine_resume(test_engine);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Should continue after resume");
    
    // Remove breakpoint and test
    result = tac_engine_remove_breakpoint(test_engine, 1);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Removing breakpoint should succeed");
    
    // Reset and run again - should not hit breakpoint
    result = tac_engine_reset(test_engine);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_run(test_engine, 10);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Should run without hitting removed breakpoint");
}

/**
 * @brief Test multiple breakpoints
 */
void test_tac_engine_multiple_breakpoints(void) {
    TACInstruction instructions[5];
    for (int i = 0; i < 5; i++) {
        instructions[i] = (TACInstruction) {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = (uint32_t)i},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = i}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        };
    }
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 5);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Set breakpoints at instructions 1 and 3
    result = tac_engine_add_breakpoint(test_engine, 1);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    result = tac_engine_add_breakpoint(test_engine, 3);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Execute until first breakpoint
    result = tac_engine_run(test_engine, 10);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_BREAKPOINT_HIT, result,
                             "Should hit first breakpoint");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, tac_engine_get_pc(test_engine),
                                    "Should be at first breakpoint");
    
    // Continue to second breakpoint
    result = tac_engine_resume(test_engine);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_run(test_engine, 10);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_BREAKPOINT_HIT, result,
                             "Should hit second breakpoint");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(3, tac_engine_get_pc(test_engine),
                                    "Should be at second breakpoint");
    
    // Clear all breakpoints
    result = tac_engine_clear_breakpoints(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Clearing breakpoints should succeed");
}

/**
 * @brief Test execution hooks
 */
void test_tac_engine_hooks(void) {
    TACInstruction instructions[] = {
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 42}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 1);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Add instruction hook
    uint32_t hook_id = tac_engine_add_hook(test_engine, TAC_HOOK_INSTRUCTION,
                                           test_hook_callback, NULL);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, hook_id, "Adding hook should return valid ID");
    
    // Add start hook
    uint32_t start_hook_id = tac_engine_add_hook(test_engine, TAC_HOOK_CODE_START,
                                                 test_hook_callback, NULL);
    TEST_ASSERT_NOT_EQUAL(0, start_hook_id);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Should have triggered start hook
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, hook_call_count, "Start hook should have been called");
    TEST_ASSERT_EQUAL_MESSAGE(TAC_HOOK_CODE_START, last_hook_type,
                             "Last hook should be start hook");
    
    int old_count = hook_call_count;
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Should have triggered instruction hook
    TEST_ASSERT_GREATER_THAN_MESSAGE(old_count, hook_call_count,
                                    "Instruction hook should have been called");
    TEST_ASSERT_EQUAL_MESSAGE(TAC_HOOK_INSTRUCTION, last_hook_type,
                             "Last hook should be instruction hook");
    
    // Test hook removal
    result = tac_engine_remove_hook(test_engine, hook_id);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Removing hook should succeed");
    
    // Test removing non-existent hook
    result = tac_engine_remove_hook(test_engine, 99999);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_NOT_FOUND, result,
                             "Removing non-existent hook should fail");
}

/**
 * @brief Test hook that stops execution
 */
void test_tac_engine_stop_hook(void) {
    TACInstruction instructions[] = {
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 1}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 2}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 2);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Add hook that stops execution
    uint32_t hook_id = tac_engine_add_hook(test_engine, TAC_HOOK_INSTRUCTION,
                                           stop_hook_callback, NULL);
    TEST_ASSERT_NOT_EQUAL(0, hook_id);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Step should succeed but hook should stop further execution
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Step should succeed");
    
    // Verify hook was called
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, hook_call_count, "Hook should have been called");
    
    // Next step should not execute the instruction (hook stops it)
    hook_call_count = 0;
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Step should return OK");
    // But PC should not advance since hook stopped execution
}

/**
 * @brief Test execution tracing
 */
void test_tac_engine_tracing(void) {
    TACInstruction instructions[] = {
        // t0 = 5
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 5}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // t1 = 3
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 3}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // t2 = t0 + t1
        {
            .op = TAC_OP_ADD,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 2},
            .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand2 = {.type = TAC_OPERAND_TEMPORARY, .id = 1}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 3);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Verify tracing is enabled
    result = tac_engine_enable_tracing(test_engine, true);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Execute all instructions
    result = tac_engine_run(test_engine, 10);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Check trace entries
    uint32_t trace_count = tac_engine_get_trace_count(test_engine);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(3, trace_count, "Should have 3 trace entries");
    
    // Verify trace entries
    for (uint32_t i = 0; i < trace_count; i++) {
        tac_trace_entry_t entry;
        result = tac_engine_get_trace_entry(test_engine, i, &entry);
        TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Getting trace entry should succeed");
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(i, entry.address, "Trace entry address should match");
        TEST_ASSERT_EQUAL_UINT32_MESSAGE(i + 1, entry.step, "Trace entry step should match");
    }
    
    // Test clearing trace
    result = tac_engine_clear_trace(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Clearing trace should succeed");
    
    trace_count = tac_engine_get_trace_count(test_engine);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, trace_count, "Trace count should be 0 after clearing");
}

/**
 * @brief Test disassembly functionality
 */
void test_tac_engine_disassembly(void) {
    TACInstruction instructions[] = {
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 42}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        {
            .op = TAC_OP_ADD,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
            .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand2 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 8}}}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 2);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Test disassembly
    char disasm_buffer[1024];
    result = tac_engine_disassemble(test_engine, 0, 2, disasm_buffer, sizeof(disasm_buffer));
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Disassembly should succeed");
    
    // Check that buffer contains something
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, strlen(disasm_buffer),
                                    "Disassembly buffer should not be empty");
    
    // Should contain instruction addresses
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(disasm_buffer, "0000:"),
                                 "Should contain first instruction address");
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(disasm_buffer, "0001:"),
                                 "Should contain second instruction address");
    
    // Test disassembly with invalid range
    result = tac_engine_disassemble(test_engine, 5, 2, disasm_buffer, sizeof(disasm_buffer));
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_INVALID_ADDRESS, result,
                             "Disassembly beyond code should fail");
}

/**
 * @brief Test variable inspection during debugging
 */
void test_tac_engine_variable_inspection(void) {
    TACInstruction instructions[] = {
        // v0 = 100
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_VARIABLE, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 100}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // t0 = v0 + 50
        {
            .op = TAC_OP_ADD,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_VARIABLE, .id = 0},
            .operand2 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 50}}}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 2);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Set breakpoint after first instruction
    result = tac_engine_add_breakpoint(test_engine, 1);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Run until breakpoint
    result = tac_engine_run(test_engine, 10);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_BREAKPOINT_HIT, result,
                             "Should hit breakpoint");
    
    // Inspect variable v0
    tac_value_t value;
    result = tac_engine_get_variable(test_engine, 0, &value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Getting variable should succeed");
    TEST_ASSERT_EQUAL_INT32_MESSAGE(100, value.value.int_val,
                                   "Variable v0 should contain 100");
    
    // t0 should still be uninitialized
    result = tac_engine_get_temporary(test_engine, 0, &value);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(150, value.value.int_val,
                                 "Temporary t0 should not contain result yet");
    
    // Continue execution
    result = tac_engine_resume(test_engine);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Now t0 should have the result
    result = tac_engine_get_temporary(test_engine, 0, &value);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL_INT32_MESSAGE(150, value.value.int_val,
                                   "Temporary t0 should contain 150");
}

/**
 * @brief Run all debugging tests
 */
void run_tac_engine_debugging_tests(void) {
    printf("Running TAC Engine Debugging Tests...\n");
    
    RUN_TEST(test_tac_engine_breakpoints);
    RUN_TEST(test_tac_engine_multiple_breakpoints);
    RUN_TEST(test_tac_engine_hooks);
    RUN_TEST(test_tac_engine_stop_hook);
    RUN_TEST(test_tac_engine_tracing);
    RUN_TEST(test_tac_engine_disassembly);
    RUN_TEST(test_tac_engine_variable_inspection);
    
    printf("TAC Engine Debugging Tests completed.\n");
}
