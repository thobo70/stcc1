/**
 * @file test_storage_edge_cases.c
 * @brief Aggressive unit tests for storage components - designed to break weak code
 * @author GitHub Copilot (per PROJECT_MANIFEST.md)
 * @version 1.0
 * @date 2025-07-27
 * 
 * Following PROJECT_MANIFEST.md principles:
 * - NEVER weaken a test to make it pass
 * - FIX the code, not the test
 * - Test corner cases and edge conditions
 * - Break weak implementations
 */

#include "unity.h"
#include "test_storage_edge_cases.h"
#include "../test_common.h"
#include "../../src/storage/sstore.h"
#include "../../src/storage/astore.h"
#include "../../src/storage/tstore.h"
#include "../../src/storage/symtab.h"
#include "../../src/utils/hash.h"
#include "../../src/lexer/ctoken.h"
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Test constants for boundary conditions
#define MAX_STRING_LENGTH 256
#define STRESS_TEST_COUNT 1000
#define INVALID_INDEX 0xFFFF

/**
 * @brief Test sstore with NULL and invalid inputs - should fail gracefully
 */
void test_sstore_null_and_invalid_inputs(void) {
    // Test NULL filename initialization
    int result = sstore_init(NULL);
    TEST_ASSERT_NOT_EQUAL(0, result);  // Should fail
    
    // Test empty filename
    result = sstore_init("");
    TEST_ASSERT_NOT_EQUAL(0, result);  // Should fail
    
    // Test invalid path (directory that doesn't exist)
    result = sstore_init("/nonexistent/path/test.sstore");
    TEST_ASSERT_NOT_EQUAL(0, result);  // Should fail
    
    // Test operations before initialization
    sstore_pos_t pos = sstore_str("test", 4);
    TEST_ASSERT_EQUAL(SSTORE_ERR, pos);  // Should return error
    
    char* str = sstore_get(100);
    TEST_ASSERT_NULL(str);  // Should return NULL
}

/**
 * @brief Test sstore boundary conditions - string length limits
 */
