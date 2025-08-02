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
#include "../error/error_stages.h"
// Enhanced cc1 with error handling and core AST capabilities

// Type specifier information for complex type parsing
typedef struct {
    int has_signed;      // -1 = signed, 0 = not specified, 1 = unsigned
    int has_long;        // 0 = not specified, 1 = long, 2 = long long
    int has_short;       // 0 = not specified, 1 = short
    int base_type;       // T_INT, T_CHAR, T_FLOAT, T_DOUBLE, T_VOID, or 0 for unspecified
    int is_valid;        // 1 if valid combination, 0 otherwise
    // C99 storage specifiers and qualifiers
    int has_inline;      // C99 inline function specifier
    int has_restrict;    // C99 restrict qualifier
    int has_const;       // const qualifier
    int has_volatile;    // volatile qualifier
    int has_static;      // static storage class
    int has_extern;      // extern storage class
    int has_auto;        // auto storage class
    int has_register;    // register storage class
    int has_typedef;     // typedef storage class
    int has_complex;     // C99 _Complex
    int has_imaginary;   // C99 _Imaginary
    int has_bool;        // C99 _Bool
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
ASTNodeIdx_t parse_conditional_expression(void);
ASTNodeIdx_t parse_relational_expression(void);
ASTNodeIdx_t parse_additive_expression(void);
ASTNodeIdx_t parse_multiplicative_expression(void);
ASTNodeIdx_t parse_unary_expression(void);
ASTNodeIdx_t parse_postfix_expression(void);
ASTNodeIdx_t parse_primary_expression(void);
ASTNodeIdx_t parse_initializer_list(void);
static sstore_pos_t parse_declarator(void);

// Additional function prototypes
static Token_t peek_token(void);
static Token_t next_token(void);
static int expect_token(TokenID_t expected);
static ASTNodeIdx_t create_ast_node(ASTNodeType type, TokenIdx_t token_idx);
static SymIdx_t add_symbol_with_c99_flags(sstore_pos_t name_pos, SymType type, TokenIdx_t token_idx, TypeSpecifier_t *type_spec);
static void parser_init(void);
static const char* get_token_name(TokenID_t token_id);
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
 * @brief Get human-readable name for token ID
 * @param token_id Token ID to convert to name
 * @return String representation of token
 */
static const char* get_token_name(TokenID_t token_id) {
    switch (token_id) {
        case T_EOF: return "EOF";
        case T_INT: return "int";
        case T_LONG: return "long";
        case T_SHORT: return "short";
        case T_FLOAT: return "float";
        case T_DOUBLE: return "double";
        case T_CHAR: return "char";
        case T_VOID: return "void";
        case T_RETURN: return "return";
        case T_IF: return "if";
        case T_ELSE: return "else";
        case T_WHILE: return "while";
        case T_FOR: return "for";
        case T_DO: return "do";
        case T_SWITCH: return "switch";
        case T_CASE: return "case";
        case T_DEFAULT: return "default";
        case T_BREAK: return "break";
        case T_CONTINUE: return "continue";
        case T_GOTO: return "goto";
        case T_SIZEOF: return "sizeof";
        case T_TYPEDEF: return "typedef";
        case T_EXTERN: return "extern";
        case T_STATIC: return "static";
        case T_AUTO: return "auto";
        case T_REGISTER: return "register";
        case T_CONST: return "const";
        case T_VOLATILE: return "volatile";
        case T_SIGNED: return "signed";
        case T_UNSIGNED: return "unsigned";
        case T_STRUCT: return "struct";
        case T_UNION: return "union";
        case T_ENUM: return "enum";
        case T_LPAREN: return "(";
        case T_RPAREN: return ")";
        case T_LBRACE: return "{";
        case T_RBRACE: return "}";
        case T_LBRACKET: return "[";
        case T_RBRACKET: return "]";
        case T_SEMICOLON: return ";";
        case T_COMMA: return ",";
        case T_DOT: return ".";
        case T_ASSIGN: return "=";
        case T_PLUS: return "+";
        case T_MINUS: return "-";
        case T_MUL: return "*";
        case T_DIV: return "/";
        case T_MOD: return "%";
        case T_AMPERSAND: return "&";
        case T_PIPE: return "|";
        case T_CARET: return "^";
        case T_TILDE: return "~";
        case T_LSHIFT: return "<<";
        case T_RSHIFT: return ">>";
        case T_LOGAND: return "&&";
        case T_LOGOR: return "||";
        case T_NOT: return "!";
        case T_EQ: return "==";
        case T_NEQ: return "!=";
        case T_LT: return "<";
        case T_GT: return ">";
        case T_LTE: return "<=";
        case T_GTE: return ">=";
        case T_INC: return "++";
        case T_DEC: return "--";
        case T_PLUSEQ: return "+=";
        case T_MINUSEQ: return "-=";
        case T_MULEQ: return "*=";
        case T_DIVEQ: return "/=";
        case T_MODEQ: return "%=";
        case T_ANDEQ: return "&=";
        case T_OREQ: return "|=";
        case T_XOREQ: return "^=";
        case T_LSHIFTEQ: return "<<=";
        case T_RSHIFTEQ: return ">>=";
        case T_ARROW: return "->";
        case T_QUESTION: return "?";
        case T_COLON: return ":";
        case T_EXCLAMATION: return "!";
        case T_ID: return "identifier";
        case T_LITINT: return "integer literal";
        case T_LITFLOAT: return "float literal";
        case T_LITCHAR: return "character literal";
        case T_LITSTRING: return "string literal";
        case T_ELLIPSIS: return "...";
        // C99 specific tokens
        case T_INLINE: return "inline";
        case T_RESTRICT: return "restrict";
        case T_BOOL: return "_Bool";
        case T_COMPLEX: return "_Complex";
        case T_IMAGINARY: return "_Imaginary";
        case T_ERROR: return "error token";
        case T_UNKNOWN: return "unknown token";
        default: return "unknown token";
    }
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

    // Report syntax error using core error system with token information
    Token_t current = peek_token();
    char error_msg[256];
    snprintf(error_msg, sizeof(error_msg), "Unexpected token '%s' at line %u", 
             get_token_name(current.id), current.line);
    
    SourceLocation_t location = error_create_location(parser_state.current_token);
    error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2001,
                     error_msg, "Check syntax", "parser", NULL);
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
    
    // Get the name string to search for and COPY it to avoid sstore_get() buffer reuse
    char* temp_name = sstore_get(name_pos);
    if (!temp_name) {
        return 0;
    }
    
    // Make a local copy since sstore_get() uses a static buffer
    char search_name[256];
    strncpy(search_name, temp_name, sizeof(search_name) - 1);
    search_name[sizeof(search_name) - 1] = '\0';
    
    // Search all symbols for matches with this name
    uint32_t symbol_count = symtab_get_count();
    
    for (SymIdx_t i = 1; i <= symbol_count; i++) {
        SymTabEntry entry = symtab_get(i);
        
        // Skip entries without names
        if (entry.name == 0) {
            continue;
        }
        
        // Compare actual string content, not positions
        char* entry_name = sstore_get(entry.name);
        if (!entry_name || strcmp(search_name, entry_name) != 0) {
            continue; // Name doesn't match
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
 * @brief Add symbol to symbol table with C99 flags from TypeSpecifier_t
 */
static SymIdx_t add_symbol_with_c99_flags(sstore_pos_t name_pos, SymType type, TokenIdx_t token_idx, TypeSpecifier_t *type_spec) {
    SymTabEntry entry = {0};
    entry.type = type;
    entry.name = name_pos;
    entry.parent = 0;  // Not used in C99 scoping
    entry.line = token_idx;
    entry.scope_depth = parser_state.scope_depth;  // Record C99 scope depth

    // Set C99 flags based on TypeSpecifier_t
    unsigned int c99_flags = 0;
    if (type_spec->has_inline) c99_flags |= SYM_FLAG_INLINE;
    if (type_spec->has_restrict) c99_flags |= SYM_FLAG_RESTRICT;
    if (type_spec->has_complex) c99_flags |= SYM_FLAG_COMPLEX;
    if (type_spec->has_imaginary) c99_flags |= SYM_FLAG_IMAGINARY;
    if (type_spec->has_const) c99_flags |= SYM_FLAG_CONST;
    if (type_spec->has_volatile) c99_flags |= SYM_FLAG_VOLATILE;
    entry.flags = c99_flags;

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
                if (node) {
                    // Look up the symbol index - this is REQUIRED for identifiers
                    SymIdx_t sym_idx = lookup_symbol(token.pos);
                    if (sym_idx != 0) {
                        // Store symbol index - this is the only valid reference for identifiers
                        node->ast.binary.value.symbol_idx = sym_idx;
                    } else {
                        // ERROR: Undefined identifier - report semantic error and fail
                        char* identifier_name = sstore_get(token.pos);
                        semantic_error_undefined_symbol(token_idx, identifier_name);
                        // Do not set any value - let the error handling decide what to do
                        return 0; // Return invalid node index to propagate error
                    }
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
            ASTNodeIdx_t node_idx = create_ast_node(AST_LIT_STRING, token_idx);
            if (node_idx) {
                HBNode *node = HBGet(node_idx, HBMODE_AST);
                if (node) {
                    // Store string position for string literals
                    node->ast.binary.value.string_pos = token.pos;
                }
            }
            return node_idx;
        }

        case T_LITCHAR: {
            next_token();
            ASTNodeIdx_t node_idx = create_ast_node(AST_LIT_CHAR, token_idx);
            if (node_idx) {
                HBNode *node = HBGet(node_idx, HBMODE_AST);
                if (node) {
                    // Extract character value from string store
                    char *str = sstore_get(token.pos);
                    if (str && str[0] == '\'' && str[2] == '\'') {
                        node->ast.binary.value.long_value = (long)str[1]; // Character value
                    }
                }
            }
            return node_idx;
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

        case T_LBRACE: {
            // C99 compound literal / initializer list
            return parse_initializer_list();
        }

        default: {
            Token_t current = peek_token();
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Expected primary expression, found '%s' at line %u", 
                     get_token_name(current.id), current.line);
            
            SourceLocation_t location = error_create_location(token_idx);
            error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                             error_msg, "Check expression syntax", "parser", NULL);
            return 0;
        }
    }
}

/**
 * @brief Parse C99 initializer lists with designated initializers
 * Handles: { .field = value, [index] = value, value }
 */
ASTNodeIdx_t parse_initializer_list(void) {
    TokenIdx_t token_idx = tstore_getidx();
    
    if (!expect_token(T_LBRACE)) {
        return 0;
    }
    
    ASTNodeIdx_t init_node = create_ast_node(AST_INITIALIZER, token_idx);
    if (!init_node) return 0;
    
    ASTNodeIdx_t first_init = 0;
    ASTNodeIdx_t last_init = 0;
    
    // Parse initializer elements
    while (peek_token().id != T_RBRACE && peek_token().id != T_EOF) {
        ASTNodeIdx_t element = 0;
        
        // Check for designated initializers
        if (peek_token().id == T_DOT) {
            // Field designator: .field = value
            next_token(); // consume '.'
            if (peek_token().id == T_ID) {
                Token_t field_token = peek_token();
                TokenIdx_t field_idx = tstore_getidx();
                next_token(); // consume field name
                
                if (expect_token(T_ASSIGN)) {
                    ASTNodeIdx_t value = parse_assignment_expression();
                    element = create_ast_node(AST_EXPR_DESIGNATED_FIELD, field_idx);
                    if (element) {
                        HBNode *node = HBGet(element, HBMODE_AST);
                        if (node) {
                            node->ast.binary.left = 0; // Field name stored in token_idx
                            node->ast.binary.right = value;
                            node->ast.binary.value.string_pos = field_token.pos;
                        }
                    }
                }
            }
        } else if (peek_token().id == T_LBRACKET) {
            // Array designator: [index] = value
            next_token(); // consume '['
            ASTNodeIdx_t index = parse_expression();
            if (expect_token(T_RBRACKET) && expect_token(T_ASSIGN)) {
                ASTNodeIdx_t value = parse_assignment_expression();
                element = create_ast_node(AST_EXPR_DESIGNATED_INDEX, token_idx);
                if (element) {
                    HBNode *node = HBGet(element, HBMODE_AST);
                    if (node) {
                        node->ast.binary.left = index;
                        node->ast.binary.right = value;
                    }
                }
            }
        } else {
            // Regular initializer element
            element = parse_assignment_expression();
        }
        
        // Chain initializer elements
        if (element) {
            if (!first_init) {
                first_init = element;
                last_init = element;
            } else {
                // Use child2 for chaining (similar to statement chaining)
                HBNode *last_node = HBGet(last_init, HBMODE_AST);
                if (last_node) {
                    last_node->ast.children.child2 = element;
                    last_init = element;
                }
            }
        }
        
        // Handle comma or end
        if (peek_token().id == T_COMMA) {
            next_token(); // consume comma
        } else {
            break;
        }
    }
    
    expect_token(T_RBRACE);
    
    // Set up the initializer node
    HBNode *init = HBGet(init_node, HBMODE_AST);
    if (init) {
        init->ast.children.child1 = first_init; // First initializer element
    }
    
    return init_node;
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
    ASTNodeIdx_t left = parse_conditional_expression();
    if (!left) return 0;

    Token_t token = peek_token();
    if (token.id == T_ASSIGN || token.id == T_MULEQ || token.id == T_DIVEQ || 
        token.id == T_PLUSEQ || token.id == T_MINUSEQ || token.id == T_MODEQ) {
        TokenIdx_t token_idx = tstore_getidx();
        next_token(); // consume assignment operator
        
        // Assignment is right-associative, so parse another assignment expression
        ASTNodeIdx_t right = parse_assignment_expression();
        if (!right) {
            SourceLocation_t location = error_create_location(token_idx);
            error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                             "Expected right operand after assignment operator", "Check assignment syntax", "parser", NULL);
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
 * @brief Parse conditional expressions (ternary operator: condition ? true_expr : false_expr)
 * Right-associative: a ? b : c ? d : e becomes a ? b : (c ? d : e)
 * C99 standard: 6.5.15 Conditional operator
 */
ASTNodeIdx_t parse_conditional_expression(void) {
    ASTNodeIdx_t condition = parse_relational_expression();
    if (!condition) return 0;

    Token_t token = peek_token();
    if (token.id == T_QUESTION) {
        TokenIdx_t token_idx = tstore_getidx();
        next_token(); // consume '?'
        
        // Parse true expression
        ASTNodeIdx_t true_expr = parse_expression();
        if (!true_expr) {
            Token_t current = peek_token();
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Expected expression after '?', found '%s' at line %u", 
                     get_token_name(current.id), current.line);
            
            SourceLocation_t location = error_create_location(token_idx);
            error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                             error_msg, "Check conditional expression syntax", "parser", NULL);
            return condition;
        }

        // Expect ':' 
        if (peek_token().id != T_COLON) {
            Token_t current = peek_token();
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Expected ':' in conditional expression, found '%s' at line %u", 
                     get_token_name(current.id), current.line);
            
            SourceLocation_t location = error_create_location(tstore_getidx());
            error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                             error_msg, "Check conditional expression syntax", "parser", NULL);
            return condition;
        }
        
        next_token(); // consume ':'
        
        // Parse false expression (right-associative)
        ASTNodeIdx_t false_expr = parse_conditional_expression();
        if (!false_expr) {
            Token_t current = peek_token();
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Expected expression after ':', found '%s' at line %u", 
                     get_token_name(current.id), current.line);
            
            SourceLocation_t location = error_create_location(token_idx);
            error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                             error_msg, "Check conditional expression syntax", "parser", NULL);
            return condition;
        }

        // Create conditional expression node
        ASTNodeIdx_t cond_node = create_ast_node(AST_EXPR_CONDITIONAL, token_idx);
        if (cond_node) {
            HBNode* hb_node = HBGet(cond_node, HBMODE_AST);
            if (hb_node) {
                hb_node->ast.conditional.condition = condition;
                hb_node->ast.conditional.then_stmt = true_expr;
                hb_node->ast.conditional.else_stmt = false_expr;
            }
        }
        return cond_node;
    }

    return condition;
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
            Token_t current = peek_token();
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Expected right operand, found '%s' at line %u", 
                     get_token_name(current.id), current.line);
            
            SourceLocation_t location = error_create_location(token_idx);
            error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                             error_msg, "Check expression syntax", "parser", NULL);
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
        } else if (token.id == T_INC || token.id == T_DEC) {
            // Postfix increment/decrement: identifier++ or identifier--
            TokenIdx_t token_idx = tstore_getidx();
            next_token(); // consume '++' or '--'
            
            ASTNodeIdx_t postfix_node = create_ast_node(AST_EXPR_UNARY_OP, token_idx);
            if (postfix_node) {
                HBNode *node = HBGet(postfix_node, HBMODE_AST);
                node->ast.unary.operand = left;
                node->ast.unary.operator = token.id; // Store T_INC or T_DEC
            }
            left = postfix_node;
        } else if (token.id == T_LBRACKET) {
            // Array subscript: array[index]
            TokenIdx_t token_idx = tstore_getidx();
            next_token(); // consume '['
            
            ASTNodeIdx_t index = parse_expression();
            if (!index) {
                Token_t current = peek_token();
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "Expected array index expression, found '%s' at line %u", 
                         get_token_name(current.id), current.line);
                
                SourceLocation_t location = error_create_location(token_idx);
                error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                                 error_msg, "Check array subscript syntax", "parser", NULL);
                return left;
            }
            
            expect_token(T_RBRACKET);
            
            // Create array subscript node (using binary op structure)
            ASTNodeIdx_t subscript_node = create_ast_node(AST_EXPR_BINARY_OP, token_idx);
            if (subscript_node) {
                HBNode *node = HBGet(subscript_node, HBMODE_AST);
                node->ast.binary.left = left;     // array
                node->ast.binary.right = index;   // index
                // The token_idx already contains the '[' position for context
            }
            left = subscript_node;
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
        if (token.id != T_MUL && token.id != T_DIV && token.id != T_MOD) {
            break;
        }

        TokenIdx_t token_idx = tstore_getidx();
        next_token(); // consume operator
        
        ASTNodeIdx_t right = parse_unary_expression();
        if (!right) {
            Token_t current = peek_token();
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Expected right operand, found '%s' at line %u", 
                     get_token_name(current.id), current.line);
            
            SourceLocation_t location = error_create_location(token_idx);
            error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                             error_msg, "Check expression syntax", "parser", NULL);
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
        case T_NOT:
        case T_INC:
        case T_DEC:
        case T_MUL: // Pointer dereference: *ptr
        case T_AMPERSAND: // Address-of: &var
        case T_TILDE: { // Bitwise NOT: ~value
            next_token(); // consume unary operator
            
            ASTNodeIdx_t operand = parse_unary_expression(); // Right-associative
            if (!operand) {
                Token_t current = peek_token();
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "Expected operand after unary operator, found '%s' at line %u", 
                         get_token_name(current.id), current.line);
                
                SourceLocation_t location = error_create_location(token_idx);
                error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                                 error_msg, "Check unary expression syntax", "parser", NULL);
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

        case T_FOR: {
            next_token(); // consume 'for'
            expect_token(T_LPAREN);
            
            // Parse for loop components: for(init; condition; update)
            ASTNodeIdx_t init = 0;
            if (peek_token().id != T_SEMICOLON) {
                // Parse init (could be declaration or expression)
                if (is_type_specifier_start(peek_token().id)) {
                    init = parse_declaration();
                } else {
                    init = parse_expression();
                    expect_token(T_SEMICOLON);
                }
            } else {
                next_token(); // consume empty init semicolon
            }
            
            ASTNodeIdx_t condition = 0;
            if (peek_token().id != T_SEMICOLON) {
                condition = parse_expression();
            }
            expect_token(T_SEMICOLON);
            
            ASTNodeIdx_t update = 0;
            if (peek_token().id != T_RPAREN) {
                update = parse_expression();
            }
            expect_token(T_RPAREN);
            
            ASTNodeIdx_t body = parse_statement();
            
            ASTNodeIdx_t node_idx = create_ast_node(AST_STMT_FOR, token_idx);
            if (node_idx) {
                HBNode *node = HBGet(node_idx, HBMODE_AST);
                // Use children structure for for loop parts
                node->ast.children.child1 = init;     // initialization  
                node->ast.children.child2 = condition; // condition
                node->ast.children.child3 = update;   // update
                // Use conditional structure for body
                node->ast.conditional.then_stmt = body;
            }
            return node_idx;
        }

        case T_LBRACE: {
            // Compound statement - creates a new scope (C99 compliant)
            next_token(); // consume '{'
            
            // Enter new scope for this block
            enter_scope();
            
            ASTNodeIdx_t compound = create_ast_node(AST_STMT_COMPOUND, token_idx);
            ASTNodeIdx_t first_stmt = 0;
            ASTNodeIdx_t last_stmt = 0;

            // C99: Parse mixed declarations and statements
            while (peek_token().id != T_RBRACE && peek_token().id != T_EOF) {
                ASTNodeIdx_t stmt = 0;
                Token_t next_token_peek = peek_token();
                
                // C99: Declarations can appear anywhere in a block, mixed with statements
                if (is_type_specifier_start(next_token_peek.id)) {
                    // Parse declaration
                    stmt = parse_declaration();
                } else {
                    // Parse statement
                    stmt = parse_statement();
                }
                
                if (!stmt) break;
                
                // Chain all statements properly - manifest compliant (no circular references)
                if (!first_stmt) {
                    first_stmt = stmt;
                    last_stmt = stmt;
                } else if (last_stmt && stmt != last_stmt) {  // Prevent self-references
                    // Chain statements using a clean approach that respects AST node structure
                    HBNode *last_node = HBGet(last_stmt, HBMODE_AST);
                    if (last_node) {
                        // Use child2 for statement chaining in compound statements
                        // This is safe because most statements don't use child2 for their own structure
                        last_node->ast.children.child2 = stmt;
                    }
                    last_stmt = stmt;
                }
            }

            expect_token(T_RBRACE);
            
            // Set up the compound statement properly
            if (compound) {
                HBNode *comp_node = HBGet(compound, HBMODE_AST);
                if (comp_node) {
                    // In C99, all declarations and statements are mixed in one list
                    comp_node->ast.compound.declarations = 0;      // Not used in C99 mixed mode
                    comp_node->ast.compound.statements = first_stmt; // Only first statement to avoid cycles
                    comp_node->ast.compound.scope_idx = parser_state.scope_depth; // Current scope
                }
            }
            
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
    
    // Check for bare type declarations (like "struct Point { ... };")
    if (peek_token().id == T_SEMICOLON) {
        next_token();  // consume semicolon
        return create_ast_node(AST_VAR_DECL, token_idx);
    }
    
    // Parse declarator (handles pointers, identifier, and arrays)
    sstore_pos_t identifier_pos = parse_declarator();
    if (identifier_pos == 0) {
        SourceLocation_t location = error_create_location(tstore_getidx());
        error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2001,
                         "Missing identifier", "Expected identifier after type", "parser", NULL);
        return 0;
    }

    // Check if this is a function definition
    if (peek_token().id == T_LPAREN) {
        // Function definition - functions have file scope (depth 0) in C99
        // But first ensure we're at file scope for function definitions
        if (parser_state.scope_depth != 0) {
            SourceLocation_t location = error_create_location(tstore_getidx());
            error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2001,
                             "Function definition not allowed in block scope", 
                             "C99: Functions must be defined at file scope", "parser", NULL);
            return 0;
        }
        
        parser_state.in_function = 1;
        // Function symbol will be created when we create the AST node

        next_token();  // consume '('
        
        // Enter function parameter scope (depth 1 for C99 function scope)
        int saved_scope = parser_state.scope_depth;
        parser_state.scope_depth = 1;
        
        // Parse parameters (C99 compliant parameter handling)
        while (peek_token().id != T_RPAREN && peek_token().id != T_EOF) {
            // Parse parameter type
            if (is_type_specifier_start(peek_token().id)) {
                TypeSpecifier_t param_type = parse_type_specifiers();
                if (param_type.is_valid) {
                    // Parse declarator (handles pointers and qualifiers)
                    sstore_pos_t param_name_pos = parse_declarator();
                    if (param_name_pos != 0) {
                        // Parameters have function scope (depth 1) in C99
                        add_symbol_with_c99_flags(param_name_pos, SYM_VARIABLE, tstore_getidx(), &param_type);
                    }
                }
                // Handle comma between parameters
                if (peek_token().id == T_COMMA) {
                    next_token();  // consume comma
                } else if (peek_token().id != T_RPAREN) {
                    // Invalid parameter list
                    break;
                }
            } else {
                // Skip unknown tokens to avoid infinite loop, but report error
                Token_t current = peek_token();
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "Unexpected token '%s' in parameter list at line %u", 
                         get_token_name(current.id), current.line);
                
                SourceLocation_t location = error_create_location(tstore_getidx());
                error_core_report(ERROR_WARNING, ERROR_SYNTAX, &location, 2002,
                                 error_msg, 
                                 "Check parameter syntax", "parser", NULL);
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
                if (node) {
                    // Use declaration structure as specified in AST Node Reference
                    SymIdx_t sym_idx = add_symbol_with_c99_flags(identifier_pos, SYM_FUNCTION, token_idx, &type_spec);
                    node->ast.declaration.symbol_idx = sym_idx;     // Function symbol
                    node->ast.declaration.type_idx = 0;             // Function type (to be added)
                    node->ast.declaration.initializer = body;       // Function body
                    node->ast.declaration.storage_class = 0;        // Default storage
                }
            }
            // Exit function scope back to file scope (depth 0)
            parser_state.in_function = 0;
            parser_state.scope_depth = saved_scope;
            return func_node;
        } else {
            // Function declaration only
            expect_token(T_SEMICOLON);
            // Reset scope depth for function declaration
            parser_state.scope_depth = saved_scope;
            return create_ast_node(AST_FUNCTION_DECL, token_idx);
        }
    } else {
        // Variable declaration - handle multiple declarators separated by commas
        ASTNodeIdx_t first_decl = 0;
        ASTNodeIdx_t last_decl = 0;
        
        do {
            // Create symbol for this declarator
            SymIdx_t sym_idx = add_symbol_with_c99_flags(identifier_pos, SYM_VARIABLE, tstore_getidx(), &type_spec);

            // Handle optional initializer
            ASTNodeIdx_t decl_node = create_ast_node(AST_VAR_DECL, token_idx);
            if (decl_node) {
                HBNode *node = HBGet(decl_node, HBMODE_AST);
                if (node) {
                    node->ast.declaration.symbol_idx = sym_idx;
                    node->ast.declaration.type_idx = 0;
                    
                    if (peek_token().id == T_ASSIGN) {
                        next_token();  // consume '='
                        ASTNodeIdx_t init_expr = parse_expression();
                        node->ast.declaration.initializer = init_expr;
                    } else {
                        node->ast.declaration.initializer = 0;
                    }
                }
            }
            
            // Chain multiple declarations
            if (!first_decl) {
                first_decl = decl_node;
                last_decl = decl_node;
            } else {
                // Chain using child2 (similar to statement chaining)
                if (last_decl) {
                    HBNode *last_node = HBGet(last_decl, HBMODE_AST);
                    if (last_node) {
                        last_node->ast.children.child2 = decl_node;
                    }
                }
                last_decl = decl_node;
            }
            
            // Check for comma (more declarators)
            if (peek_token().id == T_COMMA) {
                next_token(); // consume comma
                
                // Parse next declarator
                identifier_pos = parse_declarator();
                if (identifier_pos == 0) {
                    SourceLocation_t location = error_create_location(tstore_getidx());
                    error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2001,
                                     "Expected identifier after comma", "Check declaration syntax", "parser", NULL);
                    break;
                }
            } else {
                break; // No more declarators
            }
        } while (1);
        
        expect_token(T_SEMICOLON);
        return first_decl; // Return the first declaration in the chain
    }
}

