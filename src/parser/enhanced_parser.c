/**
 * @file enhanced_parser.c
 * @brief Enhanced parser implementation with modular AST support
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sstore.h"
#include "tstore.h"
#include "ast_builder.h"
#include "ast_visitor.h"
#include "error_handler.h"
#include "ctoken.h"

// Global parser state
static ASTBuilder g_parser_builder;
static Token_t current_token;
static TokenIdx_t current_token_idx;
static ErrorRecovery_t error_recovery;

// Forward declarations for recursive descent parser
static ASTNodeIdx_t parse_expression(void);
static ASTNodeIdx_t parse_statement(void);
static ASTNodeIdx_t parse_declaration(void);
static ASTNodeIdx_t parse_function_definition(void);

/**
 * @brief Initialize the enhanced parser
 */
void enhanced_parser_init(void) {
    ast_builder_init(&g_parser_builder, "Parser");
    error_recovery_init(&error_recovery);
    
    // Add common synchronization tokens for error recovery
    error_recovery_add_sync_token(&error_recovery, T_SEMICOLON);
    error_recovery_add_sync_token(&error_recovery, T_RBRACE);
    error_recovery_add_sync_token(&error_recovery, T_EOF);
    
    next_token();  // Initialize current token
}

/**
 * @brief Cleanup the enhanced parser
 */
void enhanced_parser_cleanup(void) {
    ast_builder_cleanup(&g_parser_builder);
}
/**
 * @brief Get the current token and advance
 */
static Token_t next_token(void) {
    current_token_idx = tstore_getidx();
    current_token = tstore_next();
    return current_token;
}

/**
 * @brief Peek at the current token without advancing
 */
static Token_t peek_token(void) {
    return current_token;
}

/**
 * @brief Check if current token matches expected type and consume it
 */
static int match_token(TokenID_t expected) {
    if (current_token.id == expected) {
        next_token();
        return 1;
    }
    return 0;
}

/**
 * @brief Report a syntax error with current token context
 */
static void report_syntax_error(const char* message, const char* suggestion) {
    if (suggestion) {
        SYNTAX_ERROR_WITH_HINT(current_token_idx, message, suggestion);
    } else {
        SYNTAX_ERROR(current_token_idx, message);
    }
}

/**
 * @brief Attempt error recovery by synchronizing to known tokens
 */
static int recover_from_error(void) {
    return error_recovery_sync(&error_recovery);
}

/**
 * @brief Parse primary expressions using modular AST builder
 */
static ASTNodeIdx_t parse_primary(void) {
    switch (current_token.id) {
        case T_LITINT: {
            // Extract integer value from token
            // This would need proper token-to-value conversion
            ASTNodeIdx_t node = ast_build_integer_literal(&g_parser_builder, 
                                                         current_token_idx, 0);
            next_token();
            return node;
        }
        
        case T_LITFLOAT: {
            ASTNodeIdx_t node = ast_create_node(&g_parser_builder, AST_LIT_FLOAT, 
                                               current_token_idx);
            next_token();
            return node;
        }
        
        case T_LITCHAR: {
            ASTNodeIdx_t node = ast_create_node(&g_parser_builder, AST_LIT_CHAR, 
                                               current_token_idx);
            next_token();
            return node;
        }
            
        case T_ID: {
            // Symbol lookup would happen in semantic analysis phase
            ASTNodeIdx_t node = ast_build_identifier(&g_parser_builder, 
                                                    current_token_idx, 0);
            next_token();
            return node;
        }
            
        case T_LPAREN: {
            next_token();  // consume '('
            ASTNodeIdx_t expr = parse_expression();
            if (expr == 0) return 0;
            
            if (!match_token(T_RPAREN)) {
                report_syntax_error("Expected ')' after expression", 
                                  "Add closing parenthesis");
                return 0;
            }
            return expr;
        }
            
        default:
            report_syntax_error("Unexpected token in expression", 
                              "Expected literal, identifier, or '('");
            return 0;
    }
}

