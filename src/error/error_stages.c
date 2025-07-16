/**
 * @file error_stages.c
 * @brief Implementation of stage-specific error handlers
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#include <stdio.h>
#include <string.h>

#include "error_stages.h"
#include "../storage/tstore.h"

// Global stage tracking
static const char* g_current_stage = "Unknown";

// ============================================================================
// STAGE MANAGEMENT
// ============================================================================

/**
 * @brief Initialize all stage-specific error handlers
 */
void error_stages_init_all(void) {
    lex_error_init();
    syntax_error_init();
    semantic_error_init();
    codegen_error_init();
    printf("[ERROR] All stage-specific error handlers initialized\n");
}

/**
 * @brief Cleanup all stage-specific error handlers
 */
void error_stages_cleanup_all(void) {
    lex_error_cleanup();
    syntax_error_cleanup();
    semantic_error_cleanup();
    codegen_error_cleanup();
    printf("[ERROR] All stage-specific error handlers cleaned up\n");
}

/**
 * @brief Set current compilation stage
 */
void error_set_current_stage(const char* stage_name) {
    g_current_stage = stage_name ? stage_name : "Unknown";
}

/**
 * @brief Get current compilation stage
 */
const char* error_get_current_stage(void) {
    return g_current_stage;
}

/**
 * @brief Common error reporting with stage context
 */
void error_report_with_context(ErrorLevel_t level, ErrorCategory_t category,
                              TokenIdx_t token_idx, uint32_t error_code,
                              const char* message, const char* suggestion,
                              void* stage_context) {
    SourceLocation_t location = error_create_location(token_idx);
    error_core_report(level, category, &location, error_code, message,
                     suggestion, g_current_stage, stage_context);
}

// ============================================================================
// LEXICAL ANALYSIS ERROR HANDLER
// ============================================================================

static LexicalErrorContext_t g_lex_context = {0};

/**
 * @brief Initialize lexical error handler
 */
void lex_error_init(void) {
    memset(&g_lex_context, 0, sizeof(LexicalErrorContext_t));
    printf("[LEX] Lexical error handler initialized\n");
}

/**
 * @brief Cleanup lexical error handler
 */
void lex_error_cleanup(void) {
    printf("[LEX] Lexical error handler cleaned up\n");
}

/**
 * @brief Report invalid character error
 */
void lex_error_invalid_char(TokenIdx_t token_idx, char invalid_char) {
    char message[128];
    char suggestion[128];
    
    if (invalid_char >= 32 && invalid_char <= 126) {
        snprintf(message, sizeof(message), "Invalid character '%c' (0x%02x)",
                invalid_char, (unsigned char)invalid_char);
    } else {
        snprintf(message, sizeof(message), "Invalid character (0x%02x)",
                (unsigned char)invalid_char);
    }
    
    snprintf(suggestion, sizeof(suggestion),
            "Remove the invalid character or check file encoding");
    
    g_lex_context.unexpected_char = invalid_char;
    
    error_report_with_context(ERROR_ERROR, ERROR_LEXICAL, token_idx,
                             LEX_ERROR_INVALID_CHAR, message, suggestion,
                             &g_lex_context);
}

/**
 * @brief Report unterminated string literal
 */
void lex_error_unterminated_string(TokenIdx_t token_idx) {
    error_report_with_context(ERROR_ERROR, ERROR_LEXICAL, token_idx,
                             LEX_ERROR_UNTERMINATED_STRING,
                             "Unterminated string literal",
                             "Add closing quote (\") to end the string",
                             &g_lex_context);
}

/**
 * @brief Report unterminated character literal
 */
void lex_error_unterminated_char(TokenIdx_t token_idx) {
    error_report_with_context(ERROR_ERROR, ERROR_LEXICAL, token_idx,
                             LEX_ERROR_UNTERMINATED_CHAR,
                             "Unterminated character literal",
                             "Add closing quote (') to end the character",
                             &g_lex_context);
}

/**
 * @brief Report invalid escape sequence
 */
void lex_error_invalid_escape(TokenIdx_t token_idx, char escape_char) {
    char message[128];
    char suggestion[256];
    
    snprintf(message, sizeof(message), "Invalid escape sequence '\\%c'", escape_char);
    snprintf(suggestion, sizeof(suggestion),
            "Use valid escape sequences: \\n, \\t, \\r, \\\\, \\\", \\' or \\xHH");
    
    g_lex_context.unexpected_char = escape_char;
    
    error_report_with_context(ERROR_ERROR, ERROR_LEXICAL, token_idx,
                             LEX_ERROR_INVALID_ESCAPE, message, suggestion,
                             &g_lex_context);
}

/**
 * @brief Report invalid number format
 */
