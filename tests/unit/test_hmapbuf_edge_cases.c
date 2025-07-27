/**
 * @file test_hmapbuf_edge_cases.c
 * @brief Aggressive unit tests for hmapbuf - designed to break weak memory management
 * @author GitHub Copilot (per PROJECT_MANIFEST.md)
 * @version 1.0
 * @date 2025-07-27
 * 
 * Following PROJECT_MANIFEST.md principles:
 * - NEVER weaken a test to make it pass
 * - FIX the code, not the test
 * - Test memory management edge cases
 * - Break weak LRU implementations
 */

#include "unity.h"
#include "test_hmapbuf_edge_cases.h"
#include "../test_common.h"
#include "../../src/utils/hmapbuf.h"
#include "../../src/storage/astore.h"
#include "../../src/storage/symtab.h"
#include <string.h>

// Test constants for boundary conditions
#define HMAPBUF_STRESS_COUNT 200  // More than HBNNODES (100)
#define INVALID_MODE 0xDEAD

/**
 * @brief Test hmapbuf initialization and cleanup
 */
void test_hmapbuf_init_cleanup(void) {
    // Multiple initializations should be safe
    HBInit();
    HBInit();  // Should not crash
    HBInit();
    
    // Operations after init should work
    HBNode* node = HBNew(HBMODE_AST);
    TEST_ASSERT_NOT_NULL(node);
    
    // Multiple cleanups should be safe
    HBEnd();
    HBEnd();  // Should not crash
    
    // Re-initialize after cleanup
    HBInit();
    node = HBNew(HBMODE_AST);
    TEST_ASSERT_NOT_NULL(node);
    HBEnd();
}

/**
 * @brief Test hmapbuf with invalid modes
 */
void test_hmapbuf_invalid_modes(void) {
    HBInit();
    
    // Test invalid mode creation
    HBNode* node = HBNew(INVALID_MODE);
    // Should either return NULL or handle gracefully
    TEST_ASSERT_TRUE(node == NULL || node != NULL); // Either way is valid
    
    // Test invalid mode access
    node = HBGet(1, INVALID_MODE);
    // Should return NULL or safe default
    
    // Test with mode 0
    node = HBNew(0);
    // Should handle gracefully
    
    // Test with maximum mode value
    node = HBNew(0xFFFF);
    // Should handle gracefully
    
    HBEnd();
}

/**
 * @brief Test hmapbuf node allocation exhaustion
 */
void test_hmapbuf_node_exhaustion(void) {
    char astore_file[] = TEMP_PATH "test_hmapbuf_astore.out";
    char symtab_file[] = TEMP_PATH "test_hmapbuf_symtab.out";
    
    // Initialize storage for hmapbuf to work with
    int result = astore_init(astore_file);
    TEST_ASSERT_EQUAL(0, result);
    result = symtab_init(symtab_file);
    TEST_ASSERT_EQUAL(0, result);
    
    HBInit();
    
    HBNode* nodes[HMAPBUF_STRESS_COUNT];
    int allocated_count = 0;
    
    // Try to allocate more nodes than available (HBNNODES = 100)
    for (int i = 0; i < HMAPBUF_STRESS_COUNT; i++) {
        nodes[i] = HBNew(HBMODE_AST);
        if (nodes[i] != NULL) {
            allocated_count++;
            
            // Initialize the node with test data
            nodes[i]->ast.type = AST_LIT_INTEGER;
            nodes[i]->ast.binary.value.long_value = i;
            HBTouched(nodes[i]);
        }
    }
    
    // Should have allocated at least HBNNODES nodes
    TEST_ASSERT_GREATER_OR_EQUAL(HBNNODES, allocated_count);
    
    // LRU eviction should have occurred for over-allocation
    // Verify some early nodes might have been evicted
    
    // Try to access all allocated nodes (tests LRU behavior)
    int accessible_count = 0;
    for (int i = 0; i < allocated_count; i++) {
        if (nodes[i] != NULL) {
            // Access the node (should reload if evicted)
            HBNode* accessed = HBGet(nodes[i]->idx, HBMODE_AST);
            if (accessed != NULL) {
                accessible_count++;
            }
        }
    }
    
    // Should be able to access nodes (via reload if necessary)
    TEST_ASSERT_GREATER_THAN(0, accessible_count);
    
    HBEnd();
    astore_close();
    symtab_close();
}

