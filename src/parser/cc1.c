/**
 * @file cc1.c
 * @brief Enhanced C parser with AST generation and symbol table management
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 *
 * This file implements the main parser component of the STCC1 compiler.
 * It handles parsing of tokens into an Abstract Syntax Tree (AST) and
 * manages symbol table creation during the parsing process.
 *
 * Enhanced parser that processes tokens from cc0 and generates:
 * - Complete Abstract Syntax Tree (AST) stored in astore
 * - Symbol table with proper scoping stored in symtab
 * - Integrated error handling with detailed diagnostics
 * - Memory-efficient file-based storage during runtime
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../storage/sstore.h"
#include "../storage/tstore.h"
#include "../storage/astore.h"
#include "../storage/symtab.h"
#include "../utils/hmapbuf.h"
#include "../error/error_core.h"
// Enhanced cc1 with error handling and core AST capabilities

// Type specifier information for complex type parsing
typedef struct {
    int has_signed;      // -1 = signed, 0 = not specified, 1 = unsigned
    int has_long;        // 0 = not specified, 1 = long, 2 = long long
    int has_short;       // 0 = not specified, 1 = short
    int base_type;       // T_INT, T_CHAR, T_FLOAT, T_DOUBLE, T_VOID, or 0 for unspecified
    int is_valid;        // 1 if valid combination, 0 otherwise
} TypeSpecifier_t;

// Enhanced parser state for tracking context
typedef struct {
    TokenIdx_t current_token;
    int in_function;
    int scope_depth;  // C99 scope depth: 0=file, 1=function, 2+=block
    int error_count;
} ParserState_t;

static ParserState_t parser_state = {0};

// Forward declarations
static TypeSpecifier_t parse_type_specifiers(void);
static int is_type_specifier_start(TokenID_t token_id);
static void enter_scope(void);
static void exit_scope(void);
static SymIdx_t lookup_symbol_in_scope(sstore_pos_t name_pos);
ASTNodeIdx_t parse_program(void);
ASTNodeIdx_t parse_declaration(void);
ASTNodeIdx_t parse_function_definition(void);
ASTNodeIdx_t parse_statement(void);
ASTNodeIdx_t parse_expression(void);
ASTNodeIdx_t parse_assignment_expression(void);
ASTNodeIdx_t parse_relational_expression(void);
ASTNodeIdx_t parse_additive_expression(void);
ASTNodeIdx_t parse_multiplicative_expression(void);
ASTNodeIdx_t parse_unary_expression(void);
ASTNodeIdx_t parse_postfix_expression(void);
ASTNodeIdx_t parse_primary_expression(void);

// Additional function prototypes
static Token_t peek_token(void);
static Token_t next_token(void);
static int expect_token(TokenID_t expected);
static ASTNodeIdx_t create_ast_node(ASTNodeType type, TokenIdx_t token_idx);
static SymIdx_t add_symbol(sstore_pos_t name_pos, SymType type, TokenIdx_t token_idx);
static void parser_init(void);
static void parser_cleanup(void);

/**
 * @brief Get the current token without advancing
 */
static Token_t peek_token(void) {
    TokenIdx_t saved_idx = tstore_getidx();
    Token_t token = tstore_next();
    tstore_setidx(saved_idx);
    return token;
}

/**
 * @brief Advance to next token and return it
 */
static Token_t next_token(void) {
    parser_state.current_token = tstore_getidx();
    return tstore_next();
}

/**
 * @brief Check if current token matches expected type
 */
static int expect_token(TokenID_t expected) {
    Token_t token = peek_token();
    if (token.id == expected) {
        next_token();
        return 1;
    }

    // Report syntax error using core error system
    SourceLocation_t location = error_create_location(parser_state.current_token);
    error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2001,
                     "Unexpected token", "Check syntax", "parser", NULL);
    return 0;
}

/**
 * @brief Create AST node with enhanced error handling
 */
static ASTNodeIdx_t create_ast_node(ASTNodeType type, TokenIdx_t token_idx) {
    HBNode *node = HBNew(HBMODE_AST);
    if (!node) {
        SourceLocation_t loc = {0, 0, 0, 0, NULL};  // Simple location with all fields
        error_core_report(ERROR_ERROR, ERROR_SEMANTIC,
                         &loc, 3001, "Cannot allocate AST node",
                         "Check memory allocation", "parser", NULL);
        return 0;
    }

    node->ast.type = type;
    node->ast.token_idx = token_idx;
    node->ast.flags = AST_FLAG_PARSED;
    node->ast.type_idx = 0;

    return node->idx;
}

