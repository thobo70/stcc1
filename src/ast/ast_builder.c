/**
 * @file ast_builder.c
 * @brief Implementation of AST construction utilities
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "ast_builder.h"
#include "../utils/hmapbuf.h"
#include "../storage/tstore.h"

/**
 * @brief Initialize AST builder for a compiler phase
 */
void ast_builder_init(ASTBuilder* builder, const char* phase_name) {
    if (!builder) return;

    memset(builder, 0, sizeof(ASTBuilder));
    builder->phase_name = phase_name;
    builder->default_flags = AST_FLAG_PARSED;

    printf("[AST] Initializing builder for phase: %s\n", phase_name);
}

/**
 * @brief Cleanup AST builder
 */
void ast_builder_cleanup(ASTBuilder* builder) {
    if (!builder) return;

    printf("[AST] %s phase complete: %d errors, %d warnings\n",
           builder->phase_name, builder->error_count, builder->warning_count);

    memset(builder, 0, sizeof(ASTBuilder));
}

/**
 * @brief Create a basic AST node
 */
ASTNodeIdx_t ast_create_node(ASTBuilder* builder, ASTNodeType type,
                            TokenIdx_t token_idx) {
    HBNode* hb_node = HBNew(HBMODE_AST);
    if (!hb_node) {
        if (builder) builder->error_count++;
        return 0;
    }

    // Initialize the AST node
    memset(&hb_node->ast, 0, sizeof(ASTNode));
    hb_node->ast.type = type;
    hb_node->ast.token_idx = token_idx;
    hb_node->ast.flags = builder ? builder->default_flags : AST_FLAG_PARSED;

    return hb_node->idx;
}

/**
 * @brief Create a typed AST node
 */
ASTNodeIdx_t ast_create_typed_node(ASTBuilder* builder, ASTNodeType type,
                                  TokenIdx_t token_idx, TypeIdx_t type_idx) {
    ASTNodeIdx_t node_idx = ast_create_node(builder, type, token_idx);
    if (node_idx != 0) {
        HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
        if (hb_node) {
            hb_node->ast.type_idx = type_idx;
            hb_node->ast.flags |= AST_FLAG_TYPED;
        }
    }
    return node_idx;
}

/**
 * @brief Build a function declaration node
 */
ASTNodeIdx_t ast_build_function_decl(ASTBuilder* builder,
                                     TokenIdx_t name_token,
                                     TypeIdx_t return_type,
                                     ASTNodeIdx_t params) {
    ASTNodeIdx_t node_idx = ast_create_typed_node(builder,
                                                  AST_FUNCTION_DECL,
                                                  name_token,
                                                  return_type);
    if (node_idx != 0) {
        HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
        if (hb_node) {
            hb_node->ast.declaration.type_idx = return_type;
            hb_node->ast.children.child1 = params;  // Parameter list
        }
    }
    return node_idx;
}

/**
 * @brief Build a function definition node
 */
ASTNodeIdx_t ast_build_function_def(ASTBuilder* builder, ASTNodeIdx_t decl,
                                   ASTNodeIdx_t body) {
    if (decl == 0) return 0;

    // Get the declaration node to extract token info
    HBNode* decl_node = HBGet(decl, HBMODE_AST);
    if (!decl_node) return 0;

    ASTNodeIdx_t node_idx = ast_create_node(builder, AST_FUNCTION_DEF,
                                           decl_node->ast.token_idx);
    if (node_idx != 0) {
        HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
        if (hb_node) {
            hb_node->ast.children.child1 = decl;  // Function declaration
            hb_node->ast.children.child2 = body;  // Function body
        }
    }
    return node_idx;
}

/**
 * @brief Build a variable declaration node
 */
ASTNodeIdx_t ast_build_var_decl(ASTBuilder* builder, TokenIdx_t name_token,
                               TypeIdx_t type_idx, ASTNodeIdx_t initializer) {
    ASTNodeIdx_t node_idx = ast_create_typed_node(builder, AST_VAR_DECL,
                                                 name_token, type_idx);
    if (node_idx != 0) {
        HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
        if (hb_node) {
            hb_node->ast.declaration.type_idx = type_idx;
            hb_node->ast.declaration.initializer = initializer;
        }
    }
    return node_idx;
}

