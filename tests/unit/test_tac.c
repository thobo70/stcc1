/**
 * @file test_tac.c
 * @brief Unit tests for the Three-Address Code (TAC) generation component
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 1.0
 * @date 2025-07-27
 */

#include "../test_common.h"
#include "../../src/ir/tac_store.h"
#include "../../src/ir/tac_types.h"

/**
 * @brief Test basic TAC initialization and cleanup
 */
void test_tac_init_close(void) {
    char tac_file[] = TEMP_PATH "test_tac.out";
    
    // Initialize TAC store
    int result = tstore_init(tac_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Should start with index 0
    TACIdx_t idx = tstore_current_index();
    TEST_ASSERT_EQUAL(0, idx);
    
    tstore_close();
    
    // File should exist after close
    TEST_ASSERT_FILE_EXISTS(tac_file);
}

/**
 * @brief Test TAC instruction storage and retrieval
 */
void test_tac_instruction_storage(void) {
    char tac_file[] = TEMP_PATH "test_tac.out";
    TEST_ASSERT_EQUAL(0, tstore_init(tac_file));
    
    // Create a simple TAC instruction: t1 = 1 + 2
    TACInstruction_t instr;
    instr.op = TAC_ADD;
    instr.result.type = TAC_OPERAND_TEMP;
    instr.result.data.temp.temp_id = 1;
    instr.operand1.type = TAC_OPERAND_CONSTANT;
    instr.operand1.data.constant.value = 1;
    instr.operand2.type = TAC_OPERAND_CONSTANT;
    instr.operand2.data.constant.value = 2;
    
    // Store the instruction
    TACIdx_t idx = tstore_put(&instr);
    TEST_ASSERT_GREATER_THAN(0, idx);
    
    // Retrieve and verify
    TACInstruction_t* retrieved = tstore_get(idx);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL(TAC_ADD, retrieved->op);
    TEST_ASSERT_EQUAL(TAC_OPERAND_TEMP, retrieved->result.type);
    TEST_ASSERT_EQUAL(1, retrieved->result.data.temp.temp_id);
    TEST_ASSERT_EQUAL(1, retrieved->operand1.data.constant.value);
    TEST_ASSERT_EQUAL(2, retrieved->operand2.data.constant.value);
    
    tstore_close();
}

/**
 * @brief Test full TAC generation from AST
 */
void test_tac_full_generation(void) {
    // Create a simple C program: int main() { x = 1 + 2; return 0; }
    char* input_file = create_temp_file("int main() { x = 1 + 2; return 0; }");
    
    // Run through lexer
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char tokens_file[] = TEMP_PATH "test_tokens.out";
    char* lexer_outputs[] = {sstore_file, tokens_file};
    
    int result = run_compiler_stage("cc0", input_file, lexer_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    // Run through parser
    char ast_file[] = TEMP_PATH "test_ast.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    char* parser_outputs[] = {sstore_file, tokens_file, ast_file, sym_file};
    
    result = run_compiler_stage("cc1", NULL, parser_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    // Run through TAC generator
    char tac_file[] = TEMP_PATH "test_tac.out";
    char* tac_outputs[] = {sstore_file, ast_file, sym_file, tac_file};
    
    result = run_compiler_stage("cc2", NULL, tac_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    // Verify TAC file exists and has content
    TEST_ASSERT_FILE_EXISTS(tac_file);
    
    // Load and verify TAC instructions
    TEST_ASSERT_EQUAL(0, tstore_init(tac_file));
    
    TACIdx_t instruction_count = tstore_current_index();
    TEST_ASSERT_GREATER_THAN(2, instruction_count); // Should have at least a few instructions
    
    // Check first instruction (should be arithmetic)
    TACInstruction_t* first_instr = tstore_get(1);
    TEST_ASSERT_NOT_NULL(first_instr);
    
    tstore_close();
}

/**
 * @brief Test TAC operand types
 */
void test_tac_operand_types(void) {
    char tac_file[] = TEMP_PATH "test_tac.out";
    TEST_ASSERT_EQUAL(0, tstore_init(tac_file));
    
    // Test constant operand
    TACInstruction_t const_instr;
    const_instr.op = TAC_ASSIGN;
    const_instr.result.type = TAC_OPERAND_TEMP;
    const_instr.result.data.temp.temp_id = 1;
    const_instr.operand1.type = TAC_OPERAND_CONSTANT;
    const_instr.operand1.data.constant.value = 42;
    const_instr.operand2.type = TAC_OPERAND_NONE;
    
    TACIdx_t const_idx = tstore_put(&const_instr);
    
    // Test variable operand
    TACInstruction_t var_instr;
    var_instr.op = TAC_ASSIGN;
    var_instr.result.type = TAC_OPERAND_VAR;
    var_instr.result.data.variable.var_id = 2;
    var_instr.operand1.type = TAC_OPERAND_TEMP;
    var_instr.operand1.data.temp.temp_id = 1;
    var_instr.operand2.type = TAC_OPERAND_NONE;
    
    TACIdx_t var_idx = tstore_put(&var_instr);
    
    // Verify instructions
    TACInstruction_t* const_retrieved = tstore_get(const_idx);
    TEST_ASSERT_EQUAL(TAC_OPERAND_CONSTANT, const_retrieved->operand1.type);
    TEST_ASSERT_EQUAL(42, const_retrieved->operand1.data.constant.value);
    
    TACInstruction_t* var_retrieved = tstore_get(var_idx);
    TEST_ASSERT_EQUAL(TAC_OPERAND_VAR, var_retrieved->result.type);
    TEST_ASSERT_EQUAL(2, var_retrieved->result.data.variable.var_id);
    
    tstore_close();
}

/**
 * @brief Test TAC operations
 */
void test_tac_operations(void) {
    char tac_file[] = TEMP_PATH "test_tac.out";
    TEST_ASSERT_EQUAL(0, tstore_init(tac_file));
    
    // Test different operations
    TACOperation_t ops[] = {TAC_ADD, TAC_SUB, TAC_MUL, TAC_DIV, TAC_ASSIGN, TAC_RETURN};
    int num_ops = sizeof(ops) / sizeof(ops[0]);
    
    for (int i = 0; i < num_ops; i++) {
        TACInstruction_t instr;
        instr.op = ops[i];
        instr.result.type = TAC_OPERAND_TEMP;
        instr.result.data.temp.temp_id = i + 1;
        instr.operand1.type = TAC_OPERAND_CONSTANT;
        instr.operand1.data.constant.value = i * 10;
        instr.operand2.type = TAC_OPERAND_CONSTANT;
        instr.operand2.data.constant.value = i * 5;
        
        TACIdx_t idx = tstore_put(&instr);
        
        TACInstruction_t* retrieved = tstore_get(idx);
        TEST_ASSERT_EQUAL(ops[i], retrieved->op);
    }
    
    tstore_close();
}

/**
 * @brief Test TAC assignment handling
 */
void test_tac_assignment(void) {
    // Test assignment: x = 5
    char* input_file = create_temp_file("int main() { x = 5; }");
    
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
    
    char tac_file[] = TEMP_PATH "test_tac.out";
    char* tac_outputs[] = {sstore_file, ast_file, sym_file, tac_file};
    
    result = run_compiler_stage("cc2", NULL, tac_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_EQUAL(0, tstore_init(tac_file));
    
    // Should have assignment instruction
    TACIdx_t count = tstore_current_index();
    TEST_ASSERT_GREATER_THAN(0, count);
    
    tstore_close();
}

/**
 * @brief Test TAC arithmetic expression handling
 */
void test_tac_arithmetic_expressions(void) {
    // Test expression: x = 1 + 2 * 3
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
    
    char tac_file[] = TEMP_PATH "test_tac.out";
    char* tac_outputs[] = {sstore_file, ast_file, sym_file, tac_file};
    
    result = run_compiler_stage("cc2", NULL, tac_outputs);
    TEST_ASSERT_EQUAL(0, result);
    
    TEST_ASSERT_EQUAL(0, tstore_init(tac_file));
    
    // Should have multiple instructions for the expression
    TACIdx_t count = tstore_current_index();
    TEST_ASSERT_GREATER_THAN(2, count); // At least multiplication, addition, assignment
    
    tstore_close();
}

/**
 * @brief Run all TAC tests
 */
void run_tac_tests(void) {
    printf("Running TAC tests...\n");
    
    RUN_TEST(test_tac_init_close);
    RUN_TEST(test_tac_instruction_storage);
    RUN_TEST(test_tac_full_generation);
    RUN_TEST(test_tac_operand_types);
    RUN_TEST(test_tac_operations);
    RUN_TEST(test_tac_assignment);
    RUN_TEST(test_tac_arithmetic_expressions);
}