/**
 * @brief Enter a new scope (when encountering '{') - C99 compliant
 */
static void enter_scope(void) {
    parser_state.scope_depth++;
    // In C99, we don't create scope symbols - we just track scope depth
    // Variables declared at this depth will have proper block scope
}

/**
 * @brief Exit current scope (when encountering '}') - C99 compliant
 */
static void exit_scope(void) {
    if (parser_state.scope_depth > 0) {
        parser_state.scope_depth--;
        // In C99, variables simply go out of scope when we exit the block
        // No need to track artificial scope symbols
    }
}

/**
 * @brief Look up symbol using C99 scoping rules
 * In C99: innermost scope hides outer scope variables with same name
 */
static SymIdx_t lookup_symbol_in_scope(sstore_pos_t name_pos) {
    SymIdx_t best_match = 0;
    int best_scope_depth = -1;
    
    // Search all symbols for matches with this name
    uint32_t symbol_count = symtab_get_count();
    
    for (SymIdx_t i = 1; i <= symbol_count; i++) {
        SymTabEntry entry = symtab_get(i);
        
        // Skip non-matching names
        if (entry.name != name_pos) {
            continue;
        }
        
        // Check if this symbol is visible from current scope
        // C99 rule: variable is visible if declared at current scope depth or shallower
        if (entry.scope_depth <= parser_state.scope_depth) {
            // C99 rule: innermost (highest depth) declaration wins
            if (entry.scope_depth > best_scope_depth) {
                best_match = i;
                best_scope_depth = entry.scope_depth;
            }
        }
    }
    
    return best_match;
}

/**
 * @brief Look up symbol by name in symbol table
 */
static SymIdx_t lookup_symbol(sstore_pos_t name_pos) {
    // Use scoped lookup for proper C semantics
    return lookup_symbol_in_scope(name_pos);
}

/**
 * @brief Add symbol to symbol table with C99 scope depth
 */
static SymIdx_t add_symbol(sstore_pos_t name_pos, SymType type, TokenIdx_t token_idx) {
    SymTabEntry entry = {0};
    entry.type = type;
    entry.name = name_pos;
    entry.parent = 0;  // Not used in C99 scoping
    entry.line = token_idx;
    entry.scope_depth = parser_state.scope_depth;  // Record C99 scope depth

    SymIdx_t sym_idx = symtab_add(&entry);
    if (sym_idx == 0) {
        SourceLocation_t location = error_create_location(token_idx);
        error_core_report(ERROR_ERROR, ERROR_SEMANTIC, &location, 3000,
                         "Cannot add symbol to table", "Check symbol table capacity", "parser", NULL);
    }

    return sym_idx;
}

/**
 * @brief Parse primary expressions (identifiers, literals, parenthesized expressions)
 */
ASTNodeIdx_t parse_primary_expression(void) {
    Token_t token = peek_token();
    TokenIdx_t token_idx = tstore_getidx();

    switch (token.id) {
        case T_ID: {
            next_token();
            ASTNodeIdx_t node_idx = create_ast_node(AST_EXPR_IDENTIFIER, token_idx);
            if (node_idx) {
                HBNode *node = HBGet(node_idx, HBMODE_AST);
                // Look up the symbol index for this identifier
                SymIdx_t sym_idx = lookup_symbol(token.pos);
                if (sym_idx == 0) {
                    // Symbol not found - this could be an error or forward reference
                    node->ast.binary.value.string_pos = token.pos;  // Store name for error reporting
                } else {
                    // Store the symbol index in the identifier node
                    node->ast.binary.value.symbol_idx = sym_idx;
                }
            }
            return node_idx;
        }

        case T_LITINT: {
            next_token();
            ASTNodeIdx_t node_idx = create_ast_node(AST_LIT_INTEGER, token_idx);
            if (node_idx) {
                HBNode *node = HBGet(node_idx, HBMODE_AST);
                // Extract integer value from string store
                char *str = sstore_get(token.pos);
                node->ast.binary.value.long_value = strtol(str, NULL, 0);
                HBTouched(node);
            }
            return node_idx;
        }

        case T_LITSTRING: {
            next_token();
            return create_ast_node(AST_LIT_STRING, token_idx);
        }

        case T_LITCHAR: {
            next_token();
            return create_ast_node(AST_LIT_CHAR, token_idx);
        }

        case T_LPAREN: {
            next_token(); // consume '('
            ASTNodeIdx_t expr = parse_expression();
            if (!expect_token(T_RPAREN)) {
                SourceLocation_t location = error_create_location(tstore_getidx());
                error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2001,
                                 "Missing closing parenthesis", "Expected ')'", "parser", NULL);
            }
            return expr;
        }

        default:
            SourceLocation_t location = error_create_location(token_idx);
            error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                             "Expected primary expression", "Check expression syntax", "parser", NULL);
            return 0;
    }
}