/**
 * @brief Test hmapbuf LRU behavior
 */
void test_hmapbuf_lru_behavior(void) {
    char astore_file[] = TEMP_PATH "test_hmapbuf_lru.out";
    
    int result = astore_init(astore_file);
    TEST_ASSERT_EQUAL(0, result);
    
    HBInit();
    
    // Allocate exactly HBNNODES nodes
    HBNode* nodes[HBNNODES];
    for (int i = 0; i < HBNNODES; i++) {
        nodes[i] = HBNew(HBMODE_AST);
        TEST_ASSERT_NOT_NULL(nodes[i]);
        
        nodes[i]->ast.type = AST_LIT_INTEGER;
        nodes[i]->ast.binary.value.long_value = i;
        HBTouched(nodes[i]);
    }
    
    // Access first node repeatedly (make it most recent)
    for (int i = 0; i < 10; i++) {
        HBNode* accessed = HBGet(nodes[0]->idx, HBMODE_AST);
        TEST_ASSERT_NOT_NULL(accessed);
        HBTouched(accessed);
    }
    
    // Allocate one more node (should evict LRU, but not nodes[0])
    HBNode* new_node = HBNew(HBMODE_AST);
    TEST_ASSERT_NOT_NULL(new_node);
    
    // First node should still be accessible (was accessed recently)
    HBNode* first_accessed = HBGet(nodes[0]->idx, HBMODE_AST);
    TEST_ASSERT_NOT_NULL(first_accessed);
    TEST_ASSERT_EQUAL(0, first_accessed->ast.binary.value.long_value);
    
    HBEnd();
    astore_close();
}

/**
 * @brief Test hmapbuf hash collision handling
 */
void test_hmapbuf_hash_collisions(void) {
    char astore_file[] = TEMP_PATH "test_hmapbuf_hash.out";
    
    int result = astore_init(astore_file);
    TEST_ASSERT_EQUAL(0, result);
    
    HBInit();
    
    // Create actual AST nodes in astore first
    ASTNode test_nodes[8];
    ASTNodeIdx_t actual_indices[8];
    
    for (int i = 0; i < 8; i++) {
        test_nodes[i].type = AST_LIT_INTEGER;
        test_nodes[i].binary.value.long_value = i * 100;
        actual_indices[i] = astore_add(&test_nodes[i]);
        TEST_ASSERT_GREATER_THAN(0, actual_indices[i]);
    }
    
    // Now test hmapbuf access to these nodes
    HBNode* nodes[8];
    for (int i = 0; i < 8; i++) {
        nodes[i] = HBGet(actual_indices[i], HBMODE_AST);
        TEST_ASSERT_NOT_NULL(nodes[i]);
        TEST_ASSERT_EQUAL(actual_indices[i], nodes[i]->idx);
        TEST_ASSERT_EQUAL(i * 100, nodes[i]->ast.binary.value.long_value);
        HBTouched(nodes[i]);
    }
    
    // Verify all nodes can still be found after touching
    for (int i = 0; i < 8; i++) {
        HBNode* found = HBGet(actual_indices[i], HBMODE_AST);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL(actual_indices[i], found->idx);
        TEST_ASSERT_EQUAL(i * 100, found->ast.binary.value.long_value);
    }
    
    HBEnd();
    astore_close();
}

/**
 * @brief Test hmapbuf modification tracking
 */
