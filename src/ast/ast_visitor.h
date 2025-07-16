/**
 * @file ast_visitor.h
 * @brief AST visitor pattern for modular compiler phases
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#ifndef SRC_AST_VISITOR_H_
#define SRC_AST_VISITOR_H_

#include "ast_types.h"

/**
 * @brief AST visitor function type
 * @param node_idx The AST node being visited
 * @param context User-defined context data
 * @return 0 to continue traversal, non-zero to stop
 */
typedef int (*ast_visit_func_t)(ASTNodeIdx_t node_idx, void* context);

/**
 * @brief AST visitor callbacks for different node types
 */
typedef struct ASTVisitor {
    // Pre/post order callbacks
    ast_visit_func_t pre_visit;   // Called before visiting children
    ast_visit_func_t post_visit;  // Called after visiting children
    
    // Category-specific callbacks
    ast_visit_func_t visit_declaration;
    ast_visit_func_t visit_statement;
    ast_visit_func_t visit_expression;
    ast_visit_func_t visit_type;
    
    // Specific node type callbacks (optional)
    ast_visit_func_t visit_function_def;
    ast_visit_func_t visit_var_decl;
    ast_visit_func_t visit_if_stmt;
    ast_visit_func_t visit_while_stmt;
    ast_visit_func_t visit_binary_expr;
    ast_visit_func_t visit_call_expr;
    ast_visit_func_t visit_identifier;
    ast_visit_func_t visit_literal;
    
    // Error handling
    ast_visit_func_t visit_error;
    
    // User context
    void* context;
    
    // Traversal options
    int max_depth;            // Maximum recursion depth (0 = unlimited)
    ASTNodeFlags skip_flags;  // Skip nodes with these flags
    ASTNodeFlags only_flags;  // Only visit nodes with these flags
} ASTVisitor;

// Visitor initialization
void ast_visitor_init(ASTVisitor* visitor);
void ast_visitor_set_context(ASTVisitor* visitor, void* context);

// Core traversal functions
int ast_visit_node(ASTVisitor* visitor, ASTNodeIdx_t node_idx);
int ast_visit_subtree(ASTVisitor* visitor, ASTNodeIdx_t root_idx);
int ast_visit_children(ASTVisitor* visitor, ASTNodeIdx_t node_idx);

// Specialized traversal patterns
int ast_visit_preorder(ASTVisitor* visitor, ASTNodeIdx_t root_idx);
int ast_visit_postorder(ASTVisitor* visitor, ASTNodeIdx_t root_idx);
int ast_visit_level_order(ASTVisitor* visitor, ASTNodeIdx_t root_idx);

// Utility visitors for common tasks
typedef struct {
    ASTNodeType target_type;
    ASTNodeIdx_t* results;
    int max_results;
    int found_count;
} FindNodesContext;

int ast_find_nodes_by_type(ASTNodeIdx_t root_idx, ASTNodeType type,
                          ASTNodeIdx_t* results, int max_results);

typedef struct {
    int node_count;
    int max_depth;
    int current_depth;
    size_t total_memory;
} TreeStatsContext;

void ast_get_tree_stats(ASTNodeIdx_t root_idx, TreeStatsContext* stats);

// Transformation visitor support
typedef struct {
    ASTNodeIdx_t old_node;
    ASTNodeIdx_t new_node;
} NodeReplacement;

typedef struct {
    NodeReplacement* replacements;
    int replacement_count;
    int max_replacements;
    int changes_made;
} TransformContext;

int ast_transform_tree(ASTNodeIdx_t root_idx, TransformContext* transform);

#endif  // SRC_AST_VISITOR_H_
