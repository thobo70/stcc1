/**
 * @file error_core.h
 * @brief Core error handling infrastructure - stage independent
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#ifndef SRC_ERROR_ERROR_CORE_H_
#define SRC_ERROR_ERROR_CORE_H_

#include <stdio.h>
#include <stdint.h>
#include "../lexer/ctoken.h"
#include "../storage/sstore.h"

// Error severity levels - universal across all stages
typedef enum {
    ERROR_INFO,
    ERROR_WARNING,
    ERROR_ERROR,
    ERROR_FATAL
} ErrorLevel_t;

// Error categories - covers all compiler stages
typedef enum {
    ERROR_LEXICAL,      // Tokenization errors
    ERROR_SYNTAX,       // Parser errors
    ERROR_SEMANTIC,     // Type checking, undefined symbols
    ERROR_CODEGEN,      // Code generation errors
    ERROR_OPTIMIZATION, // Optimization warnings/errors
    ERROR_MEMORY,       // Memory allocation failures
    ERROR_IO,           // File I/O errors
    ERROR_INTERNAL      // Internal compiler errors
} ErrorCategory_t;

// Source location information
typedef struct {
    TokenIdx_t token_idx;       // Token reference (0 if N/A)
    const char* filename;       // Source filename
    uint32_t line;             // Line number
    uint32_t column;           // Column number
    const char* line_text;     // Source line text (optional)
} SourceLocation_t;

// Core error structure
typedef struct CompilerError {
    ErrorLevel_t level;
    ErrorCategory_t category;
    SourceLocation_t location;

    // Error identification
    uint32_t error_code;       // Unique error code
    const char* message;       // Primary error message
    const char* suggestion;    // Optional fix suggestion

    // Context information
    const char* stage_name;    // Compiler stage name
    void* stage_context;       // Stage-specific context data

    // Linked list for error chaining
    struct CompilerError* next;
} CompilerError_t;

// Error handler configuration
typedef struct {
    int max_errors;            // Stop after this many errors
    int max_warnings;          // Stop after this many warnings
    int show_line_numbers;     // Include line numbers in output
    int show_source_context;   // Show source code context
    int show_suggestions;      // Show fix suggestions
    int colorize_output;       // Use colored output
    FILE* output_stream;       // Where to write errors (stderr default)
} ErrorConfig_t;

// Global error state
typedef struct {
    ErrorConfig_t config;
    CompilerError_t* error_list;    // Linked list of errors
    CompilerError_t* last_error;    // Last error for fast append

    // Statistics
    int error_count[ERROR_FATAL + 1];    // Count by severity
    int category_count[ERROR_INTERNAL + 1]; // Count by category
    int total_errors;
    int total_warnings;

    // State tracking
    int should_abort;
    int in_error_handler;      // Prevent recursive errors
} ErrorState_t;

// Core error handling functions
void error_core_init(const ErrorConfig_t* config);
void error_core_cleanup(void);
void error_core_reset(void);

// Error reporting - core interface
CompilerError_t* error_core_report(ErrorLevel_t level, ErrorCategory_t category,
                                  const SourceLocation_t* location,
                                  uint32_t error_code, const char* message,
                                  const char* suggestion, const char* stage_name,
                                  void* stage_context);

// Error state queries
int error_core_get_count(ErrorLevel_t level);
int error_core_get_category_count(ErrorCategory_t category);
int error_core_should_abort(void);
int error_core_has_errors(void);
int error_core_has_fatal_errors(void);

// Error output and formatting
void error_core_print_error(const CompilerError_t* error);
void error_core_print_summary(void);
void error_core_print_all_errors(void);

// Source location utilities
SourceLocation_t error_create_location(TokenIdx_t token_idx);
SourceLocation_t error_create_location_with_pos(const char* filename,
                                               uint32_t line, uint32_t column);
void error_extract_source_line(SourceLocation_t* location);

// Error filtering and iteration
typedef int (*error_filter_func_t)(const CompilerError_t* error, void* context);
void error_core_iterate_errors(error_filter_func_t filter, void* context);

// Configuration helpers
ErrorConfig_t error_get_default_config(void);
void error_set_max_errors(int max_errors);
void error_set_output_stream(FILE* stream);
void error_enable_colors(int enable);

#endif  // SRC_ERROR_ERROR_CORE_H_
