/**
 * @file error_demo.c
 * @brief Demonstration of the modular error handling system
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#include <stdio.h>
#include <stdlib.h>

#include "error_core.h"
#include "error_stages.h"
#include "error_recovery.h"

/**
 * @brief Demonstrate lexical error handling
 */
void demo_lexical_errors(void) {
    printf("\n=== Lexical Error Handling Demo ===\n");
    
    error_set_current_stage("Lexical Analysis");
    
    // Simulate various lexical errors
    printf("1. Invalid character error:\n");
    lex_error_invalid_char(1, '\x80');  // Invalid UTF-8 character
    
    printf("\n2. Unterminated string error:\n");
    lex_error_unterminated_string(2);
    
    printf("\n3. Invalid escape sequence error:\n");
    lex_error_invalid_escape(3, 'q');  // '\q' is not valid
    
    printf("\n4. Invalid number format error:\n");
    lex_error_invalid_number(4, "123.45.67");  // Multiple decimal points
}

/**
 * @brief Demonstrate syntax error handling
 */
void demo_syntax_errors(void) {
    printf("\n=== Syntax Error Handling Demo ===\n");
    
    error_set_current_stage("Syntax Analysis");
    
    // Simulate various syntax errors
    printf("1. Missing semicolon error:\n");
    syntax_error_missing_token(10, T_SEMICOLON);
    
    printf("\n2. Unexpected token error:\n");
    syntax_error_unexpected_token(11, T_RPAREN, T_SEMICOLON);
    
    printf("\n3. Unmatched brace error:\n");
    syntax_error_unmatched_delimiter(12, '{');
    
    printf("\n4. Invalid expression error:\n");
    syntax_error_invalid_expression(13, "assignment statement");
}

/**
 * @brief Demonstrate semantic error handling
 */
void demo_semantic_errors(void) {
    printf("\n=== Semantic Error Handling Demo ===\n");
    
    error_set_current_stage("Semantic Analysis");
    
    // Simulate various semantic errors
    printf("1. Undefined symbol error:\n");
    semantic_error_undefined_symbol(20, "undeclared_variable");
    
    printf("\n2. Redefined symbol error:\n");
    semantic_error_redefined_symbol(21, "duplicate_function", 15);
    
    printf("\n3. Type mismatch error:\n");
    semantic_error_type_mismatch(22, 1, 2, "assignment");  // int vs float
    
    printf("\n4. Invalid assignment error:\n");
    semantic_error_invalid_assignment(23, 3, 4);  // const vs non-const
}

/**
 * @brief Demonstrate code generation error handling
 */
void demo_codegen_errors(void) {
    printf("\n=== Code Generation Error Handling Demo ===\n");
    
    error_set_current_stage("Code Generation");
    
    // Simulate various code generation errors
    printf("1. Unsupported feature error:\n");
    codegen_error_unsupported_feature(100, "inline assembly");
    
    printf("\n2. Register spill warning:\n");
    codegen_error_register_spill(101);
}

/**
 * @brief Demonstrate error recovery
 */
void demo_error_recovery(void) {
    printf("\n=== Error Recovery Demo ===\n");
    
    RecoveryContext_t context;
    recovery_init_context(&context, 50);
    context.production_name = "expression";
    
    // Add expected and sync tokens
    recovery_add_expected_token(&context, T_ID);
    recovery_add_expected_token(&context, T_LITINT);
    recovery_add_sync_token(&context, T_SEMICOLON);
    recovery_add_sync_token(&context, T_RBRACE);
    
    printf("Recovery context for error at token 50:\n");
    printf("  Production: %s\n", context.production_name);
    printf("  Expected tokens: %d\n", context.expected_count);
    printf("  Sync tokens: %d\n", context.sync_count);
    
    // Suggest recovery action
    RecoveryResult_t result = recovery_suggest_action(&context);
    printf("  Suggested action: %d\n", (int)result.action);
    printf("  Explanation: %s\n", result.explanation ? result.explanation : "N/A");
}

/**
 * @brief Demonstrate cross-stage error correlation
 */
