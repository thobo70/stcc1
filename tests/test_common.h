/**
 * @file test_common.h
 * @brief Common test utilities and helper functions for STCC1 tests
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 1.0
 * @date 2025-07-27
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "../Unity/src/unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include "../src/ir/tac_types.h"

// Test fixture paths
#define FIXTURES_PATH "tests/fixtures/"
#define TEMP_PATH "tests/temp/"

// Helper macros for test assertions
#define TEST_ASSERT_FILE_EXISTS(filename) \
    do { \
        char msg[256]; \
        snprintf(msg, sizeof(msg), "File should exist: %s", filename); \
        TEST_ASSERT_MESSAGE(access(filename, F_OK) == 0, msg); \
    } while(0)

#define TEST_ASSERT_FILE_NOT_EXISTS(filename) \
    do { \
        char msg[256]; \
        snprintf(msg, sizeof(msg), "File should not exist: %s", filename); \
        TEST_ASSERT_MESSAGE(access(filename, F_OK) != 0, msg); \
    } while(0)

#define TEST_ASSERT_STRING_CONTAINS(haystack, needle) \
    TEST_ASSERT_MESSAGE(strstr(haystack, needle) != NULL, "String should contain substring")

// Test utilities
char* create_temp_file(const char* content);
void cleanup_temp_files(void);
void setup_test_environment(void);
int run_compiler_stage(const char* stage, const char* input_file, char** output_files);
char* read_file_content(const char* filename);
int compare_files(const char* file1, const char* file2);

// Common test data structures
typedef struct {
    const char* input;
    const char* expected_output;
    const char* description;
} TestCase;

typedef struct {
    const char* source_code;
    int expected_tokens;
    int expected_symbols;
    int expected_ast_nodes;
    const char* expected_tac_pattern;
} CompilerTestCase;

// TAC Engine validation for integration tests
typedef struct TACValidationResult {
    bool success;
    int executed_instructions;
    int final_return_value;
    char error_message[256];
} TACValidationResult;

// Test runner function declarations
void run_simple_tests(void);
void run_integration_tests(void);

// TAC Engine validation functions
TACValidationResult validate_tac_execution(const char* tac_file, 
                                          int expected_return_value);
TACValidationResult validate_tac_execution_with_label(const char* tac_file, 
                                                     uint16_t entry_label_id,
                                                     int expected_return_value);
uint16_t extract_main_label_from_tac_file(const char* tac_file);
TACValidationResult validate_tac_execution_with_main(const char* tac_file, 
                                                    int expected_return_value);
TACValidationResult validate_tac_execution_with_symbols(const char* tac_file,
                                                       const char* symtab_file,
                                                       const char* sstore_file,
                                                       int expected_return_value);

// Symbol table detection utilities
bool detect_symbol_files(const char* base_filename, 
                        char* symtab_path, 
                        char* sstore_path, 
                        size_t path_size);
int load_tac_from_file(const char* filename, 
                      TACInstruction** instructions, 
                      uint32_t* count);

#endif // TEST_COMMON_H