/**
 * @brief Build a compound statement (block)
 */
ASTNodeIdx_t ast_build_compound_stmt(ASTBuilder* builder,
                                     TokenIdx_t brace_token,
                                     ASTNodeIdx_t statements) {
    ASTNodeIdx_t node_idx = ast_create_node(builder,
                                            AST_STMT_COMPOUND,
                                            brace_token);
    if (node_idx != 0) {
        HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
        if (hb_node) {
            hb_node->ast.compound.statements = statements;
            // scope_idx would be set during semantic analysis
        }
    }
    return node_idx;
}

/**
 * @brief Build an if statement
 */
ASTNodeIdx_t ast_build_if_stmt(ASTBuilder* builder, TokenIdx_t if_token,
                              ASTNodeIdx_t condition, ASTNodeIdx_t then_stmt,
                              ASTNodeIdx_t else_stmt) {
    ASTNodeIdx_t node_idx = ast_create_node(builder, AST_STMT_IF, if_token);
    if (node_idx != 0) {
        HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
        if (hb_node) {
            hb_node->ast.conditional.condition = condition;
            hb_node->ast.conditional.then_stmt = then_stmt;
            hb_node->ast.conditional.else_stmt = else_stmt;
        }
    }
    return node_idx;
}

/**
 * @brief Build a while statement
 */
ASTNodeIdx_t ast_build_while_stmt(ASTBuilder* builder, TokenIdx_t while_token,
                                 ASTNodeIdx_t condition, ASTNodeIdx_t body) {
    ASTNodeIdx_t node_idx = ast_create_node(builder, AST_STMT_WHILE, while_token);
    if (node_idx != 0) {
        HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
        if (hb_node) {
            hb_node->ast.conditional.condition = condition;
            hb_node->ast.conditional.then_stmt = body;
            hb_node->ast.conditional.else_stmt = 0;  // No else in while
        }
    }
    return node_idx;
}

/**
 * @brief Build a return statement
 */
ASTNodeIdx_t ast_build_return_stmt(ASTBuilder* builder, TokenIdx_t return_token,
                                  ASTNodeIdx_t expression) {
    ASTNodeIdx_t node_idx = ast_create_node(builder, AST_STMT_RETURN, return_token);
    if (node_idx != 0) {
        HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
        if (hb_node) {
            hb_node->ast.children.child1 = expression;
        }
    }
    return node_idx;
}

/**
 * @brief Build a binary expression
 */
ASTNodeIdx_t ast_build_binary_expr(ASTBuilder* builder, TokenIdx_t op_token,
                                  ASTNodeIdx_t left, ASTNodeIdx_t right) {
    ASTNodeIdx_t node_idx = ast_create_node(builder, AST_EXPR_BINARY_OP, op_token);
    if (node_idx != 0) {
        HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
        if (hb_node) {
            hb_node->ast.binary.left = left;
            hb_node->ast.binary.right = right;
        }
    }
    return node_idx;
}

/**
 * @brief Build a unary expression
 */
ASTNodeIdx_t ast_build_unary_expr(ASTBuilder* builder, TokenIdx_t op_token,
                                 ASTNodeIdx_t operand) {
    ASTNodeIdx_t node_idx = ast_create_node(builder, AST_EXPR_UNARY_OP, op_token);
    if (node_idx != 0) {
        HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
        if (hb_node) {
            hb_node->ast.unary.operand = operand;
            // Extract operator from token
            Token_t token = tstore_get(op_token);
            hb_node->ast.unary.operator = token.id;
        }
    }
    return node_idx;
}

/**
 * @brief Build a function call expression
 */
ASTNodeIdx_t ast_build_call_expr(ASTBuilder* builder, TokenIdx_t paren_token,
                                ASTNodeIdx_t function, ASTNodeIdx_t arguments) {
    ASTNodeIdx_t node_idx = ast_create_node(builder, AST_EXPR_CALL, paren_token);
    if (node_idx != 0) {
        HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
        if (hb_node) {
            hb_node->ast.call.function = function;
            hb_node->ast.call.arguments = arguments;
            hb_node->ast.call.arg_count = 0;  // Will be computed later
        }
    }
    return node_idx;
}

