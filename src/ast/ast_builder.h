/**
 * @file ast_builder.h
 * @brief AST construction utilities for modular compiler design
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#ifndef SRC_AST_AST_BUILDER_H_
#define SRC_AST_AST_BUILDER_H_

#include "ast_types.h"
#include "../utils/hmapbuf.h"

/**
 * @brief AST builder context for different compiler phases
 */
typedef struct ASTBuilder {
    const char* phase_name;     // Current compiler phase
    int error_count;           // Number of errors in this phase
    int warning_count;         // Number of warnings in this phase
    ASTNodeFlags default_flags; // Default flags for new nodes
} ASTBuilder;

// AST builder initialization and cleanup
void ast_builder_init(ASTBuilder* builder, const char* phase_name);
void ast_builder_cleanup(ASTBuilder* builder);

// Core node creation functions
ASTNodeIdx_t ast_create_node(ASTBuilder* builder, ASTNodeType type, TokenIdx_t token_idx);
ASTNodeIdx_t ast_create_typed_node(ASTBuilder* builder, ASTNodeType type,
                                  TokenIdx_t token_idx, TypeIdx_t type_idx);

// Declaration node builders
ASTNodeIdx_t ast_build_function_decl(ASTBuilder* builder, TokenIdx_t name_token,
                                    TypeIdx_t return_type, ASTNodeIdx_t params);
ASTNodeIdx_t ast_build_function_def(ASTBuilder* builder, ASTNodeIdx_t decl,
                                   ASTNodeIdx_t body);
ASTNodeIdx_t ast_build_var_decl(ASTBuilder* builder, TokenIdx_t name_token,
                               TypeIdx_t type_idx, ASTNodeIdx_t initializer);
ASTNodeIdx_t ast_build_param_decl(ASTBuilder* builder, TokenIdx_t name_token,
                                 TypeIdx_t type_idx);

// Statement node builders
ASTNodeIdx_t ast_build_compound_stmt(ASTBuilder* builder, TokenIdx_t brace_token,
                                    ASTNodeIdx_t statements);
ASTNodeIdx_t ast_build_if_stmt(ASTBuilder* builder, TokenIdx_t if_token,
                              ASTNodeIdx_t condition, ASTNodeIdx_t then_stmt,
                              ASTNodeIdx_t else_stmt);
ASTNodeIdx_t ast_build_while_stmt(ASTBuilder* builder, TokenIdx_t while_token,
                                 ASTNodeIdx_t condition, ASTNodeIdx_t body);
ASTNodeIdx_t ast_build_for_stmt(ASTBuilder* builder, TokenIdx_t for_token,
                               ASTNodeIdx_t init, ASTNodeIdx_t condition,
                               ASTNodeIdx_t update, ASTNodeIdx_t body);
ASTNodeIdx_t ast_build_return_stmt(ASTBuilder* builder, TokenIdx_t return_token,
                                  ASTNodeIdx_t expression);
ASTNodeIdx_t ast_build_expression_stmt(ASTBuilder* builder, ASTNodeIdx_t expression);

// Expression node builders
ASTNodeIdx_t ast_build_binary_expr(ASTBuilder* builder, TokenIdx_t op_token,
                                  ASTNodeIdx_t left, ASTNodeIdx_t right);
ASTNodeIdx_t ast_build_unary_expr(ASTBuilder* builder, TokenIdx_t op_token,
                                 ASTNodeIdx_t operand);
ASTNodeIdx_t ast_build_assign_expr(ASTBuilder* builder, TokenIdx_t assign_token,
                                  ASTNodeIdx_t left, ASTNodeIdx_t right);
ASTNodeIdx_t ast_build_call_expr(ASTBuilder* builder, TokenIdx_t paren_token,
                                ASTNodeIdx_t function, ASTNodeIdx_t arguments);
ASTNodeIdx_t ast_build_member_expr(ASTBuilder* builder, TokenIdx_t dot_token,
                                  ASTNodeIdx_t object, TokenIdx_t member);
ASTNodeIdx_t ast_build_index_expr(ASTBuilder* builder, TokenIdx_t bracket_token,
                                 ASTNodeIdx_t array, ASTNodeIdx_t index);

// Literal node builders
ASTNodeIdx_t ast_build_integer_literal(ASTBuilder* builder, TokenIdx_t token,
                                      int64_t value);
ASTNodeIdx_t ast_build_float_literal(ASTBuilder* builder, TokenIdx_t token,
                                    double value);
ASTNodeIdx_t ast_build_char_literal(ASTBuilder* builder, TokenIdx_t token,
                                   char value);
ASTNodeIdx_t ast_build_string_literal(ASTBuilder* builder, TokenIdx_t token,
                                     sstore_pos_t string_pos);
ASTNodeIdx_t ast_build_identifier(ASTBuilder* builder, TokenIdx_t token,
                                 SymTabIdx_t symbol_idx);

// List management
ASTNodeIdx_t ast_create_list(ASTBuilder* builder);
ASTNodeIdx_t ast_append_to_list(ASTBuilder* builder, ASTNodeIdx_t list,
                               ASTNodeIdx_t item);
int ast_get_list_length(ASTNodeIdx_t list);
ASTNodeIdx_t ast_get_list_item(ASTNodeIdx_t list, int index);

// Node modification and annotation
void ast_set_flag(ASTNodeIdx_t node_idx, ASTNodeFlags flag);
void ast_clear_flag(ASTNodeIdx_t node_idx, ASTNodeFlags flag);
int ast_has_flag(ASTNodeIdx_t node_idx, ASTNodeFlags flag);
void ast_set_type(ASTNodeIdx_t node_idx, TypeIdx_t type_idx);
TypeIdx_t ast_get_type(ASTNodeIdx_t node_idx);

// Node validation and debugging
int ast_validate_node(ASTNodeIdx_t node_idx);
int ast_validate_subtree(ASTNodeIdx_t root_idx);
void ast_print_node(ASTNodeIdx_t node_idx, int indent);
void ast_print_subtree(ASTNodeIdx_t root_idx, int max_depth);

// Memory and performance utilities
size_t ast_calculate_subtree_size(ASTNodeIdx_t root_idx);
int ast_count_nodes_by_type(ASTNodeIdx_t root_idx, ASTNodeType type);
void ast_collect_garbage(void);

#endif  // SRC_AST_AST_BUILDER_H_
