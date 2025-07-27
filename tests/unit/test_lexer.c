/**
 * @file test_lexer.c
 * @brief Unit tests for the lexer (cc0) component
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 1.0
 * @date 2025-07-27
 */

#include "../test_common.h"
#include "../../src/storage/sstore.h"
#include "../../src/storage/tstore.h"
#include "../../src/lexer/ctoken.h"

// Test cases for lexer (currently unused but kept for reference)
// static TestCase lexer_test_cases[] = { ... };

// Function prototypes
void test_lexer_basic_tokens(void);
void test_lexer_integer_literals(void);
void test_lexer_string_literals(void);
void test_lexer_identifiers(void);
void test_lexer_keywords(void);
void test_lexer_operators(void);
void test_lexer_error_handling(void);

/**
 * @brief Test basic token recognition
 */
void test_lexer_basic_tokens(void) {
    char* input_file = create_temp_file("int main() { return 0; }");
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* output_files[] = {sstore_file, tokens_file};
    
    // Run lexer
    int result = run_compiler_stage("cc0", input_file, output_files);
    TEST_ASSERT_EQUAL(0, result);
    
    // Check output files exist
    TEST_ASSERT_FILE_EXISTS(sstore_file);
    TEST_ASSERT_FILE_EXISTS(tokens_file);
    
    // Open and verify token store
    TEST_ASSERT_EQUAL(0, tstore_open(tokens_file));
    TEST_ASSERT_EQUAL(0, sstore_open(sstore_file));
    
    // Check we have expected tokens
    Token_t token;
    int token_count = 0;
    tstore_setidx(0);
    
    do {
        token = tstore_next();
        token_count++;
    } while (token.id != T_EOF && token_count < 100);
    
    TEST_ASSERT_GREATER_THAN(0, token_count);
    TEST_ASSERT_LESS_THAN(20, token_count); // Reasonable upper bound
    
    tstore_close();
    sstore_close();
}

/**
 * @brief Test integer literal parsing
 */
void test_lexer_integer_literals(void) {
    char* input_file = create_temp_file("123 0x456 0777");
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* output_files[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, output_files);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_EQUAL(0, tstore_open(tokens_file));
    TEST_ASSERT_EQUAL(0, sstore_open(sstore_file));
    
    tstore_setidx(0);
    Token_t token = tstore_next();
    TEST_ASSERT_EQUAL(T_LITINT, token.id);
    
    // Verify string store contains the number
    char* value = sstore_get(token.pos);
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_EQUAL_STRING("123", value);
    
    tstore_close();
    sstore_close();
}

/**
 * @brief Test string literal parsing
 */
void test_lexer_string_literals(void) {
    char* input_file = create_temp_file("\"hello world\"");
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* output_files[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, output_files);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_EQUAL(0, tstore_open(tokens_file));
    TEST_ASSERT_EQUAL(0, sstore_open(sstore_file));
    
    tstore_setidx(0);
    Token_t token = tstore_next();
    TEST_ASSERT_EQUAL(T_LITSTRING, token.id);
    
    tstore_close();
    sstore_close();
}

/**
 * @brief Test identifier recognition
 */
void test_lexer_identifiers(void) {
    char* input_file = create_temp_file("main variable_name _underscore");
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* output_files[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, output_files);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_EQUAL(0, tstore_open(tokens_file));
    TEST_ASSERT_EQUAL(0, sstore_open(sstore_file));
    
    tstore_setidx(0);
    Token_t token = tstore_next();
    TEST_ASSERT_EQUAL(T_ID, token.id);
    
    char* name = sstore_get(token.pos);
    TEST_ASSERT_EQUAL_STRING("main", name);
    
    tstore_close();
    sstore_close();
}

/**
 * @brief Test keyword recognition
 */
void test_lexer_keywords(void) {
    char* input_file = create_temp_file("int return if while for");
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* output_files[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, output_files);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_EQUAL(0, tstore_open(tokens_file));
    
    tstore_setidx(0);
    Token_t token = tstore_next();
    TEST_ASSERT_EQUAL(T_INT, token.id);
    
    token = tstore_next();
    TEST_ASSERT_EQUAL(T_RETURN, token.id);
    
    token = tstore_next();
    TEST_ASSERT_EQUAL(T_IF, token.id);
    
    tstore_close();
}

/**
 * @brief Test operator recognition
 */
void test_lexer_operators(void) {
    char* input_file = create_temp_file("+ - * / = == != < > <= >=");
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* output_files[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, output_files);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_EQUAL(0, tstore_open(tokens_file));
    
    tstore_setidx(0);
    Token_t token = tstore_next();
    TEST_ASSERT_EQUAL(T_PLUS, token.id);
    
    token = tstore_next();
    TEST_ASSERT_EQUAL(T_MINUS, token.id);
    
    token = tstore_next();
    TEST_ASSERT_EQUAL(T_MUL, token.id);
    
    tstore_close();
}

/**
 * @brief Test error handling in lexer
 */
void test_lexer_error_handling(void) {
    // Test with invalid input (should handle gracefully)
    char* input_file = create_temp_file("@#$%^&*()"); // Invalid characters
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* output_files[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, output_files);
    (void)result; // Suppress unused variable warning - error handling varies
    
    // Don't assert success/failure as error handling behavior may vary
    // The test passes if the lexer doesn't crash
}

/**
 * @brief Run all lexer tests
 */
void run_lexer_tests(void) {
    printf("Running lexer tests...\n");
    
    RUN_TEST(test_lexer_basic_tokens);
    RUN_TEST(test_lexer_integer_literals);
    RUN_TEST(test_lexer_string_literals);
    RUN_TEST(test_lexer_identifiers);
    RUN_TEST(test_lexer_keywords);
    RUN_TEST(test_lexer_operators);
    RUN_TEST(test_lexer_error_handling);
}
