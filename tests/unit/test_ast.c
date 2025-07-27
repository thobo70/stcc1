/**
 * @file test_ast.c
 * @brief Unit tests for the AST (Abstract Syntax Tree) component
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 1.0
 * @date 2025-07-27
 */

#include "../test_common.h"
#include "../../src/storage/astore.h"
#include "../../src/ast/ast_types.h"

/**
 * @brief Test basic AST initialization and cleanup
 */
void test_ast_init_close(void) {
    char ast_file[] = TEMP_PATH "test_ast.out";
    
    // Initialize new AST store
    int result = astore_init(ast_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Should start with index 0
    ASTNodeIdx_t idx = astore_current_index();
    TEST_ASSERT_EQUAL(0, idx);
    
    astore_close();
    
    // File should exist after close
    TEST_ASSERT_FILE_EXISTS(ast_file);
}

/**
 * @brief Test AST node storage and retrieval
 */
void test_ast_node_storage(void) {
    char ast_file[] = TEMP_PATH "test_ast.out";
    TEST_ASSERT_EQUAL(0, astore_init(ast_file));
    
    // Create a simple expression node
    ASTNode_t node;
    node.node_type = AST_EXPR_BINARY_OP;
    node.data.binary_op.op = TOKEN_PLUS;
    node.data.binary_op.left = 0;  // Will be updated
    node.data.binary_op.right = 0; // Will be updated
    
    // Store the node
    ASTNodeIdx_t idx = astore_put(&node);
    TEST_ASSERT_GREATER_THAN(0, idx);
    
    // Retrieve and verify
    ASTNode_t* retrieved = astore_get(idx);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL(AST_EXPR_BINARY_OP, retrieved->node_type);
    TEST_ASSERT_EQUAL(TOKEN_PLUS, retrieved->data.binary_op.op);
    
    astore_close();
}

/**
 * @brief Test different AST node types
 */
void test_ast_node_types(void) {
    char ast_file[] = TEMP_PATH "test_ast.out";
    TEST_ASSERT_EQUAL(0, astore_init(ast_file));
    
    // Test identifier node
    ASTNode_t id_node;
    id_node.node_type = AST_IDENTIFIER;
    id_node.data.identifier.symbol_idx = 1;
    ASTNodeIdx_t id_idx = astore_put(&id_node);
    
    // Test literal node
    ASTNode_t lit_node;
    lit_node.node_type = AST_LITERAL_INT;
    lit_node.data.literal_int.value = 42;
    ASTNodeIdx_t lit_idx = astore_put(&lit_node);
    
    // Test assignment node
    ASTNode_t assign_node;
    assign_node.node_type = AST_EXPR_ASSIGN;
    assign_node.data.assignment.left = id_idx;
    assign_node.data.assignment.right = lit_idx;
    ASTNodeIdx_t assign_idx = astore_put(&assign_node);
    
    // Verify all nodes
    ASTNode_t* id_retrieved = astore_get(id_idx);
    TEST_ASSERT_EQUAL(AST_IDENTIFIER, id_retrieved->node_type);
    TEST_ASSERT_EQUAL(1, id_retrieved->data.identifier.symbol_idx);
    
    ASTNode_t* lit_retrieved = astore_get(lit_idx);
    TEST_ASSERT_EQUAL(AST_LITERAL_INT, lit_retrieved->node_type);
    TEST_ASSERT_EQUAL(42, lit_retrieved->data.literal_int.value);
    
    ASTNode_t* assign_retrieved = astore_get(assign_idx);
    TEST_ASSERT_EQUAL(AST_EXPR_ASSIGN, assign_retrieved->node_type);
    TEST_ASSERT_EQUAL(id_idx, assign_retrieved->data.assignment.left);
    TEST_ASSERT_EQUAL(lit_idx, assign_retrieved->data.assignment.right);
    
    astore_close();
}

/**
 * @brief Test AST with binary operations
 */
void test_ast_binary_operations(void) {
    char ast_file[] = TEMP_PATH "test_ast.out";
    TEST_ASSERT_EQUAL(0, astore_init(ast_file));
    
    // Create expression: 1 + 2
    ASTNode_t left_lit;
    left_lit.node_type = AST_LITERAL_INT;
    left_lit.data.literal_int.value = 1;
    ASTNodeIdx_t left_idx = astore_put(&left_lit);
    
    ASTNode_t right_lit;
    right_lit.node_type = AST_LITERAL_INT;
    right_lit.data.literal_int.value = 2;
    ASTNodeIdx_t right_idx = astore_put(&right_lit);
    
    ASTNode_t binop;
    binop.node_type = AST_EXPR_BINARY_OP;
    binop.data.binary_op.op = TOKEN_PLUS;
    binop.data.binary_op.left = left_idx;
    binop.data.binary_op.right = right_idx;
    ASTNodeIdx_t binop_idx = astore_put(&binop);
    
    // Verify structure
    ASTNode_t* binop_retrieved = astore_get(binop_idx);
    TEST_ASSERT_EQUAL(AST_EXPR_BINARY_OP, binop_retrieved->node_type);
    TEST_ASSERT_EQUAL(TOKEN_PLUS, binop_retrieved->data.binary_op.op);
    
    ASTNode_t* left_retrieved = astore_get(binop_retrieved->data.binary_op.left);
    ASTNode_t* right_retrieved = astore_get(binop_retrieved->data.binary_op.right);
    
    TEST_ASSERT_EQUAL(1, left_retrieved->data.literal_int.value);
    TEST_ASSERT_EQUAL(2, right_retrieved->data.literal_int.value);
    
    astore_close();
}

/**
 * @brief Test AST with function definitions
 */
void test_ast_function_definition(void) {
    char ast_file[] = TEMP_PATH "test_ast.out";
    TEST_ASSERT_EQUAL(0, astore_init(ast_file));
    
    // Create function node
    ASTNode_t func_node;
    func_node.node_type = AST_FUNCTION_DEF;
    func_node.data.function_def.name_symbol_idx = 1; // "main"
    func_node.data.function_def.return_type = TYPE_INT;
    func_node.data.function_def.param_list = 0; // No parameters
    func_node.data.function_def.body = 0; // No body for this test
    
    ASTNodeIdx_t func_idx = astore_put(&func_node);
    
    // Verify function node
    ASTNode_t* func_retrieved = astore_get(func_idx);
    TEST_ASSERT_EQUAL(AST_FUNCTION_DEF, func_retrieved->node_type);
    TEST_ASSERT_EQUAL(1, func_retrieved->data.function_def.name_symbol_idx);
    TEST_ASSERT_EQUAL(TYPE_INT, func_retrieved->data.function_def.return_type);
    
    astore_close();
}

/**
 * @brief Test AST current index tracking
 */
void test_ast_index_tracking(void) {
    char ast_file[] = TEMP_PATH "test_ast.out";
    TEST_ASSERT_EQUAL(0, astore_init(ast_file));
    
    ASTNodeIdx_t initial_idx = astore_current_index();
    TEST_ASSERT_EQUAL(0, initial_idx);
    
    // Add several nodes
    for (int i = 0; i < 5; i++) {
        ASTNode_t node;
        node.node_type = AST_LITERAL_INT;
        node.data.literal_int.value = i;
        astore_put(&node);
    }
    
    ASTNodeIdx_t final_idx = astore_current_index();
    TEST_ASSERT_EQUAL(5, final_idx);
    
    astore_close();
}

/**
 * @brief Test AST persistence across sessions
 */
void test_ast_persistence(void) {
    char ast_file[] = TEMP_PATH "test_ast.out";
    
    // First session - create and store nodes
    TEST_ASSERT_EQUAL(0, astore_init(ast_file));
    
    ASTNode_t node;
    node.node_type = AST_LITERAL_INT;
    node.data.literal_int.value = 123;
    ASTNodeIdx_t idx = astore_put(&node);
    
    astore_close();
    
    // Second session - reload and verify
    TEST_ASSERT_EQUAL(0, astore_init(ast_file));
    
    ASTNode_t* retrieved = astore_get(idx);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL(AST_LITERAL_INT, retrieved->node_type);
    TEST_ASSERT_EQUAL(123, retrieved->data.literal_int.value);
    
    astore_close();
}

/**
 * @brief Run all AST tests
 */
void run_ast_tests(void) {
    printf("Running AST tests...\n");
    
    RUN_TEST(test_ast_init_close);
    RUN_TEST(test_ast_node_storage);
    RUN_TEST(test_ast_node_types);
    RUN_TEST(test_ast_binary_operations);
    RUN_TEST(test_ast_function_definition);
    RUN_TEST(test_ast_index_tracking);
    RUN_TEST(test_ast_persistence);
}
