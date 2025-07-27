/**
 * @file test_storage_edge_cases.h
 * @brief Header for storage edge case tests - aggressive testing to break weak implementations
 * 
 * Following PROJECT_MANIFEST.md principle: "FIX the code, not the test"
 * These tests are designed to stress-test storage components with extreme conditions.
 */

#ifndef TEST_STORAGE_EDGE_CASES_H
#define TEST_STORAGE_EDGE_CASES_H

// Storage edge case test function prototypes
void test_sstore_null_and_invalid_inputs(void);
void test_sstore_boundary_conditions(void);
void test_sstore_hash_collisions_and_deduplication(void);
void test_sstore_stress_test(void);
void test_sstore_file_corruption_recovery(void);
void test_astore_invalid_operations(void);
void test_astore_boundary_conditions(void);
void test_tstore_edge_cases(void);
void test_symtab_invalid_operations(void);
void test_symtab_boundary_conditions(void);
void test_file_permission_errors(void);
void test_concurrent_access_safety(void);
void test_memory_exhaustion_scenarios(void);
void test_data_integrity(void);
void test_implementation_edge_cases(void);

// Main test runner
void run_storage_edge_case_tests(void);

#endif // TEST_STORAGE_EDGE_CASES_H
