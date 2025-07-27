/**
 * @file test_hmapbuf_edge_cases.h
 * @brief Header for hmapbuf edge case tests - memory management stress testing
 * 
 * Following PROJECT_MANIFEST.md principle: "FIX the code, not the test"
 * These tests target weaknesses in the LRU buffer memory management system.
 */

#ifndef TEST_HMAPBUF_EDGE_CASES_H
#define TEST_HMAPBUF_EDGE_CASES_H

// HMapBuf edge case test function prototypes
void test_hmapbuf_init_cleanup(void);
void test_hmapbuf_invalid_modes(void);
void test_hmapbuf_node_exhaustion(void);
void test_hmapbuf_lru_behavior(void);
void test_hmapbuf_hash_collisions(void);
void test_hmapbuf_modification_tracking(void);
void test_hmapbuf_null_pointers(void);
void test_hmapbuf_mixed_modes(void);
void test_hmapbuf_rapid_allocation(void);
void test_hmapbuf_index_boundaries(void);
void test_hmapbuf_memory_corruption(void);
void test_hmapbuf_thread_safety_basics(void);

// Main test runner
void run_hmapbuf_edge_case_tests(void);

#endif // TEST_HMAPBUF_EDGE_CASES_H
