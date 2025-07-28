/**
 * @file test_tac_engine_edge_cases.c
 * @brief TAC Engine Edge Case and Stress Tests
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 * @copyright Copyright (c) 2025
 * 
 * Aggressive tests designed to break weak implementations.
 * Following PROJECT_MANIFEST.md: "Create strong tests to break weak code"
 */

#include "test_tac_engine.h"
#include "../../../src/tools/tac_engine/tac_engine.h"
#include "../../../src/ir/tac_types.h"
#include "../../test_common.h"
#include <limits.h>
#include <string.h>

static tac_engine_t* test_engine = NULL;

void setUp(void) {
    tac_engine_config_t config = TAC_ENGINE_DEFAULT_CONFIG;
    config.max_temporaries = 1000;
    config.max_variables = 1000;
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
 * @brief Test boundary conditions for variable and temporary IDs
 */
void test_tac_engine_boundary_conditions(void) {
    tac_value_t value = {.type = TAC_VALUE_INT, .value = {.int_val = 42}};
    tac_engine_error_t result;
    
    // Test accessing variables at boundaries
    // Valid boundary - last valid temporary
    result = tac_engine_set_temporary(test_engine, 999, &value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Setting last valid temporary should succeed");
    
    result = tac_engine_get_temporary(test_engine, 999, &value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_OK, result, "Getting last valid temporary should succeed");
    TEST_ASSERT_EQUAL_INT32(42, value.value.int_val);
    
    // Invalid boundary - beyond max temporaries
    result = tac_engine_set_temporary(test_engine, 1000, &value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_INVALID_OPERAND, result,
                             "Setting temporary beyond limit should fail");
    
    result = tac_engine_get_temporary(test_engine, 1000, &value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_INVALID_OPERAND, result,
                             "Getting temporary beyond limit should fail");
    
    // Test extremely large IDs
    result = tac_engine_set_temporary(test_engine, UINT32_MAX, &value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_INVALID_OPERAND, result,
                             "Setting temporary with UINT32_MAX should fail");
    
    // Test with variables
    result = tac_engine_set_variable(test_engine, 999, &value);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_set_variable(test_engine, 1000, &value);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_INVALID_OPERAND, result,
                             "Setting variable beyond limit should fail");
}

/**
 * @brief Test invalid operand combinations
 */
void test_tac_engine_invalid_operands(void) {
    TACInstruction instructions[] = {
        // Test instruction with invalid operand type
        {
            .op = TAC_OP_ADD,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = 999, .id = 0}, // Invalid operand type
            .operand2 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 5}}}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 1);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result); // Loading might succeed
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Execution should fail with invalid operand
    result = tac_engine_step(test_engine);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(TAC_ENGINE_OK, result,
                                 "Execution with invalid operand should fail");
}

/**
 * @brief Test invalid instruction opcodes
 */
void test_tac_engine_invalid_instructions(void) {
    TACInstruction instructions[] = {
        // Valid instruction first
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 42}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // Invalid instruction opcode
        {
            .op = 999, // Invalid opcode
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
            .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
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
    
    // Second instruction should fail
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_INVALID_INSTRUCTION, result,
                             "Invalid instruction opcode should be detected");
}

/**
 * @brief Test memory corruption resistance
 */