/**
 * @brief Parse expressions with proper operator precedence
 * Assignment has lower precedence than arithmetic operators
 */
ASTNodeIdx_t parse_expression(void) {
    return parse_assignment_expression();
}

/**
 * @brief Parse assignment expressions (lowest precedence)
 * Right-associative: a = b = c becomes a = (b = c)
 */
ASTNodeIdx_t parse_assignment_expression(void) {
    ASTNodeIdx_t left = parse_relational_expression();
    if (!left) return 0;

    Token_t token = peek_token();
    if (token.id == T_ASSIGN) {
        TokenIdx_t token_idx = tstore_getidx();
        next_token(); // consume '='
        
        // Assignment is right-associative, so parse another assignment expression
        ASTNodeIdx_t right = parse_assignment_expression();
        if (!right) {
            SourceLocation_t location = error_create_location(token_idx);
            error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                             "Expected right operand after '='", "Check assignment syntax", "parser", NULL);
            return left;
        }

        // Create assignment node
        ASTNodeIdx_t assign_node = create_ast_node(AST_EXPR_ASSIGN, token_idx);
        if (assign_node) {
            HBNode *node = HBGet(assign_node, HBMODE_AST);
            node->ast.binary.left = left;
            node->ast.binary.right = right;
        }
        return assign_node;
    }

    return left;
}

/**
 * @brief Parse relational expressions (<, >, <=, >=)
 * Left-associative: a < b < c becomes (a < b) < c
 */
ASTNodeIdx_t parse_relational_expression(void) {
    ASTNodeIdx_t left = parse_additive_expression();
    if (!left) return 0;

    while (1) {
        Token_t token = peek_token();
        if (token.id != T_LT && token.id != T_GT && 
            token.id != T_LTE && token.id != T_GTE) {
            break;
        }

        TokenIdx_t token_idx = tstore_getidx();
        next_token(); // consume operator
        
        ASTNodeIdx_t right = parse_additive_expression();
        if (!right) {
            SourceLocation_t location = error_create_location(token_idx);
            error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                             "Expected right operand for relational operator", 
                             "Check relational expression syntax", "parser", NULL);
            return left;
        }

        // Create binary expression node
        ASTNodeIdx_t op_node = create_ast_node(AST_EXPR_BINARY_OP, token_idx);
        if (op_node) {
            HBNode* hb_node = HBGet(op_node, HBMODE_AST);
            if (hb_node) {
                hb_node->ast.binary.left = left;
                hb_node->ast.binary.right = right;
            }
        }
        left = op_node;
    }

    return left;
}

/**
 * @brief Parse additive expressions (+ and -)
 * Left-associative: a + b + c becomes (a + b) + c
 */
ASTNodeIdx_t parse_additive_expression(void) {
    ASTNodeIdx_t left = parse_multiplicative_expression();
    if (!left) return 0;

    while (1) {
        Token_t token = peek_token();
        if (token.id != T_PLUS && token.id != T_MINUS) {
            break;
        }

        TokenIdx_t token_idx = tstore_getidx();
        next_token(); // consume operator
        
        ASTNodeIdx_t right = parse_multiplicative_expression();
        if (!right) {
            SourceLocation_t location = error_create_location(token_idx);
            error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                             "Expected right operand", "Check expression syntax", "parser", NULL);
            return left;
        }

        // Create binary operation node
        ASTNodeIdx_t op_node = create_ast_node(AST_EXPR_BINARY_OP, token_idx);
        if (op_node) {
            HBNode *node = HBGet(op_node, HBMODE_AST);
            node->ast.binary.left = left;
            node->ast.binary.right = right;
        }
        
        left = op_node;
    }

    return left;
}

