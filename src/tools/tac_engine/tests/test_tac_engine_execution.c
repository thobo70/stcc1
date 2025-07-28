/**
 * @file test_tac_engine_execution.c
 * @brief TAC Engine Execution Tests
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 * @copyright Copyright (c) 2025
 * 
 * Tests for TAC instruction execution, arithmetic operations, and control flow.
 * These tests are designed to break weak implementations and expose edge cases.
 */

#include "test_tac_engine.h"
#include "../tac_engine.h"
#include "../../../ir/tac_types.h"
#include "../../../tests/test_common.h"
#include <limits.h>
#include <float.h>
#include <math.h>

static tac_engine_t* test_engine = NULL;

void setUp(void) {
    tac_engine_config_t config = TAC_ENGINE_DEFAULT_CONFIG;
    config.max_temporaries = 100;
    config.max_variables = 50;
    config.enable_tracing = true;
    test_engine = tac_engine_open(&config);
    TEST_ASSERT_NOT_NULL(test_engine);
}

void tearDown(void) {
    if (test_engine) {
        tac_engine_close(test_engine);
        test_engine = NULL;
    }
}

/**
 * @brief Test loading valid and invalid code
 */
void test_tac_engine_load_code(void) {
    TACInstruction valid_instructions[] = {
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 42}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        }
    };
    
    // Test valid code loading
    tac_engine_error_t result = tac_engine_load_code(test_engine, valid_instructions, 1);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Loading valid code should succeed");
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_STATE_LOADED, tac_engine_get_state(test_engine),
                             "Engine state should be LOADED after loading code");
    
    // Test NULL instructions
    result = tac_engine_load_code(test_engine, NULL, 1);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_INVALID_PARAM, result,
                             "Loading NULL instructions should fail");
    
    // Test zero count
    result = tac_engine_load_code(test_engine, valid_instructions, 0);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_INVALID_PARAM, result,
                             "Loading zero instructions should fail");
    
    // Test loading code while running (should fail)
    tac_engine_start(test_engine, 0);
    result = tac_engine_load_code(test_engine, valid_instructions, 1);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_ENGINE_BUSY, result,
                             "Loading code while running should fail");
}

/**
 * @brief Test basic arithmetic operations
 */
void test_tac_engine_basic_arithmetic(void) {
    TACInstruction instructions[] = {
        // t0 = 10
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 10}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // t1 = 5
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 5}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // t2 = t0 + t1  (10 + 5 = 15)
        {
            .op = TAC_OP_ADD,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 2},
            .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand2 = {.type = TAC_OPERAND_TEMPORARY, .id = 1}
        },
        // t3 = t0 - t1  (10 - 5 = 5)
        {
            .op = TAC_OP_SUB,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 3},
            .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand2 = {.type = TAC_OPERAND_TEMPORARY, .id = 1}
        },
        // t4 = t0 * t1  (10 * 5 = 50)
        {
            .op = TAC_OP_MUL,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 4},
            .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand2 = {.type = TAC_OPERAND_TEMPORARY, .id = 1}
        },
        // t5 = t0 / t1  (10 / 5 = 2)
        {
            .op = TAC_OP_DIV,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 5},
            .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand2 = {.type = TAC_OPERAND_TEMPORARY, .id = 1}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 6);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Execute all instructions
    result = tac_engine_run(test_engine, 10);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Verify results
    tac_value_t value;
    
    // Check addition result
    result = tac_engine_get_temporary(test_engine, 2, &value);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL_INT32_MESSAGE(15, value.value.int_val, "Addition result incorrect");
    
    // Check subtraction result
    result = tac_engine_get_temporary(test_engine, 3, &value);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL_INT32_MESSAGE(5, value.value.int_val, "Subtraction result incorrect");
    
    // Check multiplication result
    result = tac_engine_get_temporary(test_engine, 4, &value);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL_INT32_MESSAGE(50, value.value.int_val, "Multiplication result incorrect");
    
    // Check division result
    result = tac_engine_get_temporary(test_engine, 5, &value);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL_INT32_MESSAGE(2, value.value.int_val, "Division result incorrect");
}

/**
 * @brief Test division by zero - should fail gracefully
 */
void test_tac_engine_division_by_zero(void) {
    TACInstruction instructions[] = {
        // t0 = 42
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 42}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // t1 = 0
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 0}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // t2 = t0 / t1  (42 / 0 - should fail)
        {
            .op = TAC_OP_DIV,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 2},
            .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand2 = {.type = TAC_OPERAND_TEMPORARY, .id = 1}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 3);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Execute first two instructions (should succeed)
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Third instruction should fail with division by zero
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_DIVISION_BY_ZERO, result,
                             "Division by zero should be detected and fail");
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_STATE_ERROR, tac_engine_get_state(test_engine),
                             "Engine should be in error state after division by zero");
}

/**
 * @brief Test integer overflow conditions
 */
