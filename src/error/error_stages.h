/**
 * @file error_stages.h
 * @brief Stage-specific error handling interfaces
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#ifndef SRC_ERROR_STAGES_H_
#define SRC_ERROR_STAGES_H_

#include "error_core.h"
#include "../lexer/ctoken.h"
#include "../ast/ast_types.h"

// ============================================================================
// LEXICAL ANALYSIS ERROR HANDLER
// ============================================================================

// Lexical error codes
typedef enum {
    LEX_ERROR_INVALID_CHAR = 1000,
    LEX_ERROR_UNTERMINATED_STRING,
    LEX_ERROR_UNTERMINATED_CHAR,
    LEX_ERROR_INVALID_ESCAPE,
    LEX_ERROR_INVALID_NUMBER,
    LEX_ERROR_IDENTIFIER_TOO_LONG,
    LEX_ERROR_EOF_IN_COMMENT
} LexicalErrorCode_t;

// Lexical error context
typedef struct {
    const char* input_buffer;
    size_t buffer_pos;
    char unexpected_char;
    const char* token_start;
    size_t token_length;
} LexicalErrorContext_t;

// Lexical error handler interface
void lex_error_init(void);
void lex_error_cleanup(void);

void lex_error_invalid_char(TokenIdx_t token_idx, char invalid_char);
void lex_error_unterminated_string(TokenIdx_t token_idx);
void lex_error_unterminated_char(TokenIdx_t token_idx);
void lex_error_invalid_escape(TokenIdx_t token_idx, char escape_char);
void lex_error_invalid_number(TokenIdx_t token_idx, const char* number_text);
void lex_error_identifier_too_long(TokenIdx_t token_idx, size_t length);

// ============================================================================
// SYNTAX ANALYSIS ERROR HANDLER
// ============================================================================

// Syntax error codes
typedef enum {
    SYNTAX_ERROR_UNEXPECTED_TOKEN = 2000,
    SYNTAX_ERROR_MISSING_TOKEN,
    SYNTAX_ERROR_EXTRA_TOKEN,
    SYNTAX_ERROR_INVALID_EXPRESSION,
    SYNTAX_ERROR_INVALID_STATEMENT,
    SYNTAX_ERROR_INVALID_DECLARATION,
    SYNTAX_ERROR_UNMATCHED_BRACE,
    SYNTAX_ERROR_UNMATCHED_PAREN,
    SYNTAX_ERROR_INVALID_FUNCTION_DEF
} SyntaxErrorCode_t;

// Syntax error context
typedef struct {
    TokenID_t expected_token;
    TokenID_t found_token;
    const char* production_rule;
    int parser_state;
    TokenIdx_t* token_stack;
    int stack_depth;
} SyntaxErrorContext_t;

// Error recovery strategy
typedef enum {
    RECOVERY_NONE,
    RECOVERY_SYNC_TOKEN,
    RECOVERY_SKIP_TO_SEMICOLON,
    RECOVERY_SKIP_TO_BRACE,
    RECOVERY_RESTART_STATEMENT,
    RECOVERY_RESTART_DECLARATION
} RecoveryStrategy_t;

// Syntax error handler interface
void syntax_error_init(void);
void syntax_error_cleanup(void);

void syntax_error_unexpected_token(TokenIdx_t token_idx, TokenID_t expected,
                                  TokenID_t found);
void syntax_error_missing_token(TokenIdx_t token_idx, TokenID_t missing);
void syntax_error_invalid_expression(TokenIdx_t token_idx, const char* context);
void syntax_error_invalid_statement(TokenIdx_t token_idx, const char* context);
void syntax_error_unmatched_delimiter(TokenIdx_t token_idx, char delimiter);

// Error recovery
RecoveryStrategy_t syntax_suggest_recovery(const SyntaxErrorContext_t* context);
int syntax_attempt_recovery(RecoveryStrategy_t strategy);

// ============================================================================
// SEMANTIC ANALYSIS ERROR HANDLER  
// ============================================================================

// Semantic error codes
typedef enum {
    SEMANTIC_ERROR_UNDEFINED_SYMBOL = 3000,
    SEMANTIC_ERROR_REDEFINED_SYMBOL,
    SEMANTIC_ERROR_TYPE_MISMATCH,
    SEMANTIC_ERROR_INVALID_ASSIGNMENT,
    SEMANTIC_ERROR_INVALID_OPERATION,
    SEMANTIC_ERROR_FUNCTION_CALL_MISMATCH,
    SEMANTIC_ERROR_RETURN_TYPE_MISMATCH,
    SEMANTIC_ERROR_SCOPE_VIOLATION,
    SEMANTIC_ERROR_CONST_VIOLATION
} SemanticErrorCode_t;

// Semantic error context
typedef struct {
    const char* symbol_name;
    TypeIdx_t expected_type;
    TypeIdx_t found_type;
    SymTabIdx_t symbol_idx;
    const char* scope_name;
    int scope_level;
} SemanticErrorContext_t;

// Semantic error handler interface
void semantic_error_init(void);
void semantic_error_cleanup(void);

void semantic_error_undefined_symbol(TokenIdx_t token_idx, const char* symbol_name);
void semantic_error_redefined_symbol(TokenIdx_t token_idx, const char* symbol_name,
                                    TokenIdx_t first_definition);
void semantic_error_type_mismatch(TokenIdx_t token_idx, TypeIdx_t expected,
                                 TypeIdx_t found, const char* context);
void semantic_error_invalid_assignment(TokenIdx_t token_idx, TypeIdx_t lhs_type,
                                      TypeIdx_t rhs_type);
void semantic_error_function_call_mismatch(TokenIdx_t token_idx,
                                          const char* function_name,
                                          int expected_args, int found_args);

// ============================================================================
// CODE GENERATION ERROR HANDLER
// ============================================================================

// Code generation error codes
typedef enum {
    CODEGEN_ERROR_UNSUPPORTED_FEATURE = 4000,
    CODEGEN_ERROR_REGISTER_SPILL,
    CODEGEN_ERROR_INVALID_TARGET,
    CODEGEN_ERROR_ASSEMBLY_ERROR,
    CODEGEN_ERROR_OPTIMIZATION_FAILURE
} CodegenErrorCode_t;

// Code generation error context
typedef struct {
    const char* target_architecture;
    const char* instruction_template;
    int register_count;
    int available_registers;
    ASTNodeIdx_t problematic_node;
} CodegenErrorContext_t;

// Code generation error handler interface
void codegen_error_init(void);
void codegen_error_cleanup(void);

void codegen_error_unsupported_feature(ASTNodeIdx_t node_idx, const char* feature);
void codegen_error_register_spill(ASTNodeIdx_t node_idx);
void codegen_error_invalid_target(const char* target);
void codegen_error_assembly_error(ASTNodeIdx_t node_idx, const char* instruction);

// ============================================================================
// UNIFIED ERROR HANDLER INTERFACE
// ============================================================================

// Initialize all stage-specific error handlers
void error_stages_init_all(void);
void error_stages_cleanup_all(void);

// Stage context management
void error_set_current_stage(const char* stage_name);
const char* error_get_current_stage(void);

// Common error patterns across stages
void error_report_with_context(ErrorLevel_t level, ErrorCategory_t category,
                              TokenIdx_t token_idx, uint32_t error_code,
                              const char* message, const char* suggestion,
                              void* stage_context);

// Cross-stage error correlation
void error_add_related_error(CompilerError_t* primary, CompilerError_t* related);
void error_print_error_chain(const CompilerError_t* primary);

#endif  // SRC_ERROR_STAGES_H_