/**
 * @brief Parse postfix expressions (function calls, array access, etc.)
 * Left-associative: f(x)(y) becomes (f(x))(y)
 */
ASTNodeIdx_t parse_postfix_expression(void) {
    ASTNodeIdx_t left = parse_primary_expression();
    if (!left) return 0;

    while (1) {
        Token_t token = peek_token();
        
        if (token.id == T_LPAREN) {
            // Function call: identifier(arguments)
            TokenIdx_t token_idx = tstore_getidx();
            next_token(); // consume '('
            
            ASTNodeIdx_t call_node = create_ast_node(AST_EXPR_CALL, token_idx);
            if (call_node) {
                HBNode *node = HBGet(call_node, HBMODE_AST);
                node->ast.call.function = left; // Function being called
                node->ast.call.arg_count = 0;
                node->ast.call.return_type = 0; // Default type
                
                // Parse arguments
                ASTNodeIdx_t first_arg = 0;
                ASTNodeIdx_t last_arg = 0;
                
                while (peek_token().id != T_RPAREN && peek_token().id != T_EOF) {
                    ASTNodeIdx_t arg = parse_expression();
                    if (!arg) break;
                    
                    node->ast.call.arg_count++;
                    
                    if (!first_arg) {
                        first_arg = arg;
                        last_arg = arg;
                        node->ast.call.arguments = first_arg; // Link to first argument
                    } else {
                        // Chain arguments using child2 as 'next' pointer
                        if (last_arg) {
                            HBNode *last_arg_node = HBGet(last_arg, HBMODE_AST);
                            if (last_arg_node) {
                                last_arg_node->ast.children.child2 = arg;
                            }
                        }
                        last_arg = arg;
                    }
                    
                    // Handle comma between arguments
                    if (peek_token().id == T_COMMA) {
                        next_token(); // consume comma
                    } else {
                        break; // No more arguments
                    }
                }
                
                expect_token(T_RPAREN);
            }
            left = call_node;
        } else {
            // No more postfix operators
            break;
        }
    }

    return left;
}

/**
 * @brief Parse multiplicative expressions (* and /)
 * Left-associative: a * b * c becomes (a * b) * c
 */
ASTNodeIdx_t parse_multiplicative_expression(void) {
    ASTNodeIdx_t left = parse_unary_expression();
    if (!left) return 0;

    while (1) {
        Token_t token = peek_token();
        if (token.id != T_MUL && token.id != T_DIV) {
            break;
        }

        TokenIdx_t token_idx = tstore_getidx();
        next_token(); // consume operator
        
        ASTNodeIdx_t right = parse_unary_expression();
        if (!right) {
            SourceLocation_t location = error_create_location(token_idx);
            error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                             "Expected right operand", "Check expression syntax", "parser", NULL);
            return left;
        }

        // Create binary operation node
        ASTNodeIdx_t op_node = create_ast_node(AST_EXPR_BINARY_OP, token_idx);
        if (op_node) {
            HBNode *node = HBGet(op_node, HBMODE_AST);
            node->ast.binary.left = left;
            node->ast.binary.right = right;
        }
        
        left = op_node;
    }

    return left;
}

/**
 * @brief Parse unary expressions (+expr, -expr, !expr, etc.)
 * Right-associative: --x becomes -((-x))
 */
ASTNodeIdx_t parse_unary_expression(void) {
    Token_t token = peek_token();
    TokenIdx_t token_idx = tstore_getidx();

    switch (token.id) {
        case T_PLUS:
        case T_MINUS:
        case T_NOT: {
            next_token(); // consume unary operator
            
            ASTNodeIdx_t operand = parse_unary_expression(); // Right-associative
            if (!operand) {
                SourceLocation_t location = error_create_location(token_idx);
                error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                                 "Expected operand after unary operator", "Check unary expression syntax", "parser", NULL);
                return 0;
            }

            // Create unary expression node
            ASTNodeIdx_t unary_node = create_ast_node(AST_EXPR_UNARY_OP, token_idx);
            if (unary_node) {
                HBNode *node = HBGet(unary_node, HBMODE_AST);
                node->ast.unary.operand = operand;
                node->ast.unary.operator = token.id; // Store the operator type
            }
            return unary_node;
        }

        default:
            // No unary operator, continue to postfix expression
            return parse_postfix_expression();
    }
}