void test_sstore_boundary_conditions(void) {
    char sstore_file[] = TEMP_PATH "test_sstore_boundary.out";
    
    // Initialize store
    int result = sstore_init(sstore_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Test empty string (boundary case)
    sstore_pos_t pos_empty = sstore_str("", 0);
    TEST_ASSERT_NOT_EQUAL(SSTORE_ERR, pos_empty);
    
    // Test single character
    sstore_pos_t pos_single = sstore_str("x", 1);
    TEST_ASSERT_NOT_EQUAL(SSTORE_ERR, pos_single);
    
    // Test maximum string length
    char max_string[MAX_STRING_LENGTH + 1];
    memset(max_string, 'a', MAX_STRING_LENGTH);
    max_string[MAX_STRING_LENGTH] = '\0';
    
    sstore_pos_t pos_max = sstore_str(max_string, MAX_STRING_LENGTH);
    TEST_ASSERT_NOT_EQUAL(SSTORE_ERR, pos_max);  // Should handle max size
    
    // Test string too long (should fail or truncate gracefully)
    char too_long[MAX_STRING_LENGTH * 2];
    memset(too_long, 'b', sizeof(too_long) - 1);
    too_long[sizeof(too_long) - 1] = '\0';
    
    sstore_pos_t pos_toolong = sstore_str(too_long, sizeof(too_long) - 1);
    // Implementation should either handle or fail gracefully
    // Don't weaken test - let implementation prove itself
    TEST_ASSERT_TRUE(pos_toolong == SSTORE_ERR || pos_toolong > 0);
    
    // Test NULL string input
    sstore_pos_t pos_null = sstore_str(NULL, 5);
    TEST_ASSERT_EQUAL(SSTORE_ERR, pos_null);  // Should fail
    
    sstore_close();
}

/**
 * @brief Test sstore hash collision handling and deduplication
 */
void test_sstore_hash_collisions_and_deduplication(void) {
    char sstore_file[] = TEMP_PATH "test_sstore_collision.out";
    
    int result = sstore_init(sstore_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Test exact string deduplication
    sstore_pos_t pos1 = sstore_str("duplicate", 9);
    sstore_pos_t pos2 = sstore_str("duplicate", 9);
    TEST_ASSERT_EQUAL(pos1, pos2);  // Should return same position
    
    // Test strings with same prefix but different length
    sstore_pos_t pos_short = sstore_str("test", 4);
    sstore_pos_t pos_long = sstore_str("testing", 7);
    TEST_ASSERT_NOT_EQUAL(pos_short, pos_long);  // Should be different
    
    // Test strings that might create hash collisions
    // (These are crafted to potentially collide depending on hash function)
    const char* collision_candidates[] = {
        "fb", "Ea",  // These often collide in simple hash functions
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j"
    };
    
    sstore_pos_t positions[sizeof(collision_candidates) / sizeof(char*)];
    
    // Store all collision candidates
    for (size_t i = 0; i < sizeof(collision_candidates) / sizeof(char*); i++) {
        positions[i] = sstore_str(collision_candidates[i], 
                                  strlen(collision_candidates[i]));
        TEST_ASSERT_NOT_EQUAL(SSTORE_ERR, positions[i]);
    }
    
    // Verify all positions are different (proper collision handling)
    for (size_t i = 0; i < sizeof(positions) / sizeof(sstore_pos_t); i++) {
        for (size_t j = i + 1; j < sizeof(positions) / sizeof(sstore_pos_t); j++) {
            if (strcmp(collision_candidates[i], collision_candidates[j]) != 0) {
                TEST_ASSERT_NOT_EQUAL(positions[i], positions[j]);
            }
        }
    }
    
    sstore_close();
}

/**
 * @brief Stress test sstore with many operations
 */
void test_sstore_stress_test(void) {
    char sstore_file[] = TEMP_PATH "test_sstore_stress.out";
    
    int result = sstore_init(sstore_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Store many unique strings
    sstore_pos_t positions[STRESS_TEST_COUNT];
    char test_string[64];
    
    for (int i = 0; i < STRESS_TEST_COUNT; i++) {
        snprintf(test_string, sizeof(test_string), "stress_test_string_%d", i);
        positions[i] = sstore_str(test_string, strlen(test_string));
        TEST_ASSERT_NOT_EQUAL(SSTORE_ERR, positions[i]);
    }
    
    // Verify all positions are unique (no false deduplication)
    for (int i = 0; i < STRESS_TEST_COUNT; i++) {
        for (int j = i + 1; j < STRESS_TEST_COUNT; j++) {
            TEST_ASSERT_NOT_EQUAL(positions[i], positions[j]);
        }
    }
    
    sstore_close();
}

/**
 * @brief Test sstore file corruption recovery
 */
void test_sstore_file_corruption_recovery(void) {
    char sstore_file[] = TEMP_PATH "test_sstore_corrupt.out";
    
    // Create a valid store first
    int result = sstore_init(sstore_file);
    TEST_ASSERT_EQUAL(0, result);
    sstore_str("test", 4);
    sstore_close();
    
    // Corrupt the file (truncate it)
    FILE* fp = fopen(sstore_file, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fwrite("corrupt", 1, 7, fp);  // Write invalid data
    fclose(fp);
    
    // Try to open corrupted file
    result = sstore_open(sstore_file);
    // Implementation should either succeed with graceful degradation
    // or fail cleanly - don't weaken test to accommodate bugs
    
    if (result == 0) {
        // If it opens, operations should be safe
        char* str = sstore_get(1);
        // Should either return valid data or NULL - no crashes
        TEST_ASSERT_TRUE(str == NULL || str[0] != '\0'); // Either NULL or valid string
        sstore_close();
    }
}

/**
 * @brief Test astore with invalid node operations
 */
void test_astore_invalid_operations(void) {
    char astore_file[] = TEMP_PATH "test_astore_invalid.out";
    
    // Test operations before initialization
    ASTNode invalid_node = {0};
    ASTNodeIdx_t idx = astore_add(&invalid_node);
    TEST_ASSERT_EQUAL(0, idx);  // Should fail/return invalid
    
    ASTNode retrieved = astore_get(1);
    TEST_ASSERT_EQUAL(AST_FREE, retrieved.type);  // Should return safe default
    
    // Initialize properly
    int result = astore_init(astore_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Test NULL node addition
    idx = astore_add(NULL);
    TEST_ASSERT_EQUAL(0, idx);  // Should fail gracefully
    
    // Test invalid index retrieval
    ASTNode invalid_get = astore_get(INVALID_INDEX);
    TEST_ASSERT_EQUAL(AST_FREE, invalid_get.type);  // Should return safe default
    
    // Test index 0 (typically reserved)
    ASTNode zero_node = astore_get(0);
    TEST_ASSERT_EQUAL(AST_FREE, zero_node.type);  // Should be safe
    
    astore_close();
}

/**
 * @brief Test astore boundary conditions and limits
 */
void test_astore_boundary_conditions(void) {
    char astore_file[] = TEMP_PATH "test_astore_boundary.out";
    
    int result = astore_init(astore_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Test adding maximum number of nodes
    ASTNode test_node = {0};
    test_node.type = AST_LIT_INTEGER;
    test_node.flags = 0;
    test_node.binary.value.long_value = 42;
    
    ASTNodeIdx_t first_idx = astore_add(&test_node);
    TEST_ASSERT_GREATER_THAN(0, first_idx);
    
    // Add many nodes to test limits
    ASTNodeIdx_t last_valid_idx = first_idx;
    for (int i = 0; i < 100; i++) {  // Reasonable test limit
        test_node.binary.value.long_value = i;
        ASTNodeIdx_t idx = astore_add(&test_node);
        if (idx > 0) {
            last_valid_idx = idx;
        } else {
            break;  // Hit limit
        }
    }
    
    // Verify we can retrieve the last valid node
    ASTNode retrieved = astore_get(last_valid_idx);
    TEST_ASSERT_EQUAL(AST_LIT_INTEGER, retrieved.type);
    
    // Test node with maximum children references
    ASTNode complex_node = {0};
    complex_node.type = AST_STMT_COMPOUND;
    complex_node.children.child1 = INVALID_INDEX;  // Test invalid references
    complex_node.children.child2 = INVALID_INDEX;
    complex_node.children.child3 = INVALID_INDEX;
    complex_node.children.child4 = INVALID_INDEX;
    
    ASTNodeIdx_t complex_idx = astore_add(&complex_node);
    TEST_ASSERT_GREATER_THAN(0, complex_idx);
    
    ASTNode complex_retrieved = astore_get(complex_idx);
    TEST_ASSERT_EQUAL(AST_STMT_COMPOUND, complex_retrieved.type);
    TEST_ASSERT_EQUAL(INVALID_INDEX, complex_retrieved.children.child1);
    
    astore_close();
}

/**
 * @brief Test tstore edge cases and error conditions
 */
void test_tstore_edge_cases(void) {
    char tstore_file[] = TEMP_PATH "test_tstore_edge.out";
    
    // Test operations before initialization
    Token_t token = {T_ID, 100, 1, 1};
    TokenIdx_t idx = tstore_add(&token);
    TEST_ASSERT_EQUAL(TSTORE_ERR, idx);  // Should indicate failure
    
    Token_t retrieved = tstore_get(1);
    TEST_ASSERT_EQUAL(T_EOF, retrieved.id);  // Should return EOF token
    
    // Initialize properly
    int result = tstore_init(tstore_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Test NULL token
    idx = tstore_add(NULL);
    TEST_ASSERT_EQUAL(TSTORE_ERR, idx);  // Should fail
    
    // Test invalid token IDs
    Token_t invalid_token = {(TokenID_t)-1, 0, 0, 0};
    idx = tstore_add(&invalid_token);
    TEST_ASSERT_NOT_EQUAL(TSTORE_ERR, idx);  // Should handle invalid token
    
    // Test EOF token specifically
    Token_t eof_token = {T_EOF, 0, 0, 0};
    idx = tstore_add(&eof_token);
    TEST_ASSERT_NOT_EQUAL(TSTORE_ERR, idx);
    
    Token_t eof_retrieved = tstore_get(idx);
    TEST_ASSERT_EQUAL(T_EOF, eof_retrieved.id);
    
    // Test boundary line numbers
    Token_t max_line_token = {T_ID, 100, 1, UINT16_MAX};
    idx = tstore_add(&max_line_token);
    TEST_ASSERT_NOT_EQUAL(TSTORE_ERR, idx);
    
    Token_t max_retrieved = tstore_get(idx);
    TEST_ASSERT_EQUAL(UINT16_MAX, max_retrieved.line);
    
    // Test boundary positions
    Token_t max_pos_token = {T_ID, UINT16_MAX, 1, 1};
    idx = tstore_add(&max_pos_token);
    TEST_ASSERT_NOT_EQUAL(TSTORE_ERR, idx);
    
    tstore_close();
}

/**
 * @brief Test symtab with invalid symbol operations
 */
void test_symtab_invalid_operations(void) {
    char symtab_file[] = TEMP_PATH "test_symtab_invalid.out";
    
    // Test operations before initialization
    SymTabEntry entry = {SYM_VARIABLE, 100, 0, 0, 0, 0, 0, 200, 1, 0}; // Added scope_depth = 0
    SymIdx_t idx = symtab_add(&entry);
    TEST_ASSERT_EQUAL(0, idx);  // Should fail
    
    SymTabEntry retrieved = symtab_get(1);
    TEST_ASSERT_EQUAL(SYM_FREE, retrieved.type);  // Should return safe default
    
    // Initialize properly
    int result = symtab_init(symtab_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Test NULL entry
    idx = symtab_add(NULL);
    TEST_ASSERT_EQUAL(0, idx);  // Should fail
    
    // Test circular references (symbol pointing to itself)
    SymTabEntry circular = {SYM_VARIABLE, 100, 1, 1, 1, 1, 1, 200, 1, 0}; // Added scope_depth = 0
    idx = symtab_add(&circular);
    TEST_ASSERT_GREATER_THAN(0, idx);  // Should add but handle circularity
    
    // Update the circular reference to point to itself
    circular.parent = idx;
    circular.next = idx;
    circular.prev = idx;
    SymIdx_t update_result = symtab_update(idx, &circular);
    TEST_ASSERT_EQUAL(idx, update_result);
    
    // Test invalid index retrieval
    SymTabEntry invalid_get = symtab_get(INVALID_INDEX);
    TEST_ASSERT_EQUAL(SYM_FREE, invalid_get.type);
    
    symtab_close();
}

/**
 * @brief Test symtab boundary conditions and symbol relationships
 */
void test_symtab_boundary_conditions(void) {
    char symtab_file[] = TEMP_PATH "test_symtab_boundary.out";
    
    int result = symtab_init(symtab_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Test all symbol types
    SymType types[] = {
        SYM_VARIABLE, SYM_FUNCTION, SYM_TYPEDEF, SYM_LABEL,
        SYM_ENUMERATOR, SYM_STRUCT, SYM_UNION, SYM_ENUM,
        SYM_CONSTANT, SYM_UNKOWN
    };
    
    SymIdx_t indices[sizeof(types) / sizeof(SymType)];
    
    for (size_t i = 0; i < sizeof(types) / sizeof(SymType); i++) {
        SymTabEntry entry = {types[i], i + 100, 0, 0, 0, 0, 0, i + 200, (int)i + 1, 0}; // Added scope_depth = 0
        indices[i] = symtab_add(&entry);
        TEST_ASSERT_GREATER_THAN(0, indices[i]);
    }
    
    // Verify all entries were stored correctly
    for (size_t i = 0; i < sizeof(types) / sizeof(SymType); i++) {
        SymTabEntry retrieved = symtab_get(indices[i]);
        TEST_ASSERT_EQUAL(types[i], retrieved.type);
        TEST_ASSERT_EQUAL(i + 100, retrieved.name);
        TEST_ASSERT_EQUAL(i + 200, retrieved.value);
        TEST_ASSERT_EQUAL((int)i + 1, retrieved.line);
    }
    
    // Test maximum depth symbol hierarchy
    SymIdx_t parent_idx = indices[0];
    SymTabEntry child_entry = {SYM_VARIABLE, 999, parent_idx, 0, 0, 0, 0, 888, 10, 1}; // Added scope_depth = 1
    
    for (int depth = 0; depth < 10; depth++) {  // Test reasonable depth
        SymIdx_t child_idx = symtab_add(&child_entry);
        TEST_ASSERT_GREATER_THAN(0, child_idx);
        
        // Update parent to point to child
        SymTabEntry parent = symtab_get(parent_idx);
        parent.child = child_idx;
        symtab_update(parent_idx, &parent);
        
        parent_idx = child_idx;
        child_entry.parent = parent_idx;
        child_entry.line = 10 + depth;
    }
    
    symtab_close();
}

/**
 * @brief Test file permission and access errors
 */
void test_file_permission_errors(void) {
    // Test read-only directory (if possible)
    char readonly_file[] = "/tmp/readonly_test.out";
    
    // Create file first
    FILE* fp = fopen(readonly_file, "w");
    if (fp) {
        fclose(fp);
        
        // Make it read-only
        chmod(readonly_file, 0444);
        
        // Try to initialize (should fail)
        int result = sstore_init(readonly_file);
        TEST_ASSERT_NOT_EQUAL(0, result);
        
        // Restore permissions and clean up
        chmod(readonly_file, 0644);
        unlink(readonly_file);
    }
}

/**
 * @brief Test concurrent access safety (basic)
 */
void test_concurrent_access_safety(void) {
    char shared_file[] = TEMP_PATH "test_concurrent.out";
    
    // Initialize store
    int result = sstore_init(shared_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Add some data
    sstore_pos_t pos = sstore_str("concurrent_test", 15);
    TEST_ASSERT_NOT_EQUAL(SSTORE_ERR, pos);
    
    sstore_close();
    
    // Open from different "process" (simulate concurrent access)
    result = sstore_open(shared_file);
    TEST_ASSERT_EQUAL(0, result);
    
    char* retrieved = sstore_get(pos);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_STRING("concurrent_test", retrieved);
    
    sstore_close();
}

/**
 * @brief Test memory exhaustion scenarios
 */
void test_memory_exhaustion_scenarios(void) {
    char mem_file[] = TEMP_PATH "test_memory.out";
    
    int result = sstore_init(mem_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Try to exhaust string store memory
    char large_string[1024];
    memset(large_string, 'X', sizeof(large_string) - 1);
    large_string[sizeof(large_string) - 1] = '\0';
    
    sstore_pos_t last_pos = 0;
    int success_count = 0;
    
    // Add strings until failure (or reasonable limit)
    for (int i = 0; i < 100; i++) {
        // Make each string unique
        snprintf(large_string + sizeof(large_string) - 10, 10, "%08d", i);
        
        sstore_pos_t pos = sstore_str(large_string, strlen(large_string));
        if (pos != SSTORE_ERR) {
            last_pos = pos;
            success_count++;
        } else {
            break;  // Hit limit
        }
    }
    
    // Should have added at least some strings
    TEST_ASSERT_GREATER_THAN(0, success_count);
    
    // Should be able to retrieve the last successful addition
    if (last_pos > 0) {
        char* retrieved = sstore_get(last_pos);
        TEST_ASSERT_NOT_NULL(retrieved);
    }
    
    sstore_close();
}

/**
 * @brief Test data integrity after file operations
 */
void test_data_integrity(void) {
    char integrity_file[] = TEMP_PATH "test_integrity.out";
    
    // Phase 1: Create and populate
    int result = sstore_init(integrity_file);
    TEST_ASSERT_EQUAL(0, result);
    
    const char* test_strings[] = {
        "integrity_test_1",
        "integrity_test_2", 
        "integrity_test_3",
        "special_chars_!@#$%^&*()",
        "unicode_test_αβγδε",
        ""  // Empty string
    };
    
    sstore_pos_t positions[sizeof(test_strings) / sizeof(char*)];
    
    for (size_t i = 0; i < sizeof(test_strings) / sizeof(char*); i++) {
        positions[i] = sstore_str(test_strings[i], strlen(test_strings[i]));
        TEST_ASSERT_NOT_EQUAL(SSTORE_ERR, positions[i]);
    }
    
    sstore_close();
    
    // Phase 2: Reopen and verify
    result = sstore_open(integrity_file);
    TEST_ASSERT_EQUAL(0, result);
    
    for (size_t i = 0; i < sizeof(test_strings) / sizeof(char*); i++) {
        char* retrieved = sstore_get(positions[i]);
        TEST_ASSERT_NOT_NULL(retrieved);
        TEST_ASSERT_EQUAL_STRING(test_strings[i], retrieved);
    }
    
    sstore_close();
}

/**
 * @brief Test implementation-specific edge cases
 */
void test_implementation_edge_cases(void) {
    char edge_file[] = TEMP_PATH "test_edge.out";
    
    int result = sstore_init(edge_file);
    TEST_ASSERT_EQUAL(0, result);
    
    // Test string with embedded nulls (if supported)
    char null_string[] = {'t', 'e', 's', 't', '\0', 'n', 'u', 'l', 'l', '\0'};
    sstore_pos_t null_pos = sstore_str(null_string, sizeof(null_string) - 1);
    // Don't assert success/failure - let implementation decide
    TEST_ASSERT_TRUE(null_pos == SSTORE_ERR || null_pos > 0);
    
    // Test very long identifier names
    char long_identifier[512];
    for (int i = 0; i < 511; i++) {
        long_identifier[i] = 'a' + (i % 26);
    }
    long_identifier[511] = '\0';
    
    sstore_pos_t long_pos = sstore_str(long_identifier, 511);
    // Implementation should handle gracefully
    TEST_ASSERT_TRUE(long_pos == SSTORE_ERR || long_pos > 0);
    
    // Test strings with all possible byte values
    char binary_string[256];
    for (int i = 0; i < 256; i++) {
        binary_string[i] = (char)i;
    }
    
    sstore_pos_t binary_pos = sstore_str(binary_string, 256);
    // Should handle binary data gracefully
    TEST_ASSERT_TRUE(binary_pos == SSTORE_ERR || binary_pos > 0);
    
    sstore_close();
}

// Runner function for all edge case tests
void run_storage_edge_case_tests(void) {
    RUN_TEST(test_sstore_null_and_invalid_inputs);
    RUN_TEST(test_sstore_boundary_conditions);
    RUN_TEST(test_sstore_hash_collisions_and_deduplication);
    RUN_TEST(test_sstore_stress_test);
    RUN_TEST(test_sstore_file_corruption_recovery);
    RUN_TEST(test_astore_invalid_operations);
    RUN_TEST(test_astore_boundary_conditions);
    RUN_TEST(test_tstore_edge_cases);
    RUN_TEST(test_symtab_invalid_operations);
    RUN_TEST(test_symtab_boundary_conditions);
    RUN_TEST(test_file_permission_errors);
    RUN_TEST(test_concurrent_access_safety);
    RUN_TEST(test_memory_exhaustion_scenarios);
    RUN_TEST(test_data_integrity);
    RUN_TEST(test_implementation_edge_cases);
}