/**
 * @brief Build an integer literal
 */
ASTNodeIdx_t ast_build_integer_literal(ASTBuilder* builder, TokenIdx_t token,
                                      int64_t value) {
    ASTNodeIdx_t node_idx = ast_create_node(builder, AST_LIT_INTEGER, token);
    if (node_idx != 0) {
        HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
        if (hb_node) {
            hb_node->ast.binary.value.long_value = value;
        }
    }
    return node_idx;
}

/**
 * @brief Build an identifier node
 */
ASTNodeIdx_t ast_build_identifier(ASTBuilder* builder, TokenIdx_t token,
                                 SymTabIdx_t symbol_idx) {
    ASTNodeIdx_t node_idx = ast_create_node(builder, AST_EXPR_IDENTIFIER, token);
    if (node_idx != 0) {
        HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
        if (hb_node) {
            hb_node->ast.binary.value.symbol_idx = symbol_idx;
        }
    }
    return node_idx;
}

/**
 * @brief Set a flag on an AST node
 */
void ast_set_flag(ASTNodeIdx_t node_idx, ASTNodeFlags flag) {
    HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
    if (hb_node) {
        hb_node->ast.flags |= flag;
        HBTouched(hb_node);  // Mark as modified
    }
}

/**
 * @brief Check if a node has a specific flag
 */
int ast_has_flag(ASTNodeIdx_t node_idx, ASTNodeFlags flag) {
    HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
    if (hb_node) {
        return (hb_node->ast.flags & flag) != 0;
    }
    return 0;
}

/**
 * @brief Print an AST node for debugging
 */
void ast_print_node(ASTNodeIdx_t node_idx, int indent) {
    HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
    if (!hb_node) {
        printf("%*sNULL NODE\n", indent, "");
        return;
    }

    const char* type_names[] = {
        "FREE", "PROGRAM", "TRANSLATION_UNIT", "EOF", "ERROR",
        [AST_FUNCTION_DECL] = "FUNCTION_DECL",
        [AST_FUNCTION_DEF] = "FUNCTION_DEF",
        [AST_VAR_DECL] = "VAR_DECL",
        [AST_STMT_COMPOUND] = "COMPOUND_STMT",
        [AST_STMT_IF] = "IF_STMT",
        [AST_STMT_WHILE] = "WHILE_STMT",
        [AST_STMT_RETURN] = "RETURN_STMT",
        [AST_EXPR_BINARY_OP] = "BINARY_OP",
        [AST_EXPR_UNARY_OP] = "UNARY_OP",
        [AST_EXPR_CALL] = "CALL",
        [AST_EXPR_IDENTIFIER] = "IDENTIFIER",
        [AST_LIT_INTEGER] = "INTEGER_LIT"
    };

    const char* type_name = (hb_node->ast.type < AST_TYPE_COUNT &&
                            type_names[hb_node->ast.type]) ?
                           type_names[hb_node->ast.type] : "UNKNOWN";

    printf("%*s%s (idx=%d, token=%d, flags=0x%x)\n",
           indent, "", type_name, node_idx,
           hb_node->ast.token_idx, hb_node->ast.flags);
}

/**
 * @brief Validate an AST node structure
 */
int ast_validate_node(ASTNodeIdx_t node_idx) {
    if (node_idx == 0) return 1;  // NULL nodes are valid

    HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
    if (!hb_node) return 0;  // Invalid node reference

    // Check if node type is valid
    if (hb_node->ast.type >= AST_TYPE_COUNT) return 0;

    // Category-specific validation could be added here
    ASTCategory category = ast_get_category(hb_node->ast.type);
    switch (category) {
        case AST_CAT_DECLARATION:
            // Validate declaration nodes
            break;
        case AST_CAT_STATEMENT:
            // Validate statement nodes
            break;
        case AST_CAT_EXPRESSION:
            // Validate expression nodes
            break;
        default:
            break;
    }

    return 1;  // Node is valid
}