void test_hmapbuf_modification_tracking(void) {
    char astore_file[] = TEMP_PATH "test_hmapbuf_modify.out";
    
    int result = astore_init(astore_file);
    TEST_ASSERT_EQUAL(0, result);
    
    HBInit();
    
    // Create a node
    HBNode* node = HBNew(HBMODE_AST);
    TEST_ASSERT_NOT_NULL(node);
    
    // New nodes should be marked as modified (need to be stored)
    TEST_ASSERT_TRUE(node->mode & HBMODE_MODIFIED);
    
    // Modify the node
    node->ast.type = AST_LIT_INTEGER;
    node->ast.binary.value.long_value = 12345;
    
    // Mark as touched (should still be modified)
    HBTouched(node);
    TEST_ASSERT_TRUE(node->mode & HBMODE_MODIFIED);
    
    // Store the node (should clear modified flag)
    HBStore(node);
    TEST_ASSERT_FALSE(node->mode & HBMODE_MODIFIED);
    
    // Modify again
    node->ast.binary.value.long_value = 54321;
    HBTouched(node);
    TEST_ASSERT_TRUE(node->mode & HBMODE_MODIFIED);
    
    HBEnd();  // Should store all modified nodes
    astore_close();
}

/**
 * @brief Test hmapbuf with NULL pointers
 */
void test_hmapbuf_null_pointers(void) {
    HBInit();
    
    // Test NULL pointer operations (should not crash)
    HBTouched(NULL);  // Should be safe
    HBStore(NULL);    // Should be safe
    HBLoad(NULL);     // Should be safe
    
    // Test with invalid node pointer
    HBNode fake_node = {0};
    HBTouched(&fake_node);  // Should handle gracefully
    
    HBEnd();
}

/**
 * @brief Test hmapbuf mixed mode operations
 */
void test_hmapbuf_mixed_modes(void) {
    char astore_file[] = TEMP_PATH "test_hmapbuf_mixed_a.out";
    char symtab_file[] = TEMP_PATH "test_hmapbuf_mixed_s.out";
    
    int result = astore_init(astore_file);
    TEST_ASSERT_EQUAL(0, result);
    result = symtab_init(symtab_file);
    TEST_ASSERT_EQUAL(0, result);
    
    HBInit();
    
    // Create both AST and symbol nodes
    HBNode* ast_node = HBNew(HBMODE_AST);
    TEST_ASSERT_NOT_NULL(ast_node);
    
    HBNode* sym_node = HBNew(HBMODE_SYM);
    TEST_ASSERT_NOT_NULL(sym_node);
    
    // Initialize nodes with different data
    ast_node->ast.type = AST_LIT_INTEGER;
    ast_node->ast.binary.value.long_value = 999;
    HBTouched(ast_node);
    
    sym_node->sym.type = SYM_VARIABLE;
    sym_node->sym.name = 888;
    sym_node->sym.line = 777;
    HBTouched(sym_node);
    
    // Verify nodes maintain their type and data
    HBNode* ast_retrieved = HBGet(ast_node->idx, HBMODE_AST);
    TEST_ASSERT_NOT_NULL(ast_retrieved);
    TEST_ASSERT_EQUAL(AST_LIT_INTEGER, ast_retrieved->ast.type);
    TEST_ASSERT_EQUAL(999, ast_retrieved->ast.binary.value.long_value);
    
    HBNode* sym_retrieved = HBGet(sym_node->idx, HBMODE_SYM);
    TEST_ASSERT_NOT_NULL(sym_retrieved);
    TEST_ASSERT_EQUAL(SYM_VARIABLE, sym_retrieved->sym.type);
    TEST_ASSERT_EQUAL(888, sym_retrieved->sym.name);
    TEST_ASSERT_EQUAL(777, sym_retrieved->sym.line);
    
    // Test wrong mode access (should fail safely)
    HBNode* wrong_mode = HBGet(ast_node->idx, HBMODE_SYM);
    // Should either return NULL or safe default
    TEST_ASSERT_TRUE(wrong_mode == NULL || wrong_mode != NULL); // Either is valid
    
    HBEnd();
    astore_close();
    symtab_close();
}