void test_tac_engine_memory_corruption(void) {
    // Test overlapping memory operations
    uint32_t addr1 = tac_engine_alloc_memory(test_engine, 100);
    uint32_t addr2 = tac_engine_alloc_memory(test_engine, 50);
    
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, addr1, "First allocation should succeed");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, addr2, "Second allocation should succeed");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(addr1, addr2, "Allocations should be at different addresses");
    
    // Write to allocated memory
    uint8_t test_data1[100];
    uint8_t test_data2[50];
    memset(test_data1, 0xAA, sizeof(test_data1));
    memset(test_data2, 0x55, sizeof(test_data2));
    
    tac_engine_error_t result = tac_engine_write_memory(test_engine, addr1, test_data1, 100);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_write_memory(test_engine, addr2, test_data2, 50);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Verify data integrity
    uint8_t read_data1[100];
    uint8_t read_data2[50];
    
    result = tac_engine_read_memory(test_engine, addr1, read_data1, 100);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(test_data1, read_data1, 100,
                                    "First block data should be intact");
    
    result = tac_engine_read_memory(test_engine, addr2, read_data2, 50);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(test_data2, read_data2, 50,
                                    "Second block data should be intact");
    
    // Test reading/writing beyond allocated boundaries
    result = tac_engine_read_memory(test_engine, addr1 + 50, read_data1, 100);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(TAC_ENGINE_OK, result,
                                 "Reading beyond allocation should fail");
    
    result = tac_engine_write_memory(test_engine, addr2 + 25, test_data1, 100);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(TAC_ENGINE_OK, result,
                                 "Writing beyond allocation should fail");
    
    // Test access to invalid addresses
    result = tac_engine_read_memory(test_engine, 0x12345678, read_data1, 4);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_INVALID_ADDRESS, result,
                             "Access to invalid address should fail");
    
    // Cleanup
    tac_engine_free_memory(test_engine, addr1);
    tac_engine_free_memory(test_engine, addr2);
}

/**
 * @brief Test stack overflow protection
 */
void test_tac_engine_stack_overflow(void) {
    // Close current engine and create one with limited call depth
    tac_engine_close(test_engine);
    
    tac_engine_config_t config = TAC_ENGINE_DEFAULT_CONFIG;
    config.max_call_depth = 3; // Very limited stack
    test_engine = tac_engine_open(&config);
    TEST_ASSERT_NOT_NULL(test_engine);
    
    // Create recursive call instructions
    TACInstruction instructions[] = {
        // Recursive function that calls itself
        {
            .op = TAC_OP_CALL,
            .result = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 0}}}, // Call self
            .operand1 = {.type = TAC_OPERAND_NONE},
            .operand2 = {.type = TAC_OPERAND_NONE}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 1);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Execute until stack overflow
    for (int i = 0; i < 10; i++) {
        result = tac_engine_step(test_engine);
        if (result != TAC_ENGINE_OK) {
            break;
        }
    }
    
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_STACK_OVERFLOW, result,
                             "Engine should detect stack overflow");
}

/**
 * @brief Test stack underflow protection
 */
void test_tac_engine_stack_underflow(void) {
    TACInstruction instructions[] = {
        // Return without matching call
        {
            .op = TAC_OP_RETURN,
            .result = {.type = TAC_OPERAND_NONE},
            .operand1 = {.type = TAC_OPERAND_NONE},
            .operand2 = {.type = TAC_OPERAND_NONE}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 1);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Return should fail with stack underflow
    result = tac_engine_step(test_engine);
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_ERROR_STACK_UNDERFLOW, result,
                             "Return without call should cause stack underflow");
}

/**
 * @brief Test resource exhaustion scenarios
 */
void test_tac_engine_resource_exhaustion(void) {
    // Test memory exhaustion
    uint32_t addresses[1000];
    int allocated_count = 0;
    
    // Allocate until memory is exhausted
    for (int i = 0; i < 1000; i++) {
        uint32_t addr = tac_engine_alloc_memory(test_engine, 1024);
        if (addr == 0) {
            break; // Memory exhausted
        }
        addresses[allocated_count++] = addr;
    }
    
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, allocated_count,
                                    "Should be able to allocate some memory");
    
    // Try one more allocation - should fail
    uint32_t addr = tac_engine_alloc_memory(test_engine, 1024);
    TEST_ASSERT_EQUAL_MESSAGE(0, addr, "Allocation should fail when memory exhausted");
    
    // Free all allocated memory
    for (int i = 0; i < allocated_count; i++) {
        tac_engine_error_t result = tac_engine_free_memory(test_engine, addresses[i]);
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    }
    
    // Should be able to allocate again after freeing
    addr = tac_engine_alloc_memory(test_engine, 1024);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, addr, "Should be able to allocate after freeing");
    tac_engine_free_memory(test_engine, addr);
}

/**
 * @brief Test malformed TAC programs
 */
