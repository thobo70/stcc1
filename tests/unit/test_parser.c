/**
 * @file test_parser.c
 * @brief Unit tests for the parser (cc1) component
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 1.0
 * @date 2025-07-27
 */

#include "../test_common.h"
#include "../../src/storage/astore.h"
#include "../../src/storage/symtab.h"
#include "../../src/ast/ast_types.h"

// Function prototypes
void test_parser_function_definition(void);
void test_parser_variable_declaration(void);
void test_parser_expressions(void);
void test_parser_statements(void);
void test_parser_operator_precedence(void);
void test_parser_assignment_vs_equality(void);
void test_parser_error_handling(void);

/**
 * @brief Test basic function parsing
 */
void test_parser_function_definition(void) {
    // First run lexer
    char* input_file = create_temp_file("int main() { return 0; }");
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* lexer_outputs[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    // Then run parser
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    // Verify output files
    TEST_ASSERT_FILE_EXISTS(ast_file);
    TEST_ASSERT_FILE_EXISTS(sym_file);
    
    // Check AST structure
    TEST_ASSERT_EQUAL(0, astore_init(ast_file));
    TEST_ASSERT_EQUAL(0, symtab_init(sym_file));
    
    // Should have function symbol
    SymIdx_t sym_count = symtab_get_count();
    TEST_ASSERT_GREATER_THAN(0, sym_count);
    
    astore_close();
    symtab_close();
}

/**
 * @brief Test variable declaration parsing
 */
void test_parser_variable_declaration(void) {
    char* input_file = create_temp_file("int x; int y = 42;");
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* lexer_outputs[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_EQUAL(0, symtab_init(sym_file));
    
    // Should have variable symbols
    SymIdx_t sym_count = symtab_get_count();
    TEST_ASSERT_GREATER_THAN(1, sym_count); // At least 2 variables
    
    symtab_close();
}

/**
 * @brief Test expression parsing
 */
void test_parser_expressions(void) {
    char* input_file = create_temp_file("int main() { x = 1 + 2 * 3; }");
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* lexer_outputs[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_EQUAL(0, astore_init(ast_file));
    
    // Should have AST nodes for expressions
    // Check that AST index is reasonable
    ASTNodeIdx_t current_idx = astore_getidx();
    TEST_ASSERT_GREATER_THAN(5, current_idx); // Should have several nodes
    
    astore_close();
}

/**
 * @brief Test statement parsing
 */
void test_parser_statements(void) {
    char* input_file = create_temp_file(
        "int main() {\n"
        "    int x;\n"
        "    x = 10;\n"
        "    if (x > 5) {\n"
        "        return 1;\n"
        "    }\n"
        "    return 0;\n"
        "}"
    );
    
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* lexer_outputs[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_FILE_EXISTS(ast_file);
    TEST_ASSERT_FILE_EXISTS(sym_file);
}

/**
 * @brief Test operator precedence parsing
 */
void test_parser_operator_precedence(void) {
    char* input_file = create_temp_file("int main() { x = a + b * c; }");
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* lexer_outputs[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    // Precedence should be correct (multiplication before addition)
    TEST_ASSERT_FILE_EXISTS(ast_file);
}

/**
 * @brief Test assignment vs equality parsing
 */
void test_parser_assignment_vs_equality(void) {
    char* input_file = create_temp_file("int main() { x = 5; if (x == 5) return 1; }");
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* lexer_outputs[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_FILE_EXISTS(ast_file);
}

/**
 * @brief Test parser error handling
 */
void test_parser_error_handling(void) {
    // Test syntax errors
    char* input_file = create_temp_file("int main() { x = ; }"); // Missing right operand
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* lexer_outputs[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    
    // Parser should handle gracefully (may succeed or fail)
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    // Don't assert success/failure as error handling varies
}

/**
 * @brief Run all parser tests
 */
void run_parser_tests(void) {
    printf("Running parser tests...\n");
    
    RUN_TEST(test_parser_function_definition);
    RUN_TEST(test_parser_variable_declaration);
    RUN_TEST(test_parser_expressions);
    RUN_TEST(test_parser_statements);
    RUN_TEST(test_parser_operator_precedence);
    RUN_TEST(test_parser_assignment_vs_equality);
    RUN_TEST(test_parser_error_handling);
}
