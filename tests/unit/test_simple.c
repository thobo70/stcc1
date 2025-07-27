/**
 * @file test_simple.c
 * @brief Simplified unit tests for STCC1 compiler components
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 1.0
 * @date 2025-07-27
 */

#include "../test_common.h"

// Function prototypes
void test_basic_functionality(void);
void test_file_creation(void);
void test_compiler_stages(void);

/**
 * @brief Test basic functionality
 */
void test_basic_functionality(void) {
    TEST_ASSERT_EQUAL(1, 1);
    TEST_ASSERT_TRUE(1);
    TEST_ASSERT_FALSE(0);
}

/**
 * @brief Test file creation utilities
 */
void test_file_creation(void) {
    char* temp_file = create_temp_file("test content");
    TEST_ASSERT_NOT_NULL(temp_file);
    
    char* content = read_file_content(temp_file);
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_EQUAL_STRING("test content", content);
    
    free(content);
}

/**
 * @brief Test compiler stage execution
 */
void test_compiler_stages(void) {
    // Test lexer (cc0)
    char* input_file = create_temp_file("int main() { return 0; }");
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* output_files[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, output_files);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_FILE_EXISTS(sstore_file);
    TEST_ASSERT_FILE_EXISTS(tokens_file);
}

/**
 * @brief Run all simple tests
 */
void run_simple_tests(void) {
    printf("Running simple tests...\n");
    
    RUN_TEST(test_basic_functionality);
    RUN_TEST(test_file_creation);
    RUN_TEST(test_compiler_stages);
}