/**
 * @brief Parse unary expressions using modular AST builder
 */
static ASTNodeIdx_t parse_unary(void) {
    if (current_token.id == T_MINUS || current_token.id == T_PLUS ||
        current_token.id == T_NOT || current_token.id == T_BITNOT) {
        
        TokenIdx_t op_token = current_token_idx;
        next_token();
        
        ASTNodeIdx_t operand = parse_unary();
        if (operand == 0) return 0;
        
        return ast_build_unary_expr(&g_parser_builder, op_token, operand);
    }
    
    return parse_primary();
}

/**
 * @brief Parse binary expressions with precedence
 */
static ASTNodeIdx_t parse_binary_expr(ASTNodeIdx_t (*parse_higher)(void),
                                     TokenID_t* operators, int op_count) {
    ASTNodeIdx_t left = parse_higher();
    if (left == 0) return 0;
    
    while (1) {
        int found = 0;
        for (int i = 0; i < op_count; i++) {
            if (current_token.id == operators[i]) {
                found = 1;
                break;
            }
        }
        if (!found) break;
        
        TokenIdx_t op_token = current_token_idx;
        next_token();
        
        ASTNodeIdx_t right = parse_higher();
        if (right == 0) return 0;
        
        left = ast_build_binary_expr(&g_parser_builder, op_token, left, right);
        if (left == 0) return 0;
    }
    
    return left;
}

/**
 * @brief Parse multiplicative expressions (* / %)
 */
static ASTNodeIdx_t parse_multiplicative(void) {
    ASTNodeIdx_t left = parse_unary();
    if (left == 0) return 0;
    
    while (current_token.id == T_MULT || current_token.id == T_DIV || current_token.id == T_MOD) {
        TokenIdx_t op_token = current_token_idx;
        next_token();
        
        ASTNodeIdx_t right = parse_unary();
        if (right == 0) return 0;
        
        ASTNodeIdx_t node_idx = create_ast_node(AST_OPERATOR, op_token);
        HBNode *node = HBGet(node_idx, HBMODE_AST);
        node->ast.o1 = left;
        node->ast.o2 = right;
        left = node_idx;
    }
    
    return left;
}

/**
 * @brief Parse additive expressions (+ -)
 */
static ASTNodeIdx_t parse_additive(void) {
    ASTNodeIdx_t left = parse_multiplicative();
    if (left == 0) return 0;
    
    while (current_token.id == T_PLUS || current_token.id == T_MINUS) {
        TokenIdx_t op_token = current_token_idx;
        next_token();
        
        ASTNodeIdx_t right = parse_multiplicative();
        if (right == 0) return 0;
        
        ASTNodeIdx_t node_idx = create_ast_node(AST_OPERATOR, op_token);
        HBNode *node = HBGet(node_idx, HBMODE_AST);
        node->ast.o1 = left;
        node->ast.o2 = right;
        left = node_idx;
    }
    
    return left;
}

/**
 * @brief Parse comparison expressions (< <= > >= == !=)
 */
static ASTNodeIdx_t parse_comparison(void) {
    ASTNodeIdx_t left = parse_additive();
    if (left == 0) return 0;
    
    while (current_token.id == T_LT || current_token.id == T_LE || 
           current_token.id == T_GT || current_token.id == T_GE ||
           current_token.id == T_EQ || current_token.id == T_NE) {
        TokenIdx_t op_token = current_token_idx;
        next_token();
        
        ASTNodeIdx_t right = parse_additive();
        if (right == 0) return 0;
        
        ASTNodeIdx_t node_idx = create_ast_node(AST_OPERATOR, op_token);
        HBNode *node = HBGet(node_idx, HBMODE_AST);
        node->ast.o1 = left;
        node->ast.o2 = right;
        left = node_idx;
    }
    
    return left;
}

/**
 * @brief Parse logical AND expressions (&&)
 */