void demo_error_correlation(void) {
    printf("\n=== Cross-Stage Error Correlation Demo ===\n");
    
    // Create a primary error in syntax analysis
    error_set_current_stage("Syntax Analysis");
    SourceLocation_t syntax_loc = error_create_location(100);
    CompilerError_t* syntax_error = error_core_report(
        ERROR_ERROR, ERROR_SYNTAX, &syntax_loc, 
        SYNTAX_ERROR_MISSING_TOKEN,
        "Missing semicolon after statement",
        "Add semicolon (;) at end of statement",
        "Syntax Analysis", NULL
    );
    
    // Create a related semantic error
    error_set_current_stage("Semantic Analysis");
    SourceLocation_t semantic_loc = error_create_location(105);
    CompilerError_t* semantic_error = error_core_report(
        ERROR_ERROR, ERROR_SEMANTIC, &semantic_loc,
        SEMANTIC_ERROR_UNDEFINED_SYMBOL,
        "Symbol 'x' used but not declared (cascade from syntax error)",
        "This error may be caused by the previous syntax error",
        "Semantic Analysis", NULL
    );
    
    // Link the errors
    if (syntax_error && semantic_error) {
        error_add_related_error(syntax_error, semantic_error);
        printf("Linked syntax error (token 100) with semantic error (token 105)\n");
    }
}

/**
 * @brief Test error filtering and iteration
 */
static int count_syntax_errors(const CompilerError_t* error, void* context) {
    int* count = (int*)context;
    if (error && error->category == ERROR_SYNTAX) {
        (*count)++;
        printf("  Found syntax error: %s\n", error->message ? error->message : "Unknown");
    }
    return 0;  // Continue iteration
}

void demo_error_filtering(void) {
    printf("\n=== Error Filtering and Iteration Demo ===\n");
    
    int syntax_error_count = 0;
    error_core_iterate_errors(count_syntax_errors, &syntax_error_count);
    
    printf("Total syntax errors found: %d\n", syntax_error_count);
    printf("Total errors by category:\n");
    
    const char* category_names[] = {
        "Lexical", "Syntax", "Semantic", "Codegen", 
        "Optimization", "Memory", "I/O", "Internal"
    };
    
    for (int i = 0; i <= ERROR_INTERNAL; i++) {
        int count = error_core_get_category_count((ErrorCategory_t)i);
        if (count > 0) {
            printf("  %s: %d\n", category_names[i], count);
        }
    }
}

/**
 * @brief Main demonstration function
 */
int main(int argc, char* argv[]) {
    printf("STCC1 Modular Error Handling System Demonstration\n");
    printf("=================================================\n");
    
    // Initialize error handling system
    ErrorConfig_t config = error_get_default_config();
    config.show_suggestions = 1;
    config.show_source_context = 1;
    config.max_errors = 100;  // Allow many errors for demo
    
    error_core_init(&config);
    error_stages_init_all();
    
    // Run demonstrations
    demo_lexical_errors();
    demo_syntax_errors();
    demo_semantic_errors();
    demo_codegen_errors();
    demo_error_recovery();
    demo_error_correlation();
    demo_error_filtering();
    
    // Print final summary
    printf("\n");
    error_core_print_summary();
    
    printf("\nError System Statistics:\n");
    printf("  Total errors: %d\n", error_core_get_count(ERROR_ERROR) + error_core_get_count(ERROR_FATAL));
    printf("  Total warnings: %d\n", error_core_get_count(ERROR_WARNING));
    printf("  Should abort: %s\n", error_core_should_abort() ? "Yes" : "No");
    
    // Cleanup
    error_stages_cleanup_all();
    error_core_cleanup();
    
    printf("\nDemo complete. The modular error handling system provides:\n");
    printf("✓ Stage-specific error handling with consistent interface\n");
    printf("✓ Comprehensive error recovery strategies\n");
    printf("✓ Cross-stage error correlation and analysis\n");
    printf("✓ Flexible configuration and filtering\n");
    printf("✓ Memory-efficient error storage and reporting\n");
    
    return 0;
}
