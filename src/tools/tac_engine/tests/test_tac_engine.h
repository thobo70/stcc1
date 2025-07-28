/**
 * @file test_tac_engine.h
 * @brief TAC Engine Unit Test Suite Header
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 * @copyright Copyright (c) 2025
 * 
 * This header declares the TAC Engine test suite functions.
 * Following PROJECT_MANIFEST.md principles:
 * - NEVER weaken a test to make it pass
 * - FIX the code, not the test
 * - Create strong tests that break weak code
 * - Comprehensive edge case testing
 */

#ifndef TEST_TAC_ENGINE_H
#define TEST_TAC_ENGINE_H

#include "../../../Unity/src/unity.h"

// Test suite runners
void run_tac_engine_lifecycle_tests(void);
void run_tac_engine_execution_tests(void);
void run_tac_engine_debugging_tests(void);
void run_tac_engine_error_tests(void);
void run_tac_engine_memory_tests(void);
void run_tac_engine_edge_case_tests(void);
void run_tac_engine_stress_tests(void);

// Individual test functions
void test_tac_engine_create_destroy(void);
void test_tac_engine_invalid_config(void);
void test_tac_engine_load_code(void);
void test_tac_engine_load_invalid_code(void);
void test_tac_engine_basic_arithmetic(void);
void test_tac_engine_division_by_zero(void);
void test_tac_engine_overflow_conditions(void);
void test_tac_engine_control_flow(void);
void test_tac_engine_function_calls(void);
void test_tac_engine_breakpoints(void);
void test_tac_engine_hooks(void);
void test_tac_engine_tracing(void);
void test_tac_engine_variable_access(void);
void test_tac_engine_memory_operations(void);
void test_tac_engine_memory_corruption(void);
void test_tac_engine_stack_overflow(void);
void test_tac_engine_stack_underflow(void);
void test_tac_engine_invalid_operands(void);
void test_tac_engine_invalid_instructions(void);
void test_tac_engine_boundary_conditions(void);
void test_tac_engine_concurrent_operations(void);
void test_tac_engine_resource_exhaustion(void);
void test_tac_engine_fuzzing_operands(void);
void test_tac_engine_malformed_programs(void);

#endif // TEST_TAC_ENGINE_H