void test_tac_engine_malformed_programs(void) {
    // Test program with cycles in control flow
    TACInstruction instructions[] = {
        // 0: jump to 2
        {
            .op = TAC_OP_JUMP,
            .result = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 2}}},
            .operand1 = {.type = TAC_OPERAND_NONE},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // 1: t0 = 42 (should be skipped)
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 42}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // 2: jump to 0 (creates infinite loop)
        {
            .op = TAC_OP_JUMP,
            .result = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = 0}}},
            .operand1 = {.type = TAC_OPERAND_NONE},
            .operand2 = {.type = TAC_OPERAND_NONE}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 3);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Run with step limit to detect infinite loop
    result = tac_engine_run(test_engine, 10);
    
    // Should either succeed (with step limit) or detect infinite loop
    uint32_t step_count = tac_engine_get_step_count(test_engine);
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(3, step_count,
                                        "Should execute at least a few steps before stopping");
    
    // Verify t0 was not set (instruction 1 should be skipped)
    tac_value_t value;
    result = tac_engine_get_temporary(test_engine, 0, &value);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(42, value.value.int_val,
                                 "Skipped instruction should not have executed");
}

/**
 * @brief Test fuzzing with random operand values
 */
void test_tac_engine_fuzzing_operands(void) {
    // Test with extreme constant values
    TACInstruction instructions[] = {
        // Test with INT_MAX
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = INT_MAX}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // Test with INT_MIN
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = INT_MIN}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        },
        // Test addition with extremes
        {
            .op = TAC_OP_ADD,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 2},
            .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand2 = {.type = TAC_OPERAND_TEMPORARY, .id = 1}
        }
    };
    
    tac_engine_error_t result = tac_engine_load_code(test_engine, instructions, 3);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    result = tac_engine_start(test_engine, 0);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Execute all instructions - engine should not crash
    result = tac_engine_run(test_engine, 10);
    // Result is implementation-defined for overflow, but engine should survive
    TEST_ASSERT_NOT_EQUAL_MESSAGE(TAC_ENGINE_STATE_ERROR, tac_engine_get_state(test_engine),
                                 "Engine should survive extreme value operations");
}

/**
 * @brief Test concurrent-like operations (rapid state changes)
 */
void test_tac_engine_concurrent_operations(void) {
    // Rapid start/stop cycles
    for (int i = 0; i < 10; i++) {
        TACInstruction instruction = {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {.type = TAC_OPERAND_CONSTANT, .value = {.type = INT_TYPE, .value = {.int_val = i}}},
            .operand2 = {.type = TAC_OPERAND_NONE}
        };
        
        tac_engine_error_t result = tac_engine_load_code(test_engine, &instruction, 1);
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
        
        result = tac_engine_start(test_engine, 0);
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
        
        result = tac_engine_step(test_engine);
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
        
        result = tac_engine_stop(test_engine);
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
        
        result = tac_engine_reset(test_engine);
        TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    }
    
    // Verify engine is still functional
    TEST_ASSERT_EQUAL_MESSAGE(TAC_ENGINE_STATE_LOADED, tac_engine_get_state(test_engine),
                             "Engine should be functional after rapid cycles");
}

/**
 * @brief Run all edge case and stress tests
 */
void run_tac_engine_edge_case_tests(void) {
    printf("Running TAC Engine Edge Case Tests...\n");
    
    RUN_TEST(test_tac_engine_boundary_conditions);
    RUN_TEST(test_tac_engine_invalid_operands);
    RUN_TEST(test_tac_engine_invalid_instructions);
    RUN_TEST(test_tac_engine_memory_corruption);
    RUN_TEST(test_tac_engine_stack_overflow);
    RUN_TEST(test_tac_engine_stack_underflow);
    RUN_TEST(test_tac_engine_resource_exhaustion);
    RUN_TEST(test_tac_engine_malformed_programs);
    RUN_TEST(test_tac_engine_fuzzing_operands);
    RUN_TEST(test_tac_engine_concurrent_operations);
    
    printf("TAC Engine Edge Case Tests completed.\n");
}

/**
 * @brief Run stress tests
 */
void run_tac_engine_stress_tests(void) {
    printf("Running TAC Engine Stress Tests...\n");
    
    // Just run the edge case tests again as stress tests
    run_tac_engine_edge_case_tests();
    
    printf("TAC Engine Stress Tests completed.\n");
}