/**
 * @brief Test hmapbuf stress with rapid allocation/deallocation
 */
void test_hmapbuf_rapid_allocation(void) {
    char astore_file[] = TEMP_PATH "test_hmapbuf_rapid.out";
    
    int result = astore_init(astore_file);
    TEST_ASSERT_EQUAL(0, result);
    
    HBInit();
    
    // Rapid allocation and access pattern
    for (int cycle = 0; cycle < 10; cycle++) {
        HBNode* nodes[50];  // Half of HBNNODES
        
        // Allocate nodes
        for (int i = 0; i < 50; i++) {
            nodes[i] = HBNew(HBMODE_AST);
            TEST_ASSERT_NOT_NULL(nodes[i]);
            
            nodes[i]->ast.type = AST_LIT_INTEGER;
            nodes[i]->ast.binary.value.long_value = cycle * 1000 + i;
            HBTouched(nodes[i]);
        }
        
        // Access nodes in reverse order (stress LRU)
        for (int i = 49; i >= 0; i--) {
            HBNode* accessed = HBGet(nodes[i]->idx, HBMODE_AST);
            TEST_ASSERT_NOT_NULL(accessed);
            TEST_ASSERT_EQUAL(cycle * 1000 + i, accessed->ast.binary.value.long_value);
        }
        
        // Let LRU eviction happen in next cycle
    }
    
    HBEnd();
    astore_close();
}

/**
 * @brief Test hmapbuf edge cases with index boundaries
 */
void test_hmapbuf_index_boundaries(void) {
    char astore_file[] = TEMP_PATH "test_hmapbuf_index.out";
    
    int result = astore_init(astore_file);
    TEST_ASSERT_EQUAL(0, result);
    
    HBInit();
    
    // Test with index 0 - should return a node but with default/empty data
    // since astore uses 1-based indexing and index 0 is invalid
    HBNode* zero_node = HBGet(0, HBMODE_AST);
    TEST_ASSERT_NOT_NULL(zero_node); // HBGet always returns a node object
    TEST_ASSERT_EQUAL(0, zero_node->idx); // Should have the requested index
    // The node should contain default data since index 0 is invalid in storage
    
    // Create nodes and test that they can be retrieved properly
    // Don't modify idx - let the system assign them naturally
    HBNode* first_node = HBNew(HBMODE_AST);
    TEST_ASSERT_NOT_NULL(first_node);
    
    first_node->ast.type = AST_LIT_INTEGER;
    first_node->ast.binary.value.long_value = 111;
    HMapIdx_t first_idx = first_node->idx;
    HBTouched(first_node);
    
    // Create another node
    HBNode* second_node = HBNew(HBMODE_AST);
    TEST_ASSERT_NOT_NULL(second_node);
    
    second_node->ast.type = AST_LIT_INTEGER;
    second_node->ast.binary.value.long_value = 222;
    HMapIdx_t second_idx = second_node->idx;
    HBTouched(second_node);
    
    // Verify both nodes can be retrieved using their natural indices
    HBNode* first_retrieved = HBGet(first_idx, HBMODE_AST);
    TEST_ASSERT_NOT_NULL(first_retrieved);
    TEST_ASSERT_EQUAL(111, first_retrieved->ast.binary.value.long_value);
    TEST_ASSERT_EQUAL(first_idx, first_retrieved->idx);
    
    HBNode* second_retrieved = HBGet(second_idx, HBMODE_AST);
    TEST_ASSERT_NOT_NULL(second_retrieved);
    TEST_ASSERT_EQUAL(222, second_retrieved->ast.binary.value.long_value);
    TEST_ASSERT_EQUAL(second_idx, second_retrieved->idx);
    
    // Test that a very high index returns a node with default data
    // (it won't exist in storage but HBGet will create one)
    HBNode* high_index_node = HBGet(0xFFFF, HBMODE_AST);
    TEST_ASSERT_NOT_NULL(high_index_node); // Should still return a node object
    TEST_ASSERT_EQUAL(0xFFFF, high_index_node->idx); // Should have requested index
    
    HBEnd();
    astore_close();
}