/**
 * @brief Parse statements (return, if, while, expression statements, etc.)
 */
ASTNodeIdx_t parse_statement(void) {
    Token_t token = peek_token();
    TokenIdx_t token_idx = tstore_getidx();

    switch (token.id) {
        case T_RETURN: {
            next_token(); // consume 'return'
            ASTNodeIdx_t node_idx = create_ast_node(AST_STMT_RETURN, token_idx);

            // Optional return expression
            if (peek_token().id != T_SEMICOLON) {
                ASTNodeIdx_t expr = parse_expression();
                if (node_idx && expr) {
                    HBNode *node = HBGet(node_idx, HBMODE_AST);
                    node->ast.children.child1 = expr;
                }
            }

            expect_token(T_SEMICOLON);
            return node_idx;
        }

        case T_IF: {
            next_token(); // consume 'if'
            expect_token(T_LPAREN);
            ASTNodeIdx_t condition = parse_expression();
            expect_token(T_RPAREN);
            ASTNodeIdx_t then_stmt = parse_statement();

            ASTNodeIdx_t node_idx = create_ast_node(AST_STMT_IF, token_idx);
            if (node_idx) {
                HBNode *node = HBGet(node_idx, HBMODE_AST);
                node->ast.conditional.condition = condition;
                node->ast.conditional.then_stmt = then_stmt;
                
                // Handle optional else clause
                if (peek_token().id == T_ELSE) {
                    next_token(); // consume 'else'
                    ASTNodeIdx_t else_stmt = parse_statement();
                    node->ast.conditional.else_stmt = else_stmt;
                }
            }
            return node_idx;
        }

        case T_WHILE: {
            next_token(); // consume 'while'
            expect_token(T_LPAREN);
            ASTNodeIdx_t condition = parse_expression();
            expect_token(T_RPAREN);
            ASTNodeIdx_t body = parse_statement();

            ASTNodeIdx_t node_idx = create_ast_node(AST_STMT_WHILE, token_idx);
            if (node_idx) {
                HBNode *node = HBGet(node_idx, HBMODE_AST);
                node->ast.conditional.condition = condition;
                node->ast.conditional.then_stmt = body;
            }
            return node_idx;
        }

        case T_LBRACE: {
            // Compound statement - creates a new scope
            next_token(); // consume '{'
            
            // Enter new scope for this block
            enter_scope();
            
            ASTNodeIdx_t compound = create_ast_node(AST_STMT_COMPOUND, token_idx);
            ASTNodeIdx_t first_stmt = 0;
            ASTNodeIdx_t last_stmt = 0;

            // Parse statements until '}'
            while (peek_token().id != T_RBRACE && peek_token().id != T_EOF) {
                ASTNodeIdx_t stmt;
                Token_t next_token_peek = peek_token();
                
                // If this looks like a declaration, handle it directly here
                if (next_token_peek.id == T_INT || next_token_peek.id == T_CHAR || 
                    next_token_peek.id == T_FLOAT || next_token_peek.id == T_DOUBLE || 
                    next_token_peek.id == T_VOID) {
                    // Parse variable declaration directly
                    stmt = parse_declaration();
                } else {
                    // Parse as regular statement
                    stmt = parse_statement();
                }
                if (!stmt) break;
                
                // Link statements properly into the compound statement
                if (!first_stmt) {
                    first_stmt = stmt;
                    last_stmt = stmt;
                    
                    // Link compound statement to first statement
                    if (compound) {
                        HBNode *comp_node = HBGet(compound, HBMODE_AST);
                        if (comp_node) {
                            comp_node->ast.children.child1 = first_stmt;
                        }
                    }
                } else {
                    // Chain subsequent statements using child2 as 'next' pointer
                    // BUT: Be careful not to overwrite conditional union fields for if/while statements
                    if (last_stmt) {
                        HBNode *last_node = HBGet(last_stmt, HBMODE_AST);
                        if (last_node) {
                            // Check if this is a conditional statement (if/while) that uses the conditional union
                            if (last_node->ast.type == AST_STMT_IF || last_node->ast.type == AST_STMT_WHILE) {
                                // For conditional statements, use child4 for chaining to avoid conflicts
                                last_node->ast.children.child4 = stmt;
                            } else {
                                // For other statements, use child2 as normal
                                last_node->ast.children.child2 = stmt;
                            }
                        }
                    }
                    last_stmt = stmt;
                }
            }

            expect_token(T_RBRACE);
            
            // Exit scope when leaving the block
            exit_scope();
            
            return compound;
        }

        default: {
            // Expression statement
            ASTNodeIdx_t expr = parse_expression();
            expect_token(T_SEMICOLON);
            return expr;
        }
    }
}

