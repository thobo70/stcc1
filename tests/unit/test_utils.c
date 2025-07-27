/**
 * @file test_utils.c
 * @brief Unit tests for utility components (hash, hmapbuf)
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 1.0
 * @date 2025-07-27
 */

#include "../test_common.h"
#include "../../src/utils/hash.h"
#include "../../src/utils/hmapbuf.h"

/**
 * @brief Test hash function basic functionality
 */
void test_hash_basic(void) {
    // Test basic string hashing
    uint32_t hash1 = hash_string("hello");
    uint32_t hash2 = hash_string("world");
    uint32_t hash3 = hash_string("hello"); // Same as hash1
    
    TEST_ASSERT_NOT_EQUAL(hash1, hash2); // Different strings should have different hashes
    TEST_ASSERT_EQUAL(hash1, hash3);     // Same strings should have same hashes
    
    // Test empty string
    uint32_t hash_empty = hash_string("");
    TEST_ASSERT_NOT_EQUAL(0, hash_empty); // Should not be zero
}

/**
 * @brief Test hash function with various inputs
 */
void test_hash_various_inputs(void) {
    // Test single characters
    uint32_t hash_a = hash_string("a");
    uint32_t hash_b = hash_string("b");
    TEST_ASSERT_NOT_EQUAL(hash_a, hash_b);
    
    // Test long strings
    char long_str1[1000];
    char long_str2[1000];
    memset(long_str1, 'x', 999);
    long_str1[999] = '\0';
    memset(long_str2, 'y', 999);
    long_str2[999] = '\0';
    
    uint32_t hash_long1 = hash_string(long_str1);
    uint32_t hash_long2 = hash_string(long_str2);
    TEST_ASSERT_NOT_EQUAL(hash_long1, hash_long2);
    
    // Test strings with special characters
    uint32_t hash_special1 = hash_string("hello\n\t");
    uint32_t hash_special2 = hash_string("hello\n ");
    TEST_ASSERT_NOT_EQUAL(hash_special1, hash_special2);
}

/**
 * @brief Test hash function consistency
 */
void test_hash_consistency(void) {
    const char* test_strings[] = {
        "main", "printf", "int", "return", "if", "else", "while", "for"
    };
    const int num_strings = sizeof(test_strings) / sizeof(test_strings[0]);
    
    // Hash multiple times and ensure consistency
    for (int i = 0; i < num_strings; i++) {
        uint32_t hash1 = hash_string(test_strings[i]);
        uint32_t hash2 = hash_string(test_strings[i]);
        uint32_t hash3 = hash_string(test_strings[i]);
        
        TEST_ASSERT_EQUAL(hash1, hash2);
        TEST_ASSERT_EQUAL(hash2, hash3);
    }
}

/**
 * @brief Test hash distribution (basic test)
 */
void test_hash_distribution(void) {
    const int num_buckets = 16;
    int bucket_counts[num_buckets] = {0};
    
    // Generate hashes for various strings
    char buffer[32];
    for (int i = 0; i < 100; i++) {
        snprintf(buffer, sizeof(buffer), "string_%d", i);
        uint32_t hash = hash_string(buffer);
        bucket_counts[hash % num_buckets]++;
    }
    
    // Check that distribution is reasonably uniform
    // Each bucket should have at least 1 item (very basic test)
    int empty_buckets = 0;
    for (int i = 0; i < num_buckets; i++) {
        if (bucket_counts[i] == 0) {
            empty_buckets++;
        }
    }
    
    // Allow up to 25% empty buckets for a simple hash function
    TEST_ASSERT_LESS_THAN(num_buckets / 4, empty_buckets);
}

/**
 * @brief Test hmapbuf initialization and cleanup
 */
void test_hmapbuf_init_cleanup(void) {
    char hmapbuf_file[] = TEMP_PATH "test_hmapbuf.out";
    
    // Initialize hmapbuf
    int result = hmapbuf_init(hmapbuf_file);
    TEST_ASSERT_EQUAL(0, result);
    
    hmapbuf_close();
    
    // File should exist
    TEST_ASSERT_FILE_EXISTS(hmapbuf_file);
}

/**
 * @brief Test hmapbuf put and get operations
 */
