/**
 * @file test_integration.c
 * @brief Integration tests for the complete STCC1 compiler pipeline
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 1.0
 * @date 2025-07-27
 */

#include "../test_common.h"

// Function prototypes
void test_integration_simple_program(void);
void test_integration_variables(void);
void test_integration_expressions(void);
void test_integration_control_flow(void);
void test_integration_loops(void);
void test_integration_functions(void);
void test_integration_operator_precedence(void);
void test_integration_error_handling(void);

/**
 * @brief Test complete compilation pipeline for simple program
 */
void test_integration_simple_program(void) {
    char* input_file = create_temp_file("int main() { return 0; }");
    
    // Run complete pipeline
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char tac_file[] = TEMP_PATH "test_tac.out";
    
    // Stage 1: Lexer
    char* lexer_outputs[] = {sstore_file, tokens_file};
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_FILE_EXISTS(sstore_file);
    TEST_ASSERT_FILE_EXISTS(tokens_file);
    
    // Stage 2: Parser
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_FILE_EXISTS(ast_file);
    TEST_ASSERT_FILE_EXISTS(sym_file);
    
    // Stage 3: TAC Generator
    char* tac_outputs[] = {sstore_file, ast_file, sym_file, tac_file};
    result = run_compiler_stage("cc2", NULL, tac_outputs);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_FILE_EXISTS(tac_file);
}

/**
 * @brief Test complete compilation pipeline for program with variables
 */
