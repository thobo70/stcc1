/**
 * @file error_core.c
 * @brief Core error handling implementation
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "error_core.h"
#include "../storage/tstore.h"

// Global error state
static ErrorState_t g_error_state = {0};
static int g_initialized = 0;

/**
 * @brief Get default error configuration
 */
ErrorConfig_t error_get_default_config(void) {
    ErrorConfig_t config = {
        .max_errors = 50,
        .max_warnings = 100,
        .show_line_numbers = 1,
        .show_source_context = 1,
        .show_suggestions = 1,
        .colorize_output = 0,  // Disabled by default for compatibility
        .output_stream = NULL  // Will default to stderr
    };
    return config;
}

/**
 * @brief Initialize core error handling
 */
void error_core_init(const ErrorConfig_t* config) {
    if (g_initialized) {
        error_core_cleanup();
    }
    
    memset(&g_error_state, 0, sizeof(ErrorState_t));
    
    if (config) {
        g_error_state.config = *config;
    } else {
        g_error_state.config = error_get_default_config();
    }
    
    // Default to stderr if no stream specified
    if (!g_error_state.config.output_stream) {
        g_error_state.config.output_stream = stderr;
    }
    
    g_initialized = 1;
    
    printf("[ERROR] Error handler initialized (max_errors=%d, max_warnings=%d)\n",
           g_error_state.config.max_errors, g_error_state.config.max_warnings);
}

/**
 * @brief Cleanup error handling
 */
void error_core_cleanup(void) {
    if (!g_initialized) return;
    
    // Free all error structures
    CompilerError_t* current = g_error_state.error_list;
    while (current) {
        CompilerError_t* next = current->next;
        
        // Free dynamically allocated strings if any
        // (In this implementation, we assume strings are static or managed elsewhere)
        
        free(current);
        current = next;
    }
    
    printf("[ERROR] Error handler cleanup: %d errors, %d warnings total\n",
           g_error_state.total_errors, g_error_state.total_warnings);
    
    memset(&g_error_state, 0, sizeof(ErrorState_t));
    g_initialized = 0;
}

/**
 * @brief Reset error state (keep configuration)
 */
void error_core_reset(void) {
    if (!g_initialized) return;
    
    // Free existing errors
    CompilerError_t* current = g_error_state.error_list;
    while (current) {
        CompilerError_t* next = current->next;
        free(current);
        current = next;
    }
    
    // Reset counters but keep configuration
    ErrorConfig_t saved_config = g_error_state.config;
    memset(&g_error_state, 0, sizeof(ErrorState_t));
    g_error_state.config = saved_config;
}

/**
 * @brief Create source location from token
 */
SourceLocation_t error_create_location(TokenIdx_t token_idx) {
    SourceLocation_t location = {0};
    location.token_idx = token_idx;
    
    if (token_idx != 0) {
        // Get token to extract file and line information
        Token_t token = tstore_get(token_idx);
        location.filename = sstore_get(token.file);
        location.line = token.line;
        location.column = 0;  // Column info not available in current token system
        location.line_text = NULL;  // Would need source file access
    }
    
    return location;
}

/**
 * @brief Create source location with explicit position
 */
SourceLocation_t error_create_location_with_pos(const char* filename,
                                               uint32_t line, uint32_t column) {
    SourceLocation_t location = {
        .token_idx = 0,
        .filename = filename,
        .line = line,
        .column = column,
        .line_text = NULL
    };
    return location;
}

/**
 * @brief Report an error to the core system
 */
CompilerError_t* error_core_report(ErrorLevel_t level, ErrorCategory_t category,
                                  const SourceLocation_t* location,
                                  uint32_t error_code, const char* message,
                                  const char* suggestion, const char* stage_name,
                                  void* stage_context) {
    
    if (!g_initialized) {
        fprintf(stderr, "ERROR: Error handler not initialized!\n");
        return NULL;
    }
    
    // Prevent recursive errors
    if (g_error_state.in_error_handler) {
        fprintf(stderr, "ERROR: Recursive error in error handler!\n");
        return NULL;
    }
    g_error_state.in_error_handler = 1;
    
    // Check if we should abort due to too many errors
    if (level >= ERROR_ERROR && 
        g_error_state.total_errors >= g_error_state.config.max_errors) {
        g_error_state.should_abort = 1;
        g_error_state.in_error_handler = 0;
        return NULL;
    }
    
    // Allocate new error structure
    CompilerError_t* error = (CompilerError_t*)malloc(sizeof(CompilerError_t));
    if (!error) {
        fprintf(stderr, "FATAL: Cannot allocate memory for error reporting!\n");
        g_error_state.should_abort = 1;
        g_error_state.in_error_handler = 0;
        return NULL;
    }
    
    // Initialize error structure
    memset(error, 0, sizeof(CompilerError_t));
    error->level = level;
    error->category = category;
    error->error_code = error_code;
    error->message = message;
    error->suggestion = suggestion;
    error->stage_name = stage_name;
    error->stage_context = stage_context;
    
    if (location) {
        error->location = *location;
    }
    
    // Update statistics
    g_error_state.error_count[level]++;
    g_error_state.category_count[category]++;
    
    if (level >= ERROR_ERROR) {
        g_error_state.total_errors++;
    } else if (level == ERROR_WARNING) {
        g_error_state.total_warnings++;
    }
    
    // Add to error list
    if (!g_error_state.error_list) {
        g_error_state.error_list = error;
        g_error_state.last_error = error;
    } else {
        g_error_state.last_error->next = error;
        g_error_state.last_error = error;
    }
    
    // Print error immediately if configured to do so
    error_core_print_error(error);
    
    // Check for abort conditions
    if (level == ERROR_FATAL ||
        (level >= ERROR_ERROR && g_error_state.total_errors >= g_error_state.config.max_errors) ||
        (level == ERROR_WARNING && g_error_state.total_warnings >= g_error_state.config.max_warnings)) {
        g_error_state.should_abort = 1;
    }
    
    g_error_state.in_error_handler = 0;
    return error;
}

