/**
 * @file memory_config.h
 * @brief Memory configuration tuning for low-memory compilation
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.2
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#ifndef SRC_UTILS_MEMORY_CONFIG_H_
#define SRC_UTILS_MEMORY_CONFIG_H_

// Memory budget configuration (in bytes)
#define TARGET_TOTAL_MEMORY_KB    100    // Target: compile in under 100KB RAM
#define TARGET_PARSER_STACK_B     512    // Parser recursion stack budget
#define TARGET_SYMBOL_CACHE_B     2048   // In-memory symbol cache
#define TARGET_STRING_CACHE_B     1024   // In-memory string cache

// Buffer sizes based on memory budget
#define OPTIMAL_HBNNODES         64      // Reduced from 100 for tighter memory
#define OPTIMAL_HASH_SIZE        16      // Increased for better distribution
#define OPTIMAL_HASH_MASK        (OPTIMAL_HASH_SIZE - 1)

// String storage optimization
#define SSTORE_BUFFER_SIZE       1024    // Buffer for frequently accessed strings
#define SSTORE_MAX_STRING_LEN    256     // Maximum individual string length

// Token storage optimization
#define TSTORE_BUFFER_SIZE       512     // Token lookahead buffer
#define TSTORE_BATCH_SIZE        32      // Tokens to read/write at once

// AST node optimization
#define AST_INLINE_THRESHOLD     8       // Inline small AST subtrees
#define AST_COMPRESSION_LEVEL    1       // Light compression for AST storage

// Symbol table optimization
#define SYMTAB_HASH_SIZE         32      // Hash table size for symbols
#define SYMTAB_SCOPE_DEPTH       8       // Maximum nested scope depth

// Parser optimization
#define MAX_EXPRESSION_DEPTH     32      // Maximum expression nesting
#define MAX_STATEMENT_DEPTH      16      // Maximum statement nesting

// Memory monitoring thresholds
#define MEMORY_WARNING_KB        80      // Warn when approaching limit
#define MEMORY_CRITICAL_KB       95      // Force garbage collection

// Feature toggles for memory optimization
#define ENABLE_AST_COMPRESSION   1       // Compress AST nodes on disk
#define ENABLE_STRING_INTERNING  1       // Deduplicate strings aggressively
#define ENABLE_LAZY_LOADING      1       // Load symbols/AST on demand
#define ENABLE_MEMORY_TRACKING   1       // Track memory usage for tuning

// Debug and profiling
#ifdef DEBUG_MEMORY
#define MEMORY_TRACE(fmt, ...) fprintf(stderr, "[MEM] " fmt "\n", ##__VA_ARGS__)
#else
#define MEMORY_TRACE(fmt, ...)
#endif

// Memory allocation wrappers for tracking
#ifdef ENABLE_MEMORY_TRACKING
void* tracked_malloc(size_t size,
                     const char* file,
                     int line);
void tracked_free(void* ptr,
                     const char* file,
                     int line);
void print_memory_usage(void);

#define MALLOC(size) tracked_malloc(size, __FILE__, __LINE__)
#define FREE(ptr) tracked_free(ptr, __FILE__, __LINE__)
#else
#define MALLOC(size) malloc(size)
#define FREE(ptr) free(ptr)
#endif

// Compile-time memory budget checks
#if (OPTIMAL_HBNNODES * sizeof(HBNode)) > (TARGET_TOTAL_MEMORY_KB * 1024 / 4)
#warning "HBNode buffer may be too large for memory budget"
#endif

#if (SSTORE_BUFFER_SIZE + TSTORE_BUFFER_SIZE) > (TARGET_TOTAL_MEMORY_KB * 1024 / 2)
#warning "Storage buffers may be too large for memory budget"
#endif

#endif  // SRC_UTILS_MEMORY_CONFIG_H_
