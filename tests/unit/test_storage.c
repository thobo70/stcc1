/**
 * @file test_storage.c
 * @brief Unit tests for the storage components (sstore, astore, tstore, symtab)
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 1.0
 * @date 2025-07-27
 */

#include "../test_common.h"
#include "../../src/storage/sstore.h"
#include "../../src/storage/astore.h"
#include "../../src/storage/tstore.h"
#include "../../src/storage/symtab.h"

/**
 * @brief Test string store (sstore) functionality
 */
void test_sstore_functionality(void) {
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    
    // Initialize string store
    int result = sstore_init(sstore_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Add strings
    StringPos_t pos1 = sstore_put("hello");
    StringPos_t pos2 = sstore_put("world");
    StringPos_t pos3 = sstore_put("test");
    
    TEST_ASSERT_GREATER_THAN(0, pos1);
    TEST_ASSERT_GREATER_THAN(pos1, pos2);
    TEST_ASSERT_GREATER_THAN(pos2, pos3);
    
    // Retrieve strings
    char* str1 = sstore_get(pos1);
    char* str2 = sstore_get(pos2);
    char* str3 = sstore_get(pos3);
    
    TEST_ASSERT_EQUAL_STRING("hello", str1);
    TEST_ASSERT_EQUAL_STRING("world", str2);
    TEST_ASSERT_EQUAL_STRING("test", str3);
    
    sstore_close();
    
    // Test persistence
    TEST_ASSERT_EQUAL(0, sstore_init(sstore_file));
    
    char* persistent_str = sstore_get(pos1);
    TEST_ASSERT_EQUAL_STRING("hello", persistent_str);
    
    sstore_close();
}

/**
 * @brief Test symbol table functionality
 */
void test_symtab_functionality(void) {
    char sym_file[] = TEMP_PATH "test_sym.out";
    
    // Initialize symbol table
    int result = symtab_init(sym_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Add symbols
    Symbol_t sym1;
    sym1.name_pos = 1; // Position in string store
    sym1.symbol_type = SYMBOL_VARIABLE;
    sym1.data_type = TYPE_INT;
    sym1.scope_level = 0;
    
    SymIdx_t idx1 = symtab_put(&sym1);
    TEST_ASSERT_GREATER_THAN(0, idx1);
    
    Symbol_t sym2;
    sym2.name_pos = 2;
    sym2.symbol_type = SYMBOL_FUNCTION;
    sym2.data_type = TYPE_INT;
    sym2.scope_level = 0;
    
    SymIdx_t idx2 = symtab_put(&sym2);
    TEST_ASSERT_GREATER_THAN(idx1, idx2);
    
    // Retrieve symbols
    Symbol_t* retrieved1 = symtab_get(idx1);
    Symbol_t* retrieved2 = symtab_get(idx2);
    
    TEST_ASSERT_NOT_NULL(retrieved1);
    TEST_ASSERT_NOT_NULL(retrieved2);
    
    TEST_ASSERT_EQUAL(SYMBOL_VARIABLE, retrieved1->symbol_type);
    TEST_ASSERT_EQUAL(SYMBOL_FUNCTION, retrieved2->symbol_type);
    TEST_ASSERT_EQUAL(TYPE_INT, retrieved1->data_type);
    TEST_ASSERT_EQUAL(TYPE_INT, retrieved2->data_type);
    
    // Test count
    SymIdx_t count = symtab_count();
    TEST_ASSERT_EQUAL(2, count);
    
    symtab_close();
}

/**
 * @brief Test AST store functionality
 */
void test_astore_functionality(void) {
    char ast_file[] = TEMP_PATH "test_ast.out";
    
    TEST_ASSERT_EQUAL(0, astore_init(ast_file));
    
    // Create nodes
    ASTNode_t node1;
    node1.node_type = AST_LITERAL_INT;
    node1.data.literal_int.value = 123;
    
    ASTNode_t node2;
    node2.node_type = AST_IDENTIFIER;
    node2.data.identifier.symbol_idx = 1;
    
    // Store nodes
    ASTNodeIdx_t idx1 = astore_put(&node1);
    ASTNodeIdx_t idx2 = astore_put(&node2);
    
    TEST_ASSERT_GREATER_THAN(0, idx1);
    TEST_ASSERT_GREATER_THAN(idx1, idx2);
    
    // Retrieve nodes
    ASTNode_t* retrieved1 = astore_get(idx1);
    ASTNode_t* retrieved2 = astore_get(idx2);
    
    TEST_ASSERT_EQUAL(AST_LITERAL_INT, retrieved1->node_type);
    TEST_ASSERT_EQUAL(123, retrieved1->data.literal_int.value);
    
    TEST_ASSERT_EQUAL(AST_IDENTIFIER, retrieved2->node_type);
    TEST_ASSERT_EQUAL(1, retrieved2->data.identifier.symbol_idx);
    
    // Test current index
    ASTNodeIdx_t current = astore_current_index();
    TEST_ASSERT_EQUAL(2, current);
    
    astore_close();
}

/**
 * @brief Test TAC store functionality
 */
void test_tstore_functionality(void) {
    char tac_file[] = TEMP_PATH "test_tac.out";
    
    TEST_ASSERT_EQUAL(0, tstore_init(tac_file));
    
    // Create TAC instructions
    TACInstruction_t instr1;
    instr1.op = TAC_ADD;
    instr1.result.type = TAC_OPERAND_TEMP;
    instr1.result.data.temp.temp_id = 1;
    instr1.operand1.type = TAC_OPERAND_CONSTANT;
    instr1.operand1.data.constant.value = 10;
    instr1.operand2.type = TAC_OPERAND_CONSTANT;
    instr1.operand2.data.constant.value = 20;
    
    TACInstruction_t instr2;
    instr2.op = TAC_ASSIGN;
    instr2.result.type = TAC_OPERAND_VAR;
    instr2.result.data.variable.var_id = 2;
    instr2.operand1.type = TAC_OPERAND_TEMP;
    instr2.operand1.data.temp.temp_id = 1;
    instr2.operand2.type = TAC_OPERAND_NONE;
    
    // Store instructions
    TACIdx_t idx1 = tstore_put(&instr1);
    TACIdx_t idx2 = tstore_put(&instr2);
    
    TEST_ASSERT_GREATER_THAN(0, idx1);
    TEST_ASSERT_GREATER_THAN(idx1, idx2);
    
    // Retrieve instructions
    TACInstruction_t* retrieved1 = tstore_get(idx1);
    TACInstruction_t* retrieved2 = tstore_get(idx2);
    
    TEST_ASSERT_EQUAL(TAC_ADD, retrieved1->op);
    TEST_ASSERT_EQUAL(10, retrieved1->operand1.data.constant.value);
    TEST_ASSERT_EQUAL(20, retrieved1->operand2.data.constant.value);
    
    TEST_ASSERT_EQUAL(TAC_ASSIGN, retrieved2->op);
    TEST_ASSERT_EQUAL(TAC_OPERAND_VAR, retrieved2->result.type);
    TEST_ASSERT_EQUAL(2, retrieved2->result.data.variable.var_id);
    
    tstore_close();
}

/**
 * @brief Test storage persistence across sessions
 */
void test_storage_persistence(void) {
    char sstore_file[] = TEMP_PATH "test_sstore.out";
    char sym_file[] = TEMP_PATH "test_sym.out";
    
    // First session - store data
    TEST_ASSERT_EQUAL(0, sstore_init(sstore_file));
    StringPos_t str_pos = sstore_put("persistent_string");
    sstore_close();
    
    TEST_ASSERT_EQUAL(0, symtab_init(sym_file));
    Symbol_t sym;
    sym.name_pos = str_pos;
    sym.symbol_type = SYMBOL_VARIABLE;
    sym.data_type = TYPE_INT;
    sym.scope_level = 1;
    SymIdx_t sym_idx = symtab_put(&sym);
    symtab_close();
    
    // Second session - reload and verify
    TEST_ASSERT_EQUAL(0, sstore_init(sstore_file));
    char* retrieved_str = sstore_get(str_pos);
    TEST_ASSERT_EQUAL_STRING("persistent_string", retrieved_str);
    sstore_close();
    
    TEST_ASSERT_EQUAL(0, symtab_init(sym_file));
    Symbol_t* retrieved_sym = symtab_get(sym_idx);
    TEST_ASSERT_NOT_NULL(retrieved_sym);
    TEST_ASSERT_EQUAL(SYMBOL_VARIABLE, retrieved_sym->symbol_type);
    TEST_ASSERT_EQUAL(TYPE_INT, retrieved_sym->data_type);
    TEST_ASSERT_EQUAL(1, retrieved_sym->scope_level);
    symtab_close();
}

/**
 * @brief Test storage error handling
 */
void test_storage_error_handling(void) {
    // Test invalid file paths
    int result = sstore_init("/invalid/path/test.out");
    TEST_ASSERT_NOT_EQUAL(0, result);
    
    result = symtab_init("/invalid/path/test.out");
    TEST_ASSERT_NOT_EQUAL(0, result);
    
    result = astore_init("/invalid/path/test.out");
    TEST_ASSERT_NOT_EQUAL(0, result);
    
    result = tstore_init("/invalid/path/test.out");
    TEST_ASSERT_NOT_EQUAL(0, result);
}

/**
 * @brief Test storage with large amounts of data
 */
void test_storage_large_data(void) {
    char sstore_file[] = TEMP_PATH "test_large_sstore.out";
    
    TEST_ASSERT_EQUAL(0, sstore_init(sstore_file));
    
    // Add many strings
    const int num_strings = 100;
    StringPos_t positions[num_strings];
    char buffer[32];
    
    for (int i = 0; i < num_strings; i++) {
        snprintf(buffer, sizeof(buffer), "string_%d", i);
        positions[i] = sstore_put(buffer);
        TEST_ASSERT_GREATER_THAN(0, positions[i]);
    }
    
    // Verify all strings
    for (int i = 0; i < num_strings; i++) {
        char* retrieved = sstore_get(positions[i]);
        snprintf(buffer, sizeof(buffer), "string_%d", i);
        TEST_ASSERT_EQUAL_STRING(buffer, retrieved);
    }
    
    sstore_close();
}

/**
 * @brief Run all storage tests
 */
void run_storage_tests(void) {
    printf("Running storage tests...\n");
    
    RUN_TEST(test_sstore_functionality);
    RUN_TEST(test_symtab_functionality);
    RUN_TEST(test_astore_functionality);
    RUN_TEST(test_tstore_functionality);
    RUN_TEST(test_storage_persistence);
    RUN_TEST(test_storage_error_handling);
    RUN_TEST(test_storage_large_data);
}