void lex_error_invalid_number(TokenIdx_t token_idx, const char* number_text) {
    char message[256];
    char suggestion[256];
    
    snprintf(message, sizeof(message), "Invalid number format: '%.50s'",
            number_text ? number_text : "");
    snprintf(suggestion, sizeof(suggestion),
            "Check number format (decimal: 123, hex: 0x1F, octal: 077)");
    
    error_report_with_context(ERROR_ERROR, ERROR_LEXICAL, token_idx,
                             LEX_ERROR_INVALID_NUMBER, message, suggestion,
                             &g_lex_context);
}

// ============================================================================
// SYNTAX ANALYSIS ERROR HANDLER
// ============================================================================

static SyntaxErrorContext_t g_syntax_context = {0};

/**
 * @brief Initialize syntax error handler
 */
void syntax_error_init(void) {
    memset(&g_syntax_context, 0, sizeof(SyntaxErrorContext_t));
    printf("[SYNTAX] Syntax error handler initialized\n");
}

/**
 * @brief Cleanup syntax error handler
 */
void syntax_error_cleanup(void) {
    printf("[SYNTAX] Syntax error handler cleaned up\n");
}

/**
 * @brief Report unexpected token error
 */
void syntax_error_unexpected_token(TokenIdx_t token_idx, TokenID_t expected,
                                  TokenID_t found) {
    char message[256];
    char suggestion[256];
    
    // Get token names (simplified - would need a token name lookup table)
    const char* expected_name = (expected < 100) ? "keyword/operator" : "identifier";
    const char* found_name = (found < 100) ? "keyword/operator" : "identifier";
    
    snprintf(message, sizeof(message), "Expected %s but found %s",
            expected_name, found_name);
    snprintf(suggestion, sizeof(suggestion),
            "Check syntax around this location");
    
    g_syntax_context.expected_token = expected;
    g_syntax_context.found_token = found;
    
    error_report_with_context(ERROR_ERROR, ERROR_SYNTAX, token_idx,
                             SYNTAX_ERROR_UNEXPECTED_TOKEN, message, suggestion,
                             &g_syntax_context);
}

/**
 * @brief Report missing token error
 */
void syntax_error_missing_token(TokenIdx_t token_idx, TokenID_t missing) {
    char message[256];
    char suggestion[256];
    
    const char* token_name = (missing == T_SEMICOLON) ? "semicolon (;)" :
                            (missing == T_RBRACE) ? "closing brace (})" :
                            (missing == T_RPAREN) ? "closing parenthesis ())" :
                            "required token";
    
    snprintf(message, sizeof(message), "Missing %s", token_name);
    snprintf(suggestion, sizeof(suggestion), "Add the missing %s", token_name);
    
    g_syntax_context.expected_token = missing;
    
    error_report_with_context(ERROR_ERROR, ERROR_SYNTAX, token_idx,
                             SYNTAX_ERROR_MISSING_TOKEN, message, suggestion,
                             &g_syntax_context);
}

/**
 * @brief Report invalid expression error
 */
void syntax_error_invalid_expression(TokenIdx_t token_idx, const char* context) {
    char message[256];
    char suggestion[256];
    
    snprintf(message, sizeof(message), "Invalid expression%s%s",
            context ? " in " : "", context ? context : "");
    snprintf(suggestion, sizeof(suggestion),
            "Check expression syntax and operator precedence");
    
    error_report_with_context(ERROR_ERROR, ERROR_SYNTAX, token_idx,
                             SYNTAX_ERROR_INVALID_EXPRESSION, message, suggestion,
                             &g_syntax_context);
}

/**
 * @brief Report unmatched delimiter error
 */
void syntax_error_unmatched_delimiter(TokenIdx_t token_idx, char delimiter) {
    char message[256];
    char suggestion[256];
    
    const char* delimiter_name = (delimiter == '{') ? "brace" :
                                (delimiter == '(') ? "parenthesis" :
                                (delimiter == '[') ? "bracket" : "delimiter";
    
    snprintf(message, sizeof(message), "Unmatched %c (%s)", delimiter, delimiter_name);
    snprintf(suggestion, sizeof(suggestion), "Add matching closing %s",
            (delimiter == '{') ? "}" :
            (delimiter == '(') ? ")" :
            (delimiter == '[') ? "]" : "delimiter");
    
    error_report_with_context(ERROR_ERROR, ERROR_SYNTAX, token_idx,
                             SYNTAX_ERROR_UNMATCHED_BRACE, message, suggestion,
                             &g_syntax_context);
}

// ============================================================================
// SEMANTIC ANALYSIS ERROR HANDLER
// ============================================================================

static SemanticErrorContext_t g_semantic_context = {0};

/**
 * @brief Initialize semantic error handler
 */
