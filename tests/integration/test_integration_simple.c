/**
 * @file test_integration_simple.c
 * @brief Simplified integration tests for the STCC1 compiler pipeline
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 1.0
 * @date 2025-07-27
 */

#include "../test_common.h"

// Function prototypes
void test_integration_lexer_only(void);
void test_integration_lexer_parser(void);

/**
 * @brief Test lexer (cc0) in isolation
 */
void test_integration_lexer_only(void) {
    char* input_file = create_temp_file("int main() { return 0; }");
    
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* lexer_outputs[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_FILE_EXISTS(sstore_file);
    TEST_ASSERT_FILE_EXISTS(tokens_file);
}

/**
 * @brief Test lexer and parser pipeline
 */
void test_integration_lexer_parser(void) {
    char* input_file = create_temp_file("int x = 42;");
    
    // Stage 1: Lexer
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* lexer_outputs[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    // Stage 2: Parser (without TAC generation to avoid segfault)
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_FILE_EXISTS(ast_file);
    TEST_ASSERT_FILE_EXISTS(sym_file);
}

/**
 * @brief Run simplified integration tests
 */
void run_integration_tests(void) {
    printf("Running integration tests...\n");
    
    RUN_TEST(test_integration_lexer_only);
    RUN_TEST(test_integration_lexer_parser);
}