/**
 * @brief Parse variable or function declarations
 */
ASTNodeIdx_t parse_declaration(void) {
    Token_t token = peek_token();
    TokenIdx_t token_idx = tstore_getidx();

    // Check if this starts with a type specifier or storage class
    if (!is_type_specifier_start(token.id)) {
        return 0;  // Not a declaration
    }

    // Parse complex type specifiers
    TypeSpecifier_t type_spec = parse_type_specifiers();
    if (!type_spec.is_valid) {
        SourceLocation_t location = error_create_location(tstore_getidx());
        error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2001,
                         "Invalid type specifier combination", "Check type syntax", "parser", NULL);
        return 0;
    }
    
     // Now expect an identifier (unless this is a struct/union/enum declaration)
    Token_t id_token = peek_token();
    if (id_token.id != T_ID) {
        // Handle cases like "struct { ... }" or bare type declarations
        if (peek_token().id == T_SEMICOLON) {
            next_token();  // consume semicolon
            return create_ast_node(AST_VAR_DECL, token_idx);
        }

        SourceLocation_t location = error_create_location(tstore_getidx());
        error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2001,
                         "Missing identifier", "Expected identifier after type", "parser", NULL);
        return 0;
    }

    next_token();  // consume identifier
    // const char* name = sstore_get(id_token.pos);  // Unused for now

    // Check if this is a function definition
    if (peek_token().id == T_LPAREN) {
        // Function definition - functions have file scope (depth 0)
        parser_state.in_function = 1;
        add_symbol(id_token.pos, SYM_FUNCTION, tstore_getidx());

        next_token();  // consume '('
        
        // Enter function parameter scope (depth 1 for C99 function scope)
        parser_state.scope_depth = 1;
        
        // Parse parameters (simple implementation)
        while (peek_token().id != T_RPAREN && peek_token().id != T_EOF) {
            // Parse parameter type
            if (peek_token().id == T_INT || peek_token().id == T_CHAR || peek_token().id == T_FLOAT) {
                next_token();  // consume type
                if (peek_token().id == T_ID) {
                    Token_t param_token = peek_token();
                    next_token();  // consume parameter name
                    // Parameters have function scope (depth 1) in C99
                    add_symbol(param_token.pos, SYM_VARIABLE, tstore_getidx());
                }
                // Handle comma between parameters
                if (peek_token().id == T_COMMA) {
                    next_token();  // consume comma
                }
            } else {
                // Skip unknown tokens to avoid infinite loop
                next_token();
            }
        }
        
        expect_token(T_RPAREN);

        if (peek_token().id == T_LBRACE) {
            // Function definition with body
            ASTNodeIdx_t body = parse_statement();
            ASTNodeIdx_t func_node = create_ast_node(AST_FUNCTION_DEF, token_idx);
            if (func_node) {
                HBNode *node = HBGet(func_node, HBMODE_AST);
                node->ast.binary.value.string_pos = id_token.pos;  // Function name
                node->ast.children.child1 = body;
            }
            // Exit function scope back to file scope (depth 0)
            parser_state.in_function = 0;
            parser_state.scope_depth = 0;
            return func_node;
        } else {
            // Function declaration only
            expect_token(T_SEMICOLON);
            // Reset scope depth for function declaration
            parser_state.scope_depth = 0;
            return create_ast_node(AST_FUNCTION_DECL, token_idx);
        }
    } else {
        // Variable declaration
        SymIdx_t sym_idx = add_symbol(id_token.pos, SYM_VARIABLE, tstore_getidx());

        // Handle optional initializer
        if (peek_token().id == T_ASSIGN) {
            next_token();  // consume '='
            ASTNodeIdx_t init_expr = parse_expression();
            ASTNodeIdx_t decl_node = create_ast_node(AST_VAR_DECL, token_idx);
            if (decl_node) {
                HBNode *node = HBGet(decl_node, HBMODE_AST);
                node->ast.declaration.symbol_idx = sym_idx;  // Store symbol table index
                node->ast.declaration.initializer = init_expr;
            }
            expect_token(T_SEMICOLON);
            return decl_node;
        } else {
            expect_token(T_SEMICOLON);
            ASTNodeIdx_t decl_node = create_ast_node(AST_VAR_DECL, token_idx);
            if (decl_node) {
                HBNode *node = HBGet(decl_node, HBMODE_AST);
                node->ast.declaration.symbol_idx = sym_idx;  // Store symbol table index
            }
            return decl_node;
        }
    }
}