void test_tac_engine_overflow_conditions(void) {
    TACInstruction instructions[] = {
        // t0 = INT_MAX
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = INT_MAX}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // t1 = 1
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 1}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // t2 = t0 + t1  (INT_MAX + 1 - overflow)
        {
            .op = TAC_OP_ADD,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 2},
            .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand2 = {.type = TAC_OPERAND_TEMPORARY, .id = 1}
        },
        // t3 = INT_MIN
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 3},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = INT_MIN}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // t4 = t3 - t1  (INT_MIN - 1 - underflow)
        {
            .op = TAC_OP_SUB,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 4},
            .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 3},
            .operand2 = {.type = TAC_OPERAND_TEMPORARY, .id = 1}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 5);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Execute all instructions - overflow behavior is implementation-defined
    // The test ensures the engine doesn't crash
    result = tac_engine_run(test_engine, 10);
    // Engine should either succeed (with wrapped values) or detect overflow
    TEST_ASSERT_TRUE_MESSAGE(result == TAC_ENGINE_OK || result == TAC_ENGINE_ERROR_OVERFLOW,
                            "Engine should handle overflow gracefully");
    
    // Verify engine is still functional
    TEST_ASSERT_NOT_EQUAL_MESSAGE(TAC_ENGINE_STATE_ERROR, tac_engine_get_state(test_engine),
                                 "Engine should not be in error state after overflow");
}

/**
 * @brief Test control flow with jumps
 */
void test_tac_engine_control_flow(void) {
    TACInstruction instructions[] = {
        // t0 = 0
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 0}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // if t0 == 0 jump to 4
        {
            .op = TAC_OP_JUMP_IF_ZERO,
            .result = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 4}}},
            .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // t1 = 999 (should be skipped)
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 999}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // unconditional jump to 5
        {
            .op = TAC_OP_JUMP,
            .result = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 5}}},
            .operand1 = {.type = TAC_OPERAND_NONE},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // t1 = 42 (target of conditional jump)
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 42}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // t2 = 100 (target of unconditional jump)
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 2},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 100}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 6);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_run(test_engine, 10);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Verify that conditional jump worked (t1 should be 42, not 999)
    tac_value_t value;
    result = tac_engine_get_temporary(test_engine, 1, &value);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL_INT32_MESSAGE(42, value.value.int_val,
                                   "Conditional jump failed - t1 should be 42");
    
    // Verify that unconditional jump worked (t2 should be 100)
    result = tac_engine_get_temporary(test_engine, 2, &value);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL_INT32_MESSAGE(100, value.value.int_val,
                                   "Unconditional jump failed - t2 should be 100");
}

/**
 * @brief Test invalid jump addresses
 */
void test_tac_engine_invalid_jumps(void) {
    TACInstruction instructions[] = {
        // t0 = 0
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 0}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // Jump to invalid address (beyond code)
        {
            .op = TAC_OP_JUMP,
            .result = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 999}}},
            .operand1 = {.type = TAC_OPERAND_NONE},
            .operand2 = {.type = TAC_OPERAND_NONE}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 2);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // First instruction should succeed
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Second instruction should fail with invalid address
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_INVALID_ADDRESS, result,
                             "Jump to invalid address should fail");
}

/**
 * @brief Test step limits
 */
void test_tac_engine_step_limits(void) {
    tac_engine_close(test_engine);
    
    // Create engine with step limit
    tac_engine_config_t config = TAC_ENGINE_DEFAULT_CONFIG;
    config.max_steps = 3;
    test_engine = tac_engine_open(&config);
    TEST_ASSERT_NOT_NULL(test_engine);
    
    TACInstruction instructions[] = {
        {.op = TAC_OP_ASSIGN, .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
         .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 1}}}},
        {.op = TAC_OP_ASSIGN, .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
         .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 2}}}},
        {.op = TAC_OP_ASSIGN, .result = {.type = TAC_OPERAND_TEMPORARY, .id = 2},
         .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 3}}}},
        {.op = TAC_OP_ASSIGN, .result = {.type = TAC_OPERAND_TEMPORARY, .id = 3},
         .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 4}}}}
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 4);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Run should hit step limit
    result = tac_engine_run(test_engine, 0);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_STEP_LIMIT_EXCEEDED, result,
                             "Engine should hit step limit");
    
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(3, tac_engine_get_step_count(test_engine),
                                    "Step count should equal limit");
}

/**
 * @brief Run all execution tests
 */
void run_tac_engine_execution_tests(void) {
    printf("Running TAC Engine Execution Tests...\n");
    
    RUN_TEST(test_tac_engine_load_code);
    RUN_TEST(test_tac_engine_basic_arithmetic);
    RUN_TEST(test_tac_engine_division_by_zero);
    RUN_TEST(test_tac_engine_overflow_conditions);
    RUN_TEST(test_tac_engine_control_flow);
    RUN_TEST(test_tac_engine_invalid_jumps);
    RUN_TEST(test_tac_engine_step_limits);
    
    printf("TAC Engine Execution Tests completed.\n");
}
