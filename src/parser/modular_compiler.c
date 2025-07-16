/**
 * @file modular_compiler.c
 * @brief Example of using the modular AST system across compiler phases
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#include <stdio.h>
#include <stdlib.h>

#include "ast_builder.h"
#include "ast_visitor.h"
#include "error_handler.h"
#include "sstore.h"
#include "tstore.h"
#include "hmapbuf.h"

/**
 * @brief Semantic analysis visitor context
 */
typedef struct {
    ASTBuilder* builder;
    int current_scope_depth;
    int error_count;
} SemanticContext;

/**
 * @brief Type checking visitor
 */
static int semantic_analyze_node(ASTNodeIdx_t node_idx, void* context) {
    SemanticContext* ctx = (SemanticContext*)context;
    HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
    
    if (!hb_node) return 0;
    
    // Skip nodes that already have semantic analysis
    if (ast_has_flag(node_idx, AST_FLAG_ANALYZED)) {
        return 0;  // Continue traversal
    }
    
    switch (hb_node->ast.type) {
        case AST_FUNCTION_DEF:
            printf("[SEMANTIC] Analyzing function definition\n");
            // Perform function-specific semantic analysis
            // - Check return type consistency
            // - Validate parameter types
            // - Ensure all paths return a value (if non-void)
            ast_set_flag(node_idx, AST_FLAG_ANALYZED);
            break;
            
        case AST_VAR_DECL:
            printf("[SEMANTIC] Analyzing variable declaration\n");
            // Perform variable declaration analysis
            // - Check for redeclaration in same scope
            // - Validate initializer type compatibility
            // - Add to symbol table
            ast_set_flag(node_idx, AST_FLAG_ANALYZED);
            break;
            
        case AST_EXPR_BINARY_OP:
            printf("[SEMANTIC] Analyzing binary operation\n");
            // Perform binary operation type checking
            // - Check operand type compatibility
            // - Determine result type
            // - Check for division by zero (if constant)
            ast_set_flag(node_idx, AST_FLAG_ANALYZED | AST_FLAG_TYPED);
            break;
            
        case AST_STMT_IF:
            printf("[SEMANTIC] Analyzing if statement\n");
            // Validate condition is boolean-convertible
            ast_set_flag(node_idx, AST_FLAG_ANALYZED);
            break;
            
        default:
            // Mark as analyzed for nodes that don't need special handling
            ast_set_flag(node_idx, AST_FLAG_ANALYZED);
            break;
    }
    
    return 0;  // Continue traversal
}

/**
 * @brief Code generation visitor context
 */
typedef struct {
    ASTBuilder* builder;
    FILE* output_file;
    int temp_var_counter;
    int label_counter;
} CodeGenContext;

/**
 * @brief Code generation visitor
 */
static int generate_code_node(ASTNodeIdx_t node_idx, void* context) {
    CodeGenContext* ctx = (CodeGenContext*)context;
    HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
    
    if (!hb_node) return 0;
    
    // Only generate code for analyzed nodes
    if (!ast_has_flag(node_idx, AST_FLAG_ANALYZED)) {
        return 0;
    }
    
    // Skip nodes that already have code generated
    if (ast_has_flag(node_idx, AST_FLAG_CODEGEN)) {
        return 0;
    }
    
    switch (hb_node->ast.type) {
        case AST_FUNCTION_DEF:
            fprintf(ctx->output_file, "# Function definition\n");
            fprintf(ctx->output_file, ".globl function_name\n");
            fprintf(ctx->output_file, "function_name:\n");
            ast_set_flag(node_idx, AST_FLAG_CODEGEN);
            break;
            
        case AST_LIT_INTEGER:
            fprintf(ctx->output_file, "    mov $%lld, %%rax  # Integer literal\n",
                   hb_node->ast.binary.value.long_value);
            ast_set_flag(node_idx, AST_FLAG_CODEGEN);
            break;
            
        case AST_EXPR_BINARY_OP: {
            // Assuming left and right operands are already processed
            TokenID_t op = hb_node->ast.unary.operator;  // Stored in unary for space
            switch (op) {
                case T_PLUS:
                    fprintf(ctx->output_file, "    add %%rbx, %%rax  # Addition\n");
                    break;
                case T_MINUS:
                    fprintf(ctx->output_file, "    sub %%rbx, %%rax  # Subtraction\n");
                    break;
                case T_MULT:
                    fprintf(ctx->output_file, "    imul %%rbx, %%rax  # Multiplication\n");
                    break;
                default:
                    fprintf(ctx->output_file, "    # Unsupported operator\n");
                    break;
            }
            ast_set_flag(node_idx, AST_FLAG_CODEGEN);
            break;
        }
        
        case AST_STMT_RETURN:
            fprintf(ctx->output_file, "    ret  # Return statement\n");
            ast_set_flag(node_idx, AST_FLAG_CODEGEN);
            break;
            
        default:
            ast_set_flag(node_idx, AST_FLAG_CODEGEN);
            break;
    }
    
    return 0;  // Continue traversal
}

/**
 * @brief Optimization visitor - simple constant folding example
 */