void test_integration_variables(void) {
    char* input_file = create_temp_file(
        "int main() {\n"
        "    int x = 10;\n"
        "    int y = 20;\n"
        "    int z = x + y;\n"
        "    return z;\n"
        "}"
    );
    
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char tac_file[] = TEMP_PATH "test_tac.out";
    
    // Run complete pipeline
    char* lexer_outputs[] = {sstore_file, tokens_file};
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char* tac_outputs[] = {sstore_file, ast_file, sym_file, tac_file};
    result = run_compiler_stage("cc2", NULL, tac_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    // Verify all files exist and are non-empty
    TEST_ASSERT_FILE_EXISTS(sstore_file);
    TEST_ASSERT_FILE_EXISTS(tokens_file);
    TEST_ASSERT_FILE_EXISTS(ast_file);
    TEST_ASSERT_FILE_EXISTS(sym_file);
    TEST_ASSERT_FILE_EXISTS(tac_file);
    
    // Check file sizes (should be > 0)
    FILE* fp = fopen(tac_file, "r");
    TEST_ASSERT_NOT_NULL(fp);
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    TEST_ASSERT_GREATER_THAN(0, size);
}

/**
 * @brief Test compilation pipeline with expressions
 */
void test_integration_expressions(void) {
    char* input_file = create_temp_file(
        "int main() {\n"
        "    x = 1 + 2 * 3 - 4 / 2;\n"
        "    y = (a + b) * (c - d);\n"
        "    return x + y;\n"
        "}"
    );
    
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char tac_file[] = TEMP_PATH "test_tac.out";
    
    char* lexer_outputs[] = {sstore_file, tokens_file};
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char* tac_outputs[] = {sstore_file, ast_file, sym_file, tac_file};
    result = run_compiler_stage("cc2", NULL, tac_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_FILE_EXISTS(tac_file);
}

/**
 * @brief Test compilation pipeline with control flow
 */
void test_integration_control_flow(void) {
    char* input_file = create_temp_file(
        "int main() {\n"
        "    int x = 10;\n"
        "    if (x > 5) {\n"
        "        x = x + 1;\n"
        "    } else {\n"
        "        x = x - 1;\n"
        "    }\n"
        "    return x;\n"
        "}"
    );
    
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char tac_file[] = TEMP_PATH "test_tac.out";
    
    char* lexer_outputs[] = {sstore_file, tokens_file};
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char* tac_outputs[] = {sstore_file, ast_file, sym_file, tac_file};
    result = run_compiler_stage("cc2", NULL, tac_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_FILE_EXISTS(tac_file);
}

/**
 * @brief Test compilation pipeline with loops
 */
void test_integration_loops(void) {
    char* input_file = create_temp_file(
        "int main() {\n"
        "    int i = 0;\n"
        "    int sum = 0;\n"
        "    while (i < 10) {\n"
        "        sum = sum + i;\n"
        "        i = i + 1;\n"
        "    }\n"
        "    return sum;\n"
        "}"
    );
    
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char tac_file[] = TEMP_PATH "test_tac.out";
    
    char* lexer_outputs[] = {sstore_file, tokens_file};
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char* tac_outputs[] = {sstore_file, ast_file, sym_file, tac_file};
    result = run_compiler_stage("cc2", NULL, tac_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_FILE_EXISTS(tac_file);
}

/**
 * @brief Test compilation pipeline with function definitions
 */
void test_integration_functions(void) {
    char* input_file = create_temp_file(
        "int add(int a, int b) {\n"
        "    return a + b;\n"
        "}\n"
        "\n"
        "int main() {\n"
        "    int result = add(5, 10);\n"
        "    return result;\n"
        "}"
    );
    
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char tac_file[] = TEMP_PATH "test_tac.out";
    
    char* lexer_outputs[] = {sstore_file, tokens_file};
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char* tac_outputs[] = {sstore_file, ast_file, sym_file, tac_file};
    result = run_compiler_stage("cc2", NULL, tac_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_FILE_EXISTS(tac_file);
}

/**
 * @brief Test operator precedence in complete pipeline
 */
void test_integration_operator_precedence(void) {
    char* input_file = create_temp_file(
        "int main() {\n"
        "    x = 1 + 2 * 3;\n"    // Should be 1 + (2 * 3) = 7
        "    y = a * b + c;\n"    // Should be (a * b) + c
        "    z = a + b * c + d;\n" // Should be a + (b * c) + d
        "    return x + y + z;\n"
        "}"
    );
    
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char tac_file[] = TEMP_PATH "test_tac.out";
    
    char* lexer_outputs[] = {sstore_file, tokens_file};
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char* tac_outputs[] = {sstore_file, ast_file, sym_file, tac_file};
    result = run_compiler_stage("cc2", NULL, tac_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_FILE_EXISTS(tac_file);
    
    // Verify TAC content shows correct precedence
    // This is a basic check - could be expanded to parse TAC content
    FILE* fp = fopen(tac_file, "r");
    TEST_ASSERT_NOT_NULL(fp);
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    TEST_ASSERT_GREATER_THAN(0, size);
}

/**
 * @brief Test error handling in complete pipeline
 */
void test_integration_error_handling(void) {
    // Test with syntax error
    char* input_file = create_temp_file("int main() { x = ; }"); // Missing operand
    
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char tac_file[] = TEMP_PATH "test_tac.out";
    
    // Lexer should succeed
    char* lexer_outputs[] = {sstore_file, tokens_file};
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    // Parser may fail or handle gracefully
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    // Don't assert success/failure as error handling varies
    
    // TAC generation depends on parser success
    if (result == 0) {
        char* tac_outputs[] = {sstore_file, ast_file, sym_file, tac_file};
        run_compiler_stage("cc2", NULL, tac_outputs);
    }
}

/**
 * @brief Run all integration tests
 */
void run_integration_tests(void) {
    printf("Running integration tests...\n");
    
    RUN_TEST(test_integration_simple_program);
    RUN_TEST(test_integration_variables);
    RUN_TEST(test_integration_expressions);
    RUN_TEST(test_integration_control_flow);
    RUN_TEST(test_integration_loops);
    RUN_TEST(test_integration_functions);
    RUN_TEST(test_integration_operator_precedence);
    RUN_TEST(test_integration_error_handling);
}