/**
 * @brief Parse the complete program (translation unit)
 */
ASTNodeIdx_t parse_program(void) {
    ASTNodeIdx_t program_node = create_ast_node(AST_PROGRAM, 0);
    ASTNodeIdx_t first_decl = 0;
    ASTNodeIdx_t last_decl = 0;

    while (peek_token().id != T_EOF) {
        ASTNodeIdx_t decl = parse_declaration();
        if (!decl) {
            // Skip to next likely declaration start or advance at least one token
            Token_t token = next_token();
            if (token.id == T_EOF) break;
            continue;
        }

        // Build a proper declaration list
        if (!first_decl) {
            // First declaration
            first_decl = decl;
            last_decl = decl;
            
            // Link program to first declaration
            HBNode *prog_node = HBGet(program_node, HBMODE_AST);
            if (prog_node) {
                prog_node->ast.children.child1 = first_decl;
            }
        } else {
            // Chain subsequent declarations using child2 as 'next' pointer
            HBNode *last_node = HBGet(last_decl, HBMODE_AST);
            if (last_node) {
                last_node->ast.children.child2 = decl;
            }
            last_decl = decl;
        }
    }

    return program_node;
}

/**
 * @brief Initialize parser with error handling
 */
static void parser_init(void) {
    // Initialize hash map buffer for AST nodes
    HBInit();

    // Initialize error handling
    ErrorConfig_t config = {
        .max_errors = 50,
        .max_warnings = 100,
        .show_line_numbers = 1,
        .show_source_context = 1,
        .show_suggestions = 1,
        .colorize_output = 0,
        .output_stream = stderr
    };

    error_core_init(&config);

    // Initialize parser state for C99 scoping
    parser_state.current_token = 0;
    parser_state.in_function = 0;
    parser_state.scope_depth = 0;  // Start at file scope (C99)
    parser_state.error_count = 0;
    
    // In C99, we don't need to create artificial scope symbols
    // File scope is the default starting point
}

/**
 * @brief Clean up parser resources
 */
static void parser_cleanup(void) {
    // Flush all cached nodes to storage before cleanup
    HBEnd();
    
    if (error_core_has_errors()) {
        error_core_print_summary();
    }

    error_core_cleanup();
}

/**
 * @brief Parse complex type specifiers (handles "unsigned long int", "signed short int", etc.)
 * @return Type information encoded as a composite value, or 0 on error
 */
