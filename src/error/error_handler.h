/**
 * @file error_handler.h
 * @brief Enhanced error handling and reporting for STCC1 compiler
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.2
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#ifndef SRC_ERROR_ERROR_HANDLER_H_
#define SRC_ERROR_ERROR_HANDLER_H_

#include <stdio.h>
#include "ctoken.h"
#include "sstore.h"

// Error severity levels
typedef enum {
    ERROR_INFO,
    ERROR_WARNING,
    ERROR_ERROR,
    ERROR_FATAL
} ErrorLevel_t;

// Error categories
typedef enum {
    ERROR_LEXICAL,    // Tokenization errors
    ERROR_SYNTAX,     // Parser errors
    ERROR_SEMANTIC,   // Type checking,
                     undefined symbols
    ERROR_MEMORY,     // Memory allocation failures
    ERROR_IO          // File I/O errors
} ErrorCategory_t;

// Error reporting structure
typedef struct {
    ErrorLevel_t level;
    ErrorCategory_t category;
    TokenIdx_t token_idx;      // Token where error occurred
    const char* message;       // Error description
    const char* suggestion;    // Optional suggestion for fix
} CompilerError_t;

// Global error state
extern int error_count;
extern int warning_count;
extern int max_errors;

// Error reporting functions
void error_init(void);
void error_report(ErrorLevel_t level,
                     ErrorCategory_t category,
                 TokenIdx_t token_idx,
                     const char* message);
void error_report_with_suggestion(ErrorLevel_t level,
                     ErrorCategory_t category,
                                 TokenIdx_t token_idx,
                     const char* message,
                                 const char* suggestion);
void error_print_summary(void);
int error_should_abort(void);

// Convenience macros for common error types
#define SYNTAX_ERROR(token_idx,
                     msg) \
    error_report(ERROR_ERROR,
                     ERROR_SYNTAX,
                     token_idx,
                     msg)

#define SYNTAX_ERROR_WITH_HINT(token_idx,
                     msg,
                     hint) \
    error_report_with_suggestion(ERROR_ERROR,
                     ERROR_SYNTAX,
                     token_idx,
                     msg,
                     hint)

#define SEMANTIC_ERROR(token_idx,
                     msg) \
    error_report(ERROR_ERROR,
                     ERROR_SEMANTIC,
                     token_idx,
                     msg)

#define MEMORY_ERROR(msg) \
    error_report(ERROR_FATAL,
                     ERROR_MEMORY, 0,
                     msg)

#define WARNING(token_idx,
                     msg) \
    error_report(ERROR_WARNING,
                     ERROR_SYNTAX,
                     token_idx,
                     msg)

// Error recovery helpers
typedef struct {
    TokenID_t sync_tokens[8];  // Tokens to synchronize on
    int sync_count;
} ErrorRecovery_t;

void error_recovery_init(ErrorRecovery_t* recovery);
void error_recovery_add_sync_token(ErrorRecovery_t* recovery,
                     TokenID_t token);
int error_recovery_sync(ErrorRecovery_t* recovery);

#endif  // SRC_ERROR_ERROR_HANDLER_H_