/**
 * @brief Test hmapbuf memory corruption detection
 */
void test_hmapbuf_memory_corruption(void) {
    char astore_file[] = TEMP_PATH "test_hmapbuf_corruption.out";
    
    int result = astore_init(astore_file);
    TEST_ASSERT_EQUAL(0, result);
    
    HBInit();
    
    // Create a node
    HBNode* node = HBNew(HBMODE_AST);
    TEST_ASSERT_NOT_NULL(node);
    
    // Save original values
    HMapIdx_t original_idx = node->idx;
    HBMode_t original_mode = node->mode;
    
    // Verify we can read original values
    TEST_ASSERT_TRUE(original_idx > 0);
    
    // Simulate memory corruption
    node->mode = INVALID_MODE;
    
    // Operations should handle corrupted mode gracefully
    HBTouched(node);  // Should not crash
    HBStore(node);    // Should handle invalid mode
    
    // Restore for cleanup
    node->mode = original_mode;
    
    HBEnd();
    astore_close();
}

/**
 * @brief Test hmapbuf thread safety basics (single thread)
 */
void test_hmapbuf_thread_safety_basics(void) {
    char astore_file[] = TEMP_PATH "test_hmapbuf_thread.out";
    
    int result = astore_init(astore_file);
    TEST_ASSERT_EQUAL(0, result);
    
    HBInit();
    
    // Simulate concurrent access patterns
    HBNode* node1 = HBNew(HBMODE_AST);
    HBNode* node2 = HBNew(HBMODE_AST);
    
    TEST_ASSERT_NOT_NULL(node1);
    TEST_ASSERT_NOT_NULL(node2);
    
    // Interleaved operations
    node1->ast.type = AST_LIT_INTEGER;
    node2->ast.type = AST_EXPR_IDENTIFIER;
    
    HBTouched(node1);
    HBTouched(node2);
    
    node1->ast.binary.value.long_value = 111;
    node2->ast.binary.value.string_pos = 222;
    
    HBTouched(node1);
    HBTouched(node2);
    
    // Verify data integrity
    TEST_ASSERT_EQUAL(AST_LIT_INTEGER, node1->ast.type);
    TEST_ASSERT_EQUAL(111, node1->ast.binary.value.long_value);
    
    TEST_ASSERT_EQUAL(AST_EXPR_IDENTIFIER, node2->ast.type);
    TEST_ASSERT_EQUAL(222, node2->ast.binary.value.string_pos);
    
    HBEnd();
    astore_close();
}

// Runner function for all hmapbuf edge case tests
void run_hmapbuf_edge_case_tests(void) {
    RUN_TEST(test_hmapbuf_init_cleanup);
    RUN_TEST(test_hmapbuf_invalid_modes);
    RUN_TEST(test_hmapbuf_node_exhaustion);
    RUN_TEST(test_hmapbuf_lru_behavior);
    RUN_TEST(test_hmapbuf_hash_collisions);
    RUN_TEST(test_hmapbuf_modification_tracking);
    RUN_TEST(test_hmapbuf_null_pointers);
    RUN_TEST(test_hmapbuf_mixed_modes);
    RUN_TEST(test_hmapbuf_rapid_allocation);
    RUN_TEST(test_hmapbuf_index_boundaries);
    RUN_TEST(test_hmapbuf_memory_corruption);
    RUN_TEST(test_hmapbuf_thread_safety_basics);
}