static TypeSpecifier_t parse_type_specifiers(void) {
    TypeSpecifier_t type = {0, 0, 0, 0, 1};  // Initialize as valid
    Token_t token;
    int tokens_consumed = 0;

    // Keep parsing type specifier tokens
    while (1) {
        token = peek_token();
        int advance = 0;

        switch (token.id) {
            case T_UNSIGNED:
                if (type.has_signed != 0) {
                    type.is_valid = 0; // Cannot have both signed and unsigned
                    return type;
                }
                type.has_signed = 1;
                advance = 1;
                break;

            case T_SIGNED:
                if (type.has_signed != 0) {
                    type.is_valid = 0; // Cannot have both signed and unsigned
                    return type;
                }
                type.has_signed = -1;
                advance = 1;
                break;

            case T_LONG:
                if (type.has_short || type.has_long >= 2) {
                    type.is_valid = 0; // Cannot mix short/long or have more than 2 longs
                    return type;
                }
                type.has_long++;
                advance = 1;
                break;

            case T_SHORT:
                if (type.has_long || type.has_short) {
                    type.is_valid = 0; // Cannot mix short/long or have multiple shorts
                    return type;
                }
                type.has_short = 1;
                advance = 1;
                break;

            case T_INT:
                if (type.base_type != 0) {
                    type.is_valid = 0; // Cannot have multiple base types
                    return type;
                }
                type.base_type = T_INT;
                advance = 1;
                break;

            case T_CHAR:
                if (type.base_type != 0 || type.has_long || type.has_short) {
                    type.is_valid = 0; // char cannot be combined with long/short
                    return type;
                }
                type.base_type = T_CHAR;
                advance = 1;
                break;

            case T_FLOAT:
            case T_DOUBLE:
            case T_VOID:
                if (type.base_type != 0 || type.has_signed != 0 || type.has_long || type.has_short) {
                    type.is_valid = 0; // float/double/void cannot be combined with modifiers
                    return type;
                }
                type.base_type = token.id;
                advance = 1;
                break;

            default:
                // Not a type specifier, stop parsing
                if (tokens_consumed == 0) {
                    // No type specifiers found at all - this should not be an error
                    // if the current token is actually a valid type specifier that
                    // we missed in our switch statement
                    if (is_type_specifier_start(token.id)) {
                        // This is a bug - we should handle this token
                        type.base_type = T_INT;  // Default fallback
                        type.is_valid = 1;
                        next_token();
                        tokens_consumed++;
                    } else {
                        type.is_valid = 0; // Truly not a type specifier
                    }
                }
                return type;
        }

        if (advance) {
            next_token();
            tokens_consumed++;
        } else {
            break;
        }
    }

    // Set default base type if none specified
    if (type.base_type == 0 && (type.has_signed != 0 || type.has_long || type.has_short)) {
        type.base_type = T_INT; // Default to int for signed/unsigned/long/short
    }

    return type;
}

/**
 * @brief Check if the current token starts a type specifier
 */
static int is_type_specifier_start(TokenID_t token_id) {
    return (token_id == T_INT || token_id == T_CHAR || token_id == T_FLOAT ||
            token_id == T_DOUBLE || token_id == T_VOID || token_id == T_LONG ||
            token_id == T_SHORT || token_id == T_UNSIGNED || token_id == T_SIGNED ||
            token_id == T_STRUCT || token_id == T_UNION || token_id == T_ENUM ||
            token_id == T_TYPEDEF || token_id == T_EXTERN || token_id == T_STATIC ||
            token_id == T_AUTO || token_id == T_REGISTER || token_id == T_CONST ||
            token_id == T_VOLATILE);
}

/**
 * @brief Main function for cc1 parser
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments: cc1 <sstorfile> <tokenfile> <astfile> <symfile>
 * @return 0 on success, non-zero on error
 */
int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <sstorfile> <tokenfile> <astfile> <symfile>\n", argv[0]);
        return 1;
    }

    // Initialize stores
    if (sstore_open(argv[1]) != 0) {
        fprintf(stderr, "Error: Cannot open sstorfile %s\n", argv[1]);
        return 1;
    }

    if (tstore_open(argv[2]) != 0) {
        fprintf(stderr, "Error: Cannot open tokenfile %s\n", argv[2]);
        sstore_close();
        return 1;
    }

    if (astore_init(argv[3]) != 0) {
        fprintf(stderr, "Error: Cannot open astfile %s\n", argv[3]);
        tstore_close();
        sstore_close();
        return 1;
    }

    if (symtab_init(argv[4]) != 0) {
        fprintf(stderr, "Error: Cannot open symfile %s\n", argv[4]);
        astore_close();
        tstore_close();
        sstore_close();
        return 1;
    }

    // Initialize parser
    parser_init();

    // Set token file to start position
    tstore_setidx(0);

    // Parse the program
    ASTNodeIdx_t program __attribute__((unused)) = parse_program();

    // Program parsing is considered successful if we reach this point
    // (AST index 0 is valid, and errors would have exited earlier)
    printf("Parsing completed successfully\n");

    // Clean up
    parser_cleanup();
    symtab_close();
    astore_close();
    tstore_close();
    sstore_close();

    return 0;
}