/**
 * @brief Parse the complete program (translation unit) - C99 compliant
 */
ASTNodeIdx_t parse_program(void) {
    ASTNodeIdx_t program_node = create_ast_node(AST_PROGRAM, 0);
    ASTNodeIdx_t first_decl = 0;
    ASTNodeIdx_t last_decl = 0;

    // C99: Translation unit consists of external declarations only
    while (peek_token().id != T_EOF) {
        // Ensure we're at file scope for all external declarations
        parser_state.scope_depth = 0;
        
        ASTNodeIdx_t decl = parse_declaration();
        if (!decl) {
            // Skip to next likely declaration start or advance at least one token
            Token_t token = next_token();
            if (token.id == T_EOF) break;
            
            // Try to recover by finding next type specifier
            while (peek_token().id != T_EOF && !is_type_specifier_start(peek_token().id)) {
                next_token();
            }
            continue;
        }

        // Build a proper declaration list - avoid circular references
        if (!first_decl) {
            // First declaration
            first_decl = decl;
            last_decl = decl;
            
            // Link program to first declaration using child1
            if (program_node) {
                HBNode *prog_node = HBGet(program_node, HBMODE_AST);
                if (prog_node) {
                    prog_node->ast.children.child1 = first_decl;
                }
            }
        } else {
            // Chain subsequent declarations using child2 as 'next' pointer
            if (last_decl) {
                HBNode *last_node = HBGet(last_decl, HBMODE_AST);
                if (last_node) {
                    // Ensure we don't create circular references
                    if (last_node->ast.children.child2 == 0) {
                        last_node->ast.children.child2 = decl;
                    }
                }
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
    TypeSpecifier_t type = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // Initialize all fields
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

            // C99 storage class specifiers
            case T_INLINE:
                if (type.has_inline) {
                    type.is_valid = 0; // Cannot have multiple inline
                    return type;
                }
                type.has_inline = 1;
                advance = 1;
                break;

            case T_STATIC:
                if (type.has_static || type.has_extern || type.has_auto || type.has_register) {
                    type.is_valid = 0; // Cannot mix storage classes
                    return type;
                }
                type.has_static = 1;
                advance = 1;
                break;

            case T_EXTERN:
                if (type.has_static || type.has_extern || type.has_auto || type.has_register) {
                    type.is_valid = 0; // Cannot mix storage classes
                    return type;
                }
                type.has_extern = 1;
                advance = 1;
                break;

            case T_AUTO:
                if (type.has_static || type.has_extern || type.has_auto || type.has_register) {
                    type.is_valid = 0; // Cannot mix storage classes
                    return type;
                }
                type.has_auto = 1;
                advance = 1;
                break;

            case T_REGISTER:
                if (type.has_static || type.has_extern || type.has_auto || type.has_register) {
                    type.is_valid = 0; // Cannot mix storage classes
                    return type;
                }
                type.has_register = 1;
                advance = 1;
                break;

            case T_TYPEDEF:
                if (type.has_typedef) {
                    type.is_valid = 0; // Cannot have multiple typedef
                    return type;
                }
                type.has_typedef = 1;
                advance = 1;
                break;

            // Type qualifiers
            case T_CONST:
                if (type.has_const) {
                    type.is_valid = 0; // Cannot have multiple const
                    return type;
                }
                type.has_const = 1;
                advance = 1;
                break;

            case T_VOLATILE:
                if (type.has_volatile) {
                    type.is_valid = 0; // Cannot have multiple volatile
                    return type;
                }
                type.has_volatile = 1;
                advance = 1;
                break;

            case T_RESTRICT:
                if (type.has_restrict) {
                    type.is_valid = 0; // Cannot have multiple restrict
                    return type;
                }
                type.has_restrict = 1;
                advance = 1;
                break;

            // C99 complex/imaginary types
            case T_COMPLEX:
                if (type.has_complex || type.has_imaginary) {
                    type.is_valid = 0; // Cannot mix complex/imaginary
                    return type;
                }
                type.has_complex = 1;
                advance = 1;
                break;

            case T_IMAGINARY:
                if (type.has_complex || type.has_imaginary) {
                    type.is_valid = 0; // Cannot mix complex/imaginary
                    return type;
                }
                type.has_imaginary = 1;
                advance = 1;
                break;

            case T_BOOL:
                if (type.base_type != 0 || type.has_signed != 0 || type.has_long || type.has_short) {
                    type.is_valid = 0; // _Bool cannot be combined with modifiers
                    return type;
                }
                type.base_type = T_BOOL;
                advance = 1;
                break;
                
            case T_STRUCT:
            case T_UNION:
            case T_ENUM:
                // For now, treat these as base types (proper struct parsing would be more complex)
                if (type.base_type != 0) {
                    type.is_valid = 0; // Cannot have multiple base types
                    return type;
                }
                type.base_type = token.id;
                next_token(); // consume struct/union/enum keyword
                
                // Skip the tag name if present
                if (peek_token().id == T_ID) {
                    next_token(); // consume tag name
                }
                
                // Skip struct body if present
                if (peek_token().id == T_LBRACE) {
                    next_token(); // consume '{'
                    int brace_count = 1;
                    while (brace_count > 0 && peek_token().id != T_EOF) {
                        Token_t t = next_token();
                        if (t.id == T_LBRACE) brace_count++;
                        else if (t.id == T_RBRACE) brace_count--;
                    }
                }
                tokens_consumed++; // We consumed tokens manually
                advance = 0; // Don't advance again - we already did it manually
                break;

            default:
                // Not a type specifier, stop parsing
                if (tokens_consumed == 0) {
                    // No type specifiers found at all - this should not be an error
                    // if the current token is actually a valid type specifier that
                    // we missed in our switch statement
                    if (is_type_specifier_start(token.id)) {
                        // This is a bug - we should handle this token properly
                        fprintf(stderr, "ERROR: Unhandled type specifier token %d\n", token.id);
                        type.is_valid = 0;
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
 * @brief Parse a simple declarator (handles pointer syntax, identifier, and arrays)
 * Simplified C99 declarator parsing for parameters and declarations  
 * @return Position of the identifier, or 0 on error
 */
static sstore_pos_t parse_declarator(void) {
    // Handle pointer prefixes (* and * restrict, * const, etc.)
    while (peek_token().id == T_MUL) {
        next_token(); // consume '*'
        
        // Skip type qualifiers after '*' 
        while (peek_token().id == T_CONST || peek_token().id == T_VOLATILE || peek_token().id == T_RESTRICT) {
            next_token(); // consume qualifier
        }
    }
    
    // Expect identifier after all pointers and qualifiers
    if (peek_token().id != T_ID) {
        return 0; // No valid identifier found
    }
    
    Token_t id_token = peek_token();
    next_token(); // consume identifier
    
    // Handle array dimensions - C99 supports variable length arrays
    while (peek_token().id == T_LBRACKET) {
        next_token(); // consume '['
        
        // Parse array size expression (could be constant or variable for VLA)
        if (peek_token().id != T_RBRACKET) {
            // Parse the array size expression
            ASTNodeIdx_t size_expr = parse_assignment_expression();
            if (!size_expr) {
                Token_t current = peek_token();
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "Expected array size expression, found '%s' at line %u", 
                         get_token_name(current.id), current.line);
                
                SourceLocation_t location = error_create_location(tstore_getidx());
                error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                                 error_msg, "Check array declaration syntax", "parser", NULL);
                return 0;
            }
        }
        
        // Expect closing bracket
        if (peek_token().id != T_RBRACKET) {
            Token_t current = peek_token();
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Expected ']' in array declaration, found '%s' at line %u", 
                     get_token_name(current.id), current.line);
            
            SourceLocation_t location = error_create_location(tstore_getidx());
            error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 2003,
                             error_msg, "Check array declaration syntax", "parser", NULL);
            return 0;
        }
        next_token(); // consume ']'
    }
    
    return id_token.pos;
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
            token_id == T_VOLATILE || token_id == T_INLINE || token_id == T_RESTRICT ||
            token_id == T_COMPLEX || token_id == T_IMAGINARY || token_id == T_BOOL);
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