static int optimize_node(ASTNodeIdx_t node_idx, void* context) {
    HBNode* hb_node = HBGet(node_idx, HBMODE_AST);
    if (!hb_node) return 0;
    
    // Only optimize analyzed nodes
    if (!ast_has_flag(node_idx, AST_FLAG_ANALYZED)) {
        return 0;
    }
    
    if (hb_node->ast.type == AST_EXPR_BINARY_OP) {
        // Check if both operands are integer literals
        ASTNodeIdx_t left_idx = hb_node->ast.binary.left;
        ASTNodeIdx_t right_idx = hb_node->ast.binary.right;
        
        HBNode* left_node = HBGet(left_idx, HBMODE_AST);
        HBNode* right_node = HBGet(right_idx, HBMODE_AST);
        
        if (left_node && right_node &&
            left_node->ast.type == AST_LIT_INTEGER &&
            right_node->ast.type == AST_LIT_INTEGER) {
            
            // Perform constant folding
            int64_t left_val = left_node->ast.binary.value.long_value;
            int64_t right_val = right_node->ast.binary.value.long_value;
            int64_t result = 0;
            
            TokenID_t op = hb_node->ast.unary.operator;
            switch (op) {
                case T_PLUS:
                    result = left_val + right_val;
                    break;
                case T_MINUS:
                    result = left_val - right_val;
                    break;
                case T_MULT:
                    result = left_val * right_val;
                    break;
                default:
                    return 0;  // Can't optimize this operator
            }
            
            // Transform binary op into literal
            hb_node->ast.type = AST_LIT_INTEGER;
            hb_node->ast.binary.value.long_value = result;
            hb_node->ast.binary.left = 0;
            hb_node->ast.binary.right = 0;
            
            printf("[OPTIMIZE] Constant folded: %lld %c %lld = %lld\n",
                   left_val, (op == T_PLUS ? '+' : op == T_MINUS ? '-' : '*'),
                   right_val, result);
            
            ast_set_flag(node_idx, AST_FLAG_OPTIMIZED);
        }
    }
    
    return 0;
}

/**
 * @brief Main compilation driver demonstrating modular AST usage
 */
int modular_compile(const char* sstore_file, const char* token_file,
                   const char* ast_file, const char* output_file) {
    
    printf("=== STCC1 Modular Compiler ===\n");
    
    // Initialize all storage systems
    if (sstore_open(sstore_file) != 0) {
        fprintf(stderr, "Error: Cannot open string store file\n");
        return 1;
    }
    
    if (tstore_open(token_file) != 0) {
        fprintf(stderr, "Error: Cannot open token file\n");
        sstore_close();
        return 1;
    }
    
    HBInit();
    error_init();
    
    printf("Phase 1: Parsing (AST Construction)\n");
    ASTBuilder parser_builder;
    ast_builder_init(&parser_builder, "Parser");
    
    // This would normally call the enhanced parser
    // For demo, create a simple AST manually
    ASTNodeIdx_t main_func = ast_build_function_def(&parser_builder, 0, 0);
    ASTNodeIdx_t return_stmt = ast_build_return_stmt(&parser_builder, 0, 0);
    
    ast_builder_cleanup(&parser_builder);
    
    printf("\nPhase 2: Semantic Analysis\n");
    ASTBuilder semantic_builder;
    ast_builder_init(&semantic_builder, "Semantic Analysis");
    
    ASTVisitor semantic_visitor;
    ast_visitor_init(&semantic_visitor);
    semantic_visitor.pre_visit = semantic_analyze_node;
    
    SemanticContext semantic_ctx = {&semantic_builder, 0, 0};
    semantic_visitor.context = &semantic_ctx;
    
    ast_visit_subtree(&semantic_visitor, main_func);
    
    ast_builder_cleanup(&semantic_builder);
    
    printf("\nPhase 3: Optimization\n");
    ASTVisitor optimizer;
    ast_visitor_init(&optimizer);
    optimizer.post_visit = optimize_node;  // Post-order for bottom-up optimization
    
    ast_visit_subtree(&optimizer, main_func);
    
    printf("\nPhase 4: Code Generation\n");
    FILE* output = fopen(output_file, "w");
    if (!output) {
        fprintf(stderr, "Error: Cannot create output file\n");
        return 1;
    }
    
    ASTBuilder codegen_builder;
    ast_builder_init(&codegen_builder, "Code Generation");
    
    ASTVisitor codegen_visitor;
    ast_visitor_init(&codegen_visitor);
    codegen_visitor.pre_visit = generate_code_node;
    
    CodeGenContext codegen_ctx = {&codegen_builder, output, 0, 0};
    codegen_visitor.context = &codegen_ctx;
    
    ast_visit_subtree(&codegen_visitor, main_func);
    
    fclose(output);
    ast_builder_cleanup(&codegen_builder);
    
    printf("\nPhase 5: Cleanup and Statistics\n");
    TreeStatsContext stats;
    ast_get_tree_stats(main_func, &stats);
    
    printf("AST Statistics:\n");
    printf("  Nodes: %d\n", stats.node_count);
    printf("  Max depth: %d\n", stats.max_depth);
    printf("  Memory usage: %zu bytes\n", stats.total_memory);
    
    error_print_summary();
    
    // Cleanup
    HBEnd();
    tstore_close();
    sstore_close();
    
    printf("=== Compilation Complete ===\n");
    return 0;
}
