/**
 * @file error_recovery.h
 * @brief Error recovery strategies for different compiler stages
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#ifndef SRC_ERROR_ERROR_RECOVERY_H_
#define SRC_ERROR_ERROR_RECOVERY_H_

#include "error_core.h"
#include "ctoken.h"

// ============================================================================
// GENERIC ERROR RECOVERY
// ============================================================================

// Recovery actions that can be taken
typedef enum {
    RECOVERY_ACTION_NONE,
    RECOVERY_ACTION_SKIP_TOKEN,
    RECOVERY_ACTION_INSERT_TOKEN,
    RECOVERY_ACTION_REPLACE_TOKEN,
    RECOVERY_ACTION_SYNC_TO_TOKEN,
    RECOVERY_ACTION_RESTART_PRODUCTION,
    RECOVERY_ACTION_ABORT_PRODUCTION
} RecoveryAction_t;

// Recovery context for decision making
typedef struct {
    TokenIdx_t error_token;        // Token where error occurred
    TokenID_t expected_tokens[8];  // Tokens that would be valid
    int expected_count;
    TokenID_t sync_tokens[8];      // Tokens to synchronize to
    int sync_count;
    const char* production_name;   // Grammar production being parsed
    int confidence_level;          // 0-100, confidence in recovery
} RecoveryContext_t;

// Recovery result
typedef struct {
    RecoveryAction_t action;
    TokenID_t suggested_token;     // For insert/replace actions
    TokenIdx_t sync_target;        // For sync actions
    int tokens_to_skip;           // For skip actions
    const char* explanation;       // Human-readable explanation
} RecoveryResult_t;

// Recovery strategy function type
typedef RecoveryResult_t (*recovery_strategy_func_t)(const RecoveryContext_t* context);

// Generic recovery functions
RecoveryResult_t recovery_suggest_action(const RecoveryContext_t* context);
int recovery_attempt_action(const RecoveryResult_t* result);
void recovery_init_context(RecoveryContext_t* context, TokenIdx_t error_token);
void recovery_add_expected_token(RecoveryContext_t* context, TokenID_t token);
void recovery_add_sync_token(RecoveryContext_t* context, TokenID_t token);

// ============================================================================
// LEXICAL ERROR RECOVERY
// ============================================================================

// Lexical recovery strategies
RecoveryResult_t lex_recovery_invalid_char(const RecoveryContext_t* context);
RecoveryResult_t lex_recovery_unterminated_string(const RecoveryContext_t* context);
RecoveryResult_t lex_recovery_invalid_number(const RecoveryContext_t* context);

// Lexical recovery helpers
int lex_skip_to_whitespace(void);
int lex_skip_to_newline(void);
int lex_insert_missing_quote(char quote_char);

// ============================================================================
// SYNTAX ERROR RECOVERY
// ============================================================================

// Syntax recovery strategies
RecoveryResult_t syntax_recovery_missing_semicolon(const RecoveryContext_t* context);
RecoveryResult_t syntax_recovery_missing_brace(const RecoveryContext_t* context);
RecoveryResult_t syntax_recovery_unexpected_token(const RecoveryContext_t* context);
RecoveryResult_t syntax_recovery_invalid_expression(const RecoveryContext_t* context);

// Syntax recovery helpers
int syntax_sync_to_statement_end(void);
int syntax_sync_to_declaration_start(void);
int syntax_skip_balanced_delimiters(char open_delim, char close_delim);
int syntax_find_matching_delimiter(TokenID_t delimiter);

// Statement-level recovery
typedef enum {
    STMT_RECOVERY_SKIP_TO_SEMICOLON,
    STMT_RECOVERY_SKIP_TO_BRACE,
    STMT_RECOVERY_INSERT_SEMICOLON,
    STMT_RECOVERY_RESTART_STATEMENT
} StatementRecoveryStrategy_t;

StatementRecoveryStrategy_t syntax_choose_statement_recovery(const RecoveryContext_t* context);

// Expression-level recovery
typedef enum {
    EXPR_RECOVERY_REPLACE_WITH_ZERO,
    EXPR_RECOVERY_REPLACE_WITH_IDENTIFIER,
    EXPR_RECOVERY_SKIP_OPERAND,
    EXPR_RECOVERY_SKIP_OPERATOR
} ExpressionRecoveryStrategy_t;

ExpressionRecoveryStrategy_t syntax_choose_expression_recovery(const RecoveryContext_t* context);

// ============================================================================
// SEMANTIC ERROR RECOVERY
// ============================================================================

// Semantic recovery strategies
RecoveryResult_t semantic_recovery_undefined_symbol(const RecoveryContext_t* context);
RecoveryResult_t semantic_recovery_type_mismatch(const RecoveryContext_t* context);
RecoveryResult_t semantic_recovery_redefined_symbol(const RecoveryContext_t* context);

// Type recovery helpers
typedef struct {
    TypeIdx_t original_type;
    TypeIdx_t suggested_type;
    int confidence;
} TypeSuggestion_t;

TypeSuggestion_t semantic_suggest_type_cast(TypeIdx_t from_type, TypeIdx_t to_type);
int semantic_can_implicit_cast(TypeIdx_t from_type, TypeIdx_t to_type);

// Symbol recovery helpers
typedef struct {
    const char* suggested_name;
    SymTabIdx_t suggested_symbol;
    int edit_distance;
} SymbolSuggestion_t;

SymbolSuggestion_t semantic_suggest_similar_symbol(const char* undefined_symbol);
int semantic_calculate_edit_distance(const char* s1, const char* s2);

// ============================================================================
// RECOVERY QUALITY ASSESSMENT
// ============================================================================

// Recovery quality metrics
typedef struct {
    int syntax_errors_fixed;
    int semantic_errors_introduced;
    int tokens_skipped;
    int nodes_discarded;
    double confidence_score;  // 0.0 to 1.0
} RecoveryQuality_t;

RecoveryQuality_t recovery_assess_quality(const RecoveryResult_t* result,
                                         const RecoveryContext_t* context);
int recovery_is_worthwhile(const RecoveryQuality_t* quality);

// Recovery history for learning
typedef struct {
    RecoveryAction_t action;
    ErrorCategory_t error_category;
    uint32_t error_code;
    int success_count;
    int failure_count;
    double average_quality;
} RecoveryStats_t;

void recovery_record_attempt(const RecoveryResult_t* result,
                            const RecoveryQuality_t* quality);
void recovery_update_strategy_effectiveness(ErrorCategory_t category,
                                           RecoveryAction_t action,
                                           int was_successful);

// ============================================================================
// RECOVERY CONFIGURATION
// ============================================================================

// Recovery preferences
typedef struct {
    int enable_aggressive_recovery;  // Try harder recovery strategies
    int max_tokens_to_skip;         // Limit for skip-based recovery
    int max_cascade_errors;         // Stop recovery after this many cascade errors
    double min_confidence_threshold; // Minimum confidence to attempt recovery
    int prefer_insertion_over_deletion; // Preference for recovery type
} RecoveryConfig_t;

void recovery_set_config(const RecoveryConfig_t* config);
RecoveryConfig_t recovery_get_default_config(void);

#endif  // SRC_ERROR_ERROR_RECOVERY_H_
