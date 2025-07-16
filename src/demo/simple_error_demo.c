/**
 * @file simple_error_demo.c
 * @brief Simple demonstration of the modular error handling system
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#include <stdio.h>
#include <stdlib.h>
#include "error_core.h"
#include "error_stages.h"

/**
 * @brief Demonstrate lexical analysis error handling
 */
void demo_lexical_errors(void) {
    printf("\n=== Lexical Analysis Error Handling Demo ===\n");

    // Initialize error handling
    ErrorConfig_t config = {
        .max_errors = 10,
        .max_warnings = 20,
        .show_line_numbers = 1,
        .show_source_context = 1,
        .show_suggestions = 1,
        .colorize_output = 0,
        .output_stream = stderr
    };

    error_core_init(&config);

    // Simulate lexical errors
    lex_error_invalid_char(1, '\n');  // Invalid character
    lex_error_unterminated_string(5);  // String not closed
    lex_error_invalid_number(10, "123.45.67");  // Invalid number format

    // Print error summary
    printf("Lexical errors encountered: %d\n", error_core_get_count(ERROR_ERROR));
    printf("Lexical warnings encountered: %d\n", error_core_get_count(ERROR_WARNING));

    error_core_cleanup();
}

/**
 * @brief Demonstrate syntax analysis error handling
 */
void demo_syntax_errors(void) {
    printf("\n=== Syntax Analysis Error Handling Demo ===\n");

    ErrorConfig_t config = {
        .max_errors = 10,
        .max_warnings = 20,
        .show_line_numbers = 1,
        .show_source_context = 1,
        .show_suggestions = 1,
        .colorize_output = 0,
        .output_stream = stderr
    };

    error_core_init(&config);

    // Simulate syntax errors
    syntax_error_missing_token(15, T_SEMICOLON);  // Missing semicolon
    syntax_error_unmatched_delimiter(20, '{');  // Unmatched brace
    syntax_error_invalid_expression(30, "Variable declaration");  // Invalid expression

    printf("Syntax errors encountered: %d\n", error_core_get_count(ERROR_ERROR));
    printf("Syntax warnings encountered: %d\n", error_core_get_count(ERROR_WARNING));

    error_core_cleanup();
}

/**
 * @brief Demonstrate semantic analysis error handling
 */
void demo_semantic_errors(void) {
    printf("\n=== Semantic Analysis Error Handling Demo ===\n");

    ErrorConfig_t config = {
        .max_errors = 10,
        .max_warnings = 20,
        .show_line_numbers = 1,
        .show_source_context = 1,
        .show_suggestions = 1,
        .colorize_output = 0,
        .output_stream = stderr
    };

    error_core_init(&config);

    // Simulate semantic errors
    semantic_error_undefined_symbol(40, "undefined_var");
    semantic_error_redefined_symbol(45, "duplicate_func", 25);  // First defined at token 25

    printf("Semantic errors encountered: %d\n", error_core_get_count(ERROR_ERROR));
    printf("Semantic warnings encountered: %d\n", error_core_get_count(ERROR_WARNING));

    error_core_cleanup();
}

/**
 * @brief Main demonstration function
 */
int main(void) {
    printf("=== Modular Error Handling System Demonstration ===\n");
    printf("Testing stage-specific error handlers with simple interfaces\n");

    demo_lexical_errors();
    demo_syntax_errors();
    demo_semantic_errors();

    printf("\n=== Error Handling Demo Complete ===\n");
    printf("All error handling stages tested successfully.\n");
    printf("Check the error messages above for formatting and details.\n");

    return 0;
}