void semantic_error_init(void) {
    memset(&g_semantic_context, 0, sizeof(SemanticErrorContext_t));
    printf("[SEMANTIC] Semantic error handler initialized\n");
}

/**
 * @brief Cleanup semantic error handler
 */
void semantic_error_cleanup(void) {
    printf("[SEMANTIC] Semantic error handler cleaned up\n");
}

/**
 * @brief Report undefined symbol error
 */
void semantic_error_undefined_symbol(TokenIdx_t token_idx, const char* symbol_name) {
    char message[256];
    char suggestion[256];
    
    snprintf(message, sizeof(message), "Undefined symbol '%s'",
            symbol_name ? symbol_name : "unknown");
    snprintf(suggestion, sizeof(suggestion),
            "Declare the symbol before using it, or check for typos");
    
    g_semantic_context.symbol_name = symbol_name;
    
    error_report_with_context(ERROR_ERROR, ERROR_SEMANTIC, token_idx,
                             SEMANTIC_ERROR_UNDEFINED_SYMBOL, message, suggestion,
                             &g_semantic_context);
}

/**
 * @brief Report redefined symbol error
 */
void semantic_error_redefined_symbol(TokenIdx_t token_idx, const char* symbol_name,
                                    TokenIdx_t first_definition) {
    char message[256];
    char suggestion[256];
    
    snprintf(message, sizeof(message), "Symbol '%s' redefined",
            symbol_name ? symbol_name : "unknown");
    
    if (first_definition > 0) {
        snprintf(suggestion, sizeof(suggestion),
                "Previous definition at token %u. Use a different name or remove one definition",
                first_definition);
    } else {
        snprintf(suggestion, sizeof(suggestion),
                "Symbol already defined. Use a different name or remove one definition");
    }
    
    g_semantic_context.symbol_name = symbol_name;
    
    error_report_with_context(ERROR_ERROR, ERROR_SEMANTIC, token_idx,
                             SEMANTIC_ERROR_REDEFINED_SYMBOL, message, suggestion,
                             &g_semantic_context);
}

/**
 * @brief Report type mismatch error
 */
void semantic_error_type_mismatch(TokenIdx_t token_idx, TypeIdx_t expected,
                                 TypeIdx_t found, const char* context) {
    char message[256];
    char suggestion[256];
    
    snprintf(message, sizeof(message), "Type mismatch%s%s (expected type %u, found type %u)",
            context ? " in " : "", context ? context : "", expected, found);
    snprintf(suggestion, sizeof(suggestion),
            "Cast the value to the expected type or change the variable type");
    
    g_semantic_context.expected_type = expected;
    g_semantic_context.found_type = found;
    
    error_report_with_context(ERROR_ERROR, ERROR_SEMANTIC, token_idx,
                             SEMANTIC_ERROR_TYPE_MISMATCH, message, suggestion,
                             &g_semantic_context);
}

// ============================================================================
// CODE GENERATION ERROR HANDLER
// ============================================================================

static CodegenErrorContext_t g_codegen_context = {0};

/**
 * @brief Initialize code generation error handler
 */
void codegen_error_init(void) {
    memset(&g_codegen_context, 0, sizeof(CodegenErrorContext_t));
    g_codegen_context.target_architecture = "x86_64";  // Default
    printf("[CODEGEN] Code generation error handler initialized\n");
}

/**
 * @brief Cleanup code generation error handler
 */
void codegen_error_cleanup(void) {
    printf("[CODEGEN] Code generation error handler cleaned up\n");
}

/**
 * @brief Report unsupported feature error
 */
void codegen_error_unsupported_feature(ASTNodeIdx_t node_idx, const char* feature) {
    (void)node_idx;  // Suppress unused parameter warning
    char message[256];
    char suggestion[256];
    
    snprintf(message, sizeof(message), "Unsupported feature: %s",
            feature ? feature : "unknown feature");
    snprintf(suggestion, sizeof(suggestion),
            "This feature is not yet implemented for %s target",
            g_codegen_context.target_architecture);
    
    // Convert AST node to token for location (simplified)
    TokenIdx_t token_idx = 0;  // Would need AST-to-token mapping
    
    error_report_with_context(ERROR_ERROR, ERROR_CODEGEN, token_idx,
                             CODEGEN_ERROR_UNSUPPORTED_FEATURE, message, suggestion,
                             &g_codegen_context);
}

/**
 * @brief Report register spill error
 */
void codegen_error_register_spill(ASTNodeIdx_t node_idx) {
    (void)node_idx;  // Suppress unused parameter warning
    error_report_with_context(ERROR_WARNING, ERROR_CODEGEN, 0,
                             CODEGEN_ERROR_REGISTER_SPILL,
                             "Register spill required - performance may be affected",
                             "Consider simplifying the expression or using fewer variables",
                             &g_codegen_context);
}