/**
 * @brief Print a single error
 */
void error_core_print_error(const CompilerError_t* error) {
    if (!error || !g_initialized) return;
    
    FILE* out = g_error_state.config.output_stream;
    
    // Error level prefix
    const char* level_names[] = {"INFO", "WARNING", "ERROR", "FATAL"};
    const char* level_name = (error->level <= ERROR_FATAL) ? 
                            level_names[error->level] : "UNKNOWN";
    
    // Print location information
    if (error->location.filename && error->location.line > 0) {
        fprintf(out, "%s:%u: ", error->location.filename, error->location.line);
    } else if (error->location.token_idx > 0) {
        fprintf(out, "token %u: ", error->location.token_idx);
    }
    
    // Print error level and stage
    fprintf(out, "%s", level_name);
    if (error->stage_name) {
        fprintf(out, " [%s]", error->stage_name);
    }
    fprintf(out, ": ");
    
    // Print error message
    fprintf(out, "%s", error->message ? error->message : "Unknown error");
    
    // Print error code if available
    if (error->error_code > 0) {
        fprintf(out, " (E%u)", error->error_code);
    }
    
    fprintf(out, "\n");
    
    // Print source context if available
    if (g_error_state.config.show_source_context && error->location.line_text) {
        fprintf(out, "  %s\n", error->location.line_text);
        if (error->location.column > 0) {
            fprintf(out, "  %*s^\n", error->location.column - 1, "");
        }
    }
    
    // Print suggestion if available
    if (g_error_state.config.show_suggestions && error->suggestion) {
        fprintf(out, "  Suggestion: %s\n", error->suggestion);
    }
}

/**
 * @brief Print error summary
 */
void error_core_print_summary(void) {
    if (!g_initialized) return;
    
    FILE* out = g_error_state.config.output_stream;
    
    fprintf(out, "\n=== Compilation Summary ===\n");
    fprintf(out, "Errors: %d, Warnings: %d\n", 
            g_error_state.total_errors, g_error_state.total_warnings);
    
    if (g_error_state.total_errors > 0 || g_error_state.total_warnings > 0) {
        fprintf(out, "Breakdown by category:\n");
        
        const char* category_names[] = {
            "Lexical", "Syntax", "Semantic", "Codegen", 
            "Optimization", "Memory", "I/O", "Internal"
        };
        
        for (int i = 0; i <= ERROR_INTERNAL; i++) {
            if (g_error_state.category_count[i] > 0) {
                fprintf(out, "  %s: %d\n", category_names[i], 
                       g_error_state.category_count[i]);
            }
        }
    }
    
    if (g_error_state.should_abort) {
        fprintf(out, "Compilation aborted due to errors.\n");
    } else if (g_error_state.total_errors == 0) {
        fprintf(out, "Compilation completed successfully.\n");
    }
}

/**
 * @brief Check if compilation should abort
 */
int error_core_should_abort(void) {
    return g_initialized ? g_error_state.should_abort : 0;
}

/**
 * @brief Get error count by level
 */
int error_core_get_count(ErrorLevel_t level) {
    return (g_initialized && level <= ERROR_FATAL) ? 
           g_error_state.error_count[level] : 0;
}

/**
 * @brief Check if there are any errors
 */
int error_core_has_errors(void) {
    return g_initialized ? (g_error_state.total_errors > 0) : 0;
}

/**
 * @brief Check if there are fatal errors
 */
int error_core_has_fatal_errors(void) {
    return error_core_get_count(ERROR_FATAL) > 0;
}

/**
 * @brief Set maximum number of errors
 */
void error_set_max_errors(int max_errors) {
    if (g_initialized) {
        g_error_state.config.max_errors = max_errors;
    }
}

/**
 * @brief Set output stream for errors
 */
void error_set_output_stream(FILE* stream) {
    if (g_initialized && stream) {
        g_error_state.config.output_stream = stream;
    }
}