void test_hmapbuf_put_get(void) {
    char hmapbuf_file[] = TEMP_PATH "test_hmapbuf.out";
    TEST_ASSERT_EQUAL(0, hmapbuf_init(hmapbuf_file));
    
    // Put some key-value pairs
    uint32_t key1 = hash_string("variable1");
    uint32_t key2 = hash_string("function1");
    uint32_t key3 = hash_string("type1");
    
    int result1 = hmapbuf_put(key1, 101);
    int result2 = hmapbuf_put(key2, 102);
    int result3 = hmapbuf_put(key3, 103);
    
    TEST_ASSERT_EQUAL(0, result1);
    TEST_ASSERT_EQUAL(0, result2);
    TEST_ASSERT_EQUAL(0, result3);
    
    // Get values back
    uint32_t value1, value2, value3;
    int found1 = hmapbuf_get(key1, &value1);
    int found2 = hmapbuf_get(key2, &value2);
    int found3 = hmapbuf_get(key3, &value3);
    
    TEST_ASSERT_EQUAL(1, found1);
    TEST_ASSERT_EQUAL(1, found2);
    TEST_ASSERT_EQUAL(1, found3);
    
    TEST_ASSERT_EQUAL(101, value1);
    TEST_ASSERT_EQUAL(102, value2);
    TEST_ASSERT_EQUAL(103, value3);
    
    hmapbuf_close();
}

/**
 * @brief Test hmapbuf with non-existent keys
 */
void test_hmapbuf_missing_keys(void) {
    char hmapbuf_file[] = TEMP_PATH "test_hmapbuf.out";
    TEST_ASSERT_EQUAL(0, hmapbuf_init(hmapbuf_file));
    
    // Try to get a non-existent key
    uint32_t key = hash_string("nonexistent");
    uint32_t value;
    int found = hmapbuf_get(key, &value);
    
    TEST_ASSERT_EQUAL(0, found); // Should not be found
    
    hmapbuf_close();
}

/**
 * @brief Test hmapbuf key collision handling
 */
void test_hmapbuf_collisions(void) {
    char hmapbuf_file[] = TEMP_PATH "test_hmapbuf.out";
    TEST_ASSERT_EQUAL(0, hmapbuf_init(hmapbuf_file));
    
    // Add many entries to force potential collisions
    const int num_entries = 50;
    uint32_t keys[num_entries];
    uint32_t values[num_entries];
    
    char buffer[32];
    for (int i = 0; i < num_entries; i++) {
        snprintf(buffer, sizeof(buffer), "key_%d", i);
        keys[i] = hash_string(buffer);
        values[i] = i * 10;
        
        int result = hmapbuf_put(keys[i], values[i]);
        TEST_ASSERT_EQUAL(0, result);
    }
    
    // Verify all entries can be retrieved
    for (int i = 0; i < num_entries; i++) {
        uint32_t retrieved_value;
        int found = hmapbuf_get(keys[i], &retrieved_value);
        
        TEST_ASSERT_EQUAL(1, found);
        TEST_ASSERT_EQUAL(values[i], retrieved_value);
    }
    
    hmapbuf_close();
}

/**
 * @brief Test hmapbuf update operations
 */
void test_hmapbuf_updates(void) {
    char hmapbuf_file[] = TEMP_PATH "test_hmapbuf.out";
    TEST_ASSERT_EQUAL(0, hmapbuf_init(hmapbuf_file));
    
    uint32_t key = hash_string("update_test");
    
    // Put initial value
    int result = hmapbuf_put(key, 100);
    TEST_ASSERT_EQUAL(0, result);
    
    // Update value
    result = hmapbuf_put(key, 200);
    TEST_ASSERT_EQUAL(0, result);
    
    // Verify updated value
    uint32_t value;
    int found = hmapbuf_get(key, &value);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(200, value);
    
    hmapbuf_close();
}

/**
 * @brief Test hmapbuf persistence
 */
void test_hmapbuf_persistence(void) {
    char hmapbuf_file[] = TEMP_PATH "test_hmapbuf.out";
    uint32_t key = hash_string("persistent_key");
    uint32_t original_value = 12345;
    
    // First session - store data
    TEST_ASSERT_EQUAL(0, hmapbuf_init(hmapbuf_file));
    int result = hmapbuf_put(key, original_value);
    TEST_ASSERT_EQUAL(0, result);
    hmapbuf_close();
    
    // Second session - reload and verify
    TEST_ASSERT_EQUAL(0, hmapbuf_init(hmapbuf_file));
    uint32_t retrieved_value;
    int found = hmapbuf_get(key, &retrieved_value);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(original_value, retrieved_value);
    hmapbuf_close();
}

/**
 * @brief Run all utility tests
 */
void run_utils_tests(void) {
    printf("Running utils tests...\n");
    
    RUN_TEST(test_hash_basic);
    RUN_TEST(test_hash_various_inputs);
    RUN_TEST(test_hash_consistency);
    RUN_TEST(test_hash_distribution);
    RUN_TEST(test_hmapbuf_init_cleanup);
    RUN_TEST(test_hmapbuf_put_get);
    RUN_TEST(test_hmapbuf_missing_keys);
    RUN_TEST(test_hmapbuf_collisions);
    RUN_TEST(test_hmapbuf_updates);
    RUN_TEST(test_hmapbuf_persistence);
}