static ASTNodeIdx_t parse_logical_and(void) {
    ASTNodeIdx_t left = parse_comparison();
    if (left == 0) return 0;
    
    while (current_token.id == T_AND) {
        TokenIdx_t op_token = current_token_idx;
        next_token();
        
        ASTNodeIdx_t right = parse_comparison();
        if (right == 0) return 0;
        
        ASTNodeIdx_t node_idx = create_ast_node(AST_OPERATOR, op_token);
        HBNode *node = HBGet(node_idx, HBMODE_AST);
        node->ast.o1 = left;
        node->ast.o2 = right;
        left = node_idx;
    }
    
    return left;
}

/**
 * @brief Parse logical OR expressions (||)
 */
static ASTNodeIdx_t parse_logical_or(void) {
    ASTNodeIdx_t left = parse_logical_and();
    if (left == 0) return 0;
    
    while (current_token.id == T_OR) {
        TokenIdx_t op_token = current_token_idx;
        next_token();
        
        ASTNodeIdx_t right = parse_logical_and();
        if (right == 0) return 0;
        
        ASTNodeIdx_t node_idx = create_ast_node(AST_OPERATOR, op_token);
        HBNode *node = HBGet(node_idx, HBMODE_AST);
        node->ast.o1 = left;
        node->ast.o2 = right;
        left = node_idx;
    }
    
    return left;
}

/**
 * @brief Parse assignment expressions (=)
 */
static ASTNodeIdx_t parse_assignment(void) {
    ASTNodeIdx_t left = parse_logical_or();
    if (left == 0) return 0;
    
    if (current_token.id == T_ASSIGN) {
        TokenIdx_t op_token = current_token_idx;
        next_token();
        
        ASTNodeIdx_t right = parse_assignment(); // Right-associative
        if (right == 0) return 0;
        
        ASTNodeIdx_t node_idx = create_ast_node(AST_ASSIGNMENT, op_token);
        HBNode *node = HBGet(node_idx, HBMODE_AST);
        node->ast.o1 = left;
        node->ast.o2 = right;
        return node_idx;
    }
    
    return left;
}

/**
 * @brief Parse expressions (top level)
 */
static ASTNodeIdx_t parse_expression(void) {
    return parse_assignment();
}

/**
 * @brief Parse expression statements
 */
static ASTNodeIdx_t parse_expression_statement(void) {
    if (current_token.id == T_SEMICOLON) {
        next_token(); // Empty statement
        return create_ast_node(AST_STATEMENT, current_token_idx);
    }
    
    ASTNodeIdx_t expr = parse_expression();
    if (expr == 0) return 0;
    
    if (!match_token(T_SEMICOLON)) {
        fprintf(stderr, "Error: Expected ';' after expression\n");
        return 0;
    }
    
    ASTNodeIdx_t stmt = create_ast_node(AST_STATEMENT, current_token_idx);
    HBNode *node = HBGet(stmt, HBMODE_AST);
    node->ast.o1 = expr;
    return stmt;
}

/**
 * @brief Parse if statements
 */
static ASTNodeIdx_t parse_if_statement(void) {
    TokenIdx_t if_token = current_token_idx;
    next_token(); // consume 'if'
    
    if (!match_token(T_LPAREN)) {
        fprintf(stderr, "Error: Expected '(' after 'if'\n");
        return 0;
    }
    
    ASTNodeIdx_t condition = parse_expression();
    if (condition == 0) return 0;
    
    if (!match_token(T_RPAREN)) {
        fprintf(stderr, "Error: Expected ')' after if condition\n");
        return 0;
    }
    
    ASTNodeIdx_t then_stmt = parse_statement();
    if (then_stmt == 0) return 0;
    
    ASTNodeIdx_t if_node = create_ast_node(AST_IF, if_token);
    HBNode *node = HBGet(if_node, HBMODE_AST);
    node->ast.o1 = condition;
    node->ast.o2 = then_stmt;
    
    // Handle optional else clause
    if (current_token.id == T_ELSE) {
        next_token();
        ASTNodeIdx_t else_stmt = parse_statement();
        if (else_stmt == 0) return 0;
        
        // Create a compound node for then/else
        ASTNodeIdx_t compound = create_ast_node(AST_STATEMENT, current_token_idx);
        HBNode *compound_node = HBGet(compound, HBMODE_AST);
        compound_node->ast.o1 = then_stmt;
        compound_node->ast.o2 = else_stmt;
        node->ast.o2 = compound;
    }
    
    return if_node;
}

