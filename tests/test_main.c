/**
 * @file test_main.c
 * @brief Main test runner for STCC1 compiler unit tests
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 1.0
 * @date 2025-07-27
 * 
 * This file contains the main test runner that executes all unit and integration
 * tests for the STCC1 compiler using the Unity test framework.
 * 
 * Following PROJECT_MANIFEST.md principles:
 * - NEVER weaken a test to make it pass
 * - FIX the code, not the test
 * - Comprehensive edge case testing
 * - Break weak implementations
 */

#include "unity.h"
#include "unit/test_storage_edge_cases.h"
#include "unit/test_hmapbuf_edge_cases.h"
#include "unit/test_lexer_edge_cases.h"
#include "unit/test_parser_edge_cases.h"
#include <stdio.h>
#include <stdlib.h>

// Forward declarations for test suites
void run_simple_tests(void);
void run_integration_tests(void);

/**
 * @brief Setup function called before each test
 */
void setUp(void) {
    // Setup code that runs before each test
    // Initialize any global state or temporary files needed
}

/**
 * @brief Teardown function called after each test
 */
void tearDown(void) {
    // Cleanup code that runs after each test
    // Clean up temporary files, reset global state
}

/**
 * @brief Main test runner
 */
int main(void) {
    printf("=== STCC1 Compiler Test Suite ===\n");
    printf("Running comprehensive unit and integration tests...\n");
    printf("Following PROJECT_MANIFEST.md: NEVER weaken tests to make them pass!\n\n");
    
    UNITY_BEGIN();
    
    // Unit Tests
    printf("--- Unit Tests ---\n");
    printf("Running simple tests...\n");
    run_simple_tests();
    
    // Edge Case Tests - Designed to break weak code
    printf("\n--- Edge Case Tests (Aggressive) ---\n");
    printf("Testing storage components with extreme conditions...\n");
    run_storage_edge_case_tests();
    
    printf("\nTesting hmapbuf memory management edge cases...\n");
    run_hmapbuf_edge_case_tests();
    
    printf("\nTesting lexer with malformed input...\n");
    run_lexer_edge_case_tests();
    
    printf("\nTesting parser with invalid syntax...\n");
    run_parser_edge_case_tests();
    
    // Integration Tests
    printf("\n--- Integration Tests ---\n");
    printf("Running integration tests...\n");
    run_integration_tests();
    
    printf("\n=== Test Summary ===\n");
    return UNITY_END();
}