/**
 * @brief Parse while statements
 */
static ASTNodeIdx_t parse_while_statement(void) {
    TokenIdx_t while_token = current_token_idx;
    next_token(); // consume 'while'
    
    if (!match_token(T_LPAREN)) {
        fprintf(stderr, "Error: Expected '(' after 'while'\n");
        return 0;
    }
    
    ASTNodeIdx_t condition = parse_expression();
    if (condition == 0) return 0;
    
    if (!match_token(T_RPAREN)) {
        fprintf(stderr, "Error: Expected ')' after while condition\n");
        return 0;
    }
    
    ASTNodeIdx_t body = parse_statement();
    if (body == 0) return 0;
    
    ASTNodeIdx_t while_node = create_ast_node(AST_WHILE, while_token);
    HBNode *node = HBGet(while_node, HBMODE_AST);
    node->ast.o1 = condition;
    node->ast.o2 = body;
    
    return while_node;
}

/**
 * @brief Parse return statements
 */
static ASTNodeIdx_t parse_return_statement(void) {
    TokenIdx_t return_token = current_token_idx;
    next_token(); // consume 'return'
    
    ASTNodeIdx_t return_node = create_ast_node(AST_STATEMENT, return_token);
    HBNode *node = HBGet(return_node, HBMODE_AST);
    
    if (current_token.id != T_SEMICOLON) {
        ASTNodeIdx_t expr = parse_expression();
        if (expr == 0) return 0;
        node->ast.o1 = expr;
    }
    
    if (!match_token(T_SEMICOLON)) {
        fprintf(stderr, "Error: Expected ';' after return statement\n");
        return 0;
    }
    
    return return_node;
}

/**
 * @brief Parse compound statements (blocks)
 */
static ASTNodeIdx_t parse_compound_statement(void) {
    if (!match_token(T_LBRACE)) {
        fprintf(stderr, "Error: Expected '{' to start compound statement\n");
        return 0;
    }
    
    ASTNodeIdx_t compound = create_ast_node(AST_STATEMENT, current_token_idx);
    ASTNodeIdx_t last_stmt = 0;
    ASTNodeIdx_t first_stmt = 0;
    
    while (current_token.id != T_RBRACE && current_token.id != T_EOF) {
        ASTNodeIdx_t stmt = parse_statement();
        if (stmt == 0) return 0;
        
        if (first_stmt == 0) {
            first_stmt = stmt;
            last_stmt = stmt;
        } else {
            // Chain statements together
            HBNode *last_node = HBGet(last_stmt, HBMODE_AST);
            last_node->ast.o2 = stmt;
            last_stmt = stmt;
        }
    }
    
    if (!match_token(T_RBRACE)) {
        fprintf(stderr, "Error: Expected '}' to end compound statement\n");
        return 0;
    }
    
    HBNode *node = HBGet(compound, HBMODE_AST);
    node->ast.o1 = first_stmt;
    
    return compound;
}

/**
 * @brief Parse statements
 */
static ASTNodeIdx_t parse_statement(void) {
    switch (current_token.id) {
        case T_IF:
            return parse_if_statement();
            
        case T_WHILE:
            return parse_while_statement();
            
        case T_RETURN:
            return parse_return_statement();
            
        case T_LBRACE:
            return parse_compound_statement();
            
        default:
            return parse_expression_statement();
    }
}

// Export the enhanced parser functions
ASTNodeIdx_t enhanced_parse_expression(void) {
    return parse_expression();
}

ASTNodeIdx_t enhanced_parse_statement(void) {
    return parse_statement();
}

void enhanced_parser_init(void) {
    next_token(); // Initialize current token
}
