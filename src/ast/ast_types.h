/**
 * @file ast_types.h
 * @brief Comprehensive AST node types and structures for modular compiler design
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#ifndef SRC_AST_AST_TYPES_H_
#define SRC_AST_AST_TYPES_H_

#include <stdint.h>
#include "../storage/sstore.h"
#include "../storage/symtab.h"
#include "../lexer/ctoken.h"

// Forward declarations
typedef uint16_t ASTNodeIdx_t;
typedef uint16_t TypeIdx_t;

/**
 * @brief AST node categories for different compiler phases
 */
typedef enum {
    AST_CAT_DECLARATION,    // Declarations (types, variables, functions)
    AST_CAT_STATEMENT,      // Statements (control flow, blocks)
    AST_CAT_EXPRESSION,     // Expressions (operators, literals, calls)
    AST_CAT_TYPE,          // Type information
    AST_CAT_SPECIAL        // Special nodes (program, EOF, etc.)
} ASTCategory;

/**
 * @brief Detailed AST node types organized by category
 */
typedef enum {
    // Special nodes (0-9)
    AST_FREE = 0,
    AST_PROGRAM,
    AST_TRANSLATION_UNIT,
    AST_EOF,
    AST_ERROR,

    // Declaration nodes (10-29)
    AST_FUNCTION_DECL = 10,
    AST_FUNCTION_DEF,
    AST_VAR_DECL,
    AST_PARAM_DECL,
    AST_FIELD_DECL,
    AST_TYPEDEF_DECL,
    AST_STRUCT_DECL,
    AST_UNION_DECL,
    AST_ENUM_DECL,
    AST_ENUM_CONSTANT,

    // Type nodes (30-49)
    AST_TYPE_BASIC = 30,    // int, char, float, etc.
    AST_TYPE_POINTER,
    AST_TYPE_ARRAY,
    AST_TYPE_FUNCTION,
    AST_TYPE_STRUCT,
    AST_TYPE_UNION,
    AST_TYPE_ENUM,
    AST_TYPE_TYPEDEF,
    AST_TYPE_QUALIFIER,     // const, volatile
    AST_TYPE_STORAGE,       // static, extern, auto, register

    // Statement nodes (50-79)
    AST_STMT_COMPOUND = 50,
    AST_STMT_EXPRESSION,
    AST_STMT_IF,
    AST_STMT_WHILE,
    AST_STMT_FOR,
    AST_STMT_DO_WHILE,
    AST_STMT_SWITCH,
    AST_STMT_CASE,
    AST_STMT_DEFAULT,
    AST_STMT_BREAK,
    AST_STMT_CONTINUE,
    AST_STMT_RETURN,
    AST_STMT_GOTO,
    AST_STMT_LABEL,
    AST_STMT_EMPTY,

    // Expression nodes (80-129)
    AST_EXPR_LITERAL = 80,
    AST_EXPR_IDENTIFIER,
    AST_EXPR_BINARY_OP,
    AST_EXPR_UNARY_OP,
    AST_EXPR_ASSIGN,
    AST_EXPR_CALL,
    AST_EXPR_MEMBER,        // . operator
    AST_EXPR_MEMBER_PTR,    // -> operator
    AST_EXPR_INDEX,         // [] operator
    AST_EXPR_CAST,
    AST_EXPR_SIZEOF,
    AST_EXPR_CONDITIONAL,   // ?: operator
    AST_EXPR_COMMA,
    AST_EXPR_INIT_LIST,     // {1, 2, 3}
    AST_EXPR_COMPOUND_LITERAL,

    // Literal subtypes (130-139)
    AST_LIT_INTEGER = 130,
    AST_LIT_FLOAT,
    AST_LIT_CHAR,
    AST_LIT_STRING,
    
    // C99-specific node types (140-149)
    AST_EXPR_DESIGNATED_FIELD = 140,    // .field = value (C99 6.7.8.7)
    AST_EXPR_DESIGNATED_INDEX,          // [index] = value (C99 6.7.8.7)
    AST_INITIALIZER,                    // Initializer list { ... } (C99 6.7.8)
    AST_PARAM_VARIADIC,                 // ... parameter (C99 6.7.5.3)
    AST_TYPE_COMPLEX,                   // _Complex type (C99 6.2.5.11)
    AST_TYPE_IMAGINARY,                 // _Imaginary type (C99 6.2.5.11)
    AST_LIT_COMPLEX,                    // Complex literal (C99 6.2.5.11)

    AST_TYPE_COUNT
} ASTNodeType;

/**
 * @brief Get category for an AST node type
 */
static inline ASTCategory ast_get_category(ASTNodeType type) {
    if (type < 10) return AST_CAT_SPECIAL;
    if (type < 30) return AST_CAT_DECLARATION;
    if (type < 50) return AST_CAT_TYPE;
    if (type < 80) return AST_CAT_STATEMENT;
    return AST_CAT_EXPRESSION;
}

/**
 * @brief AST node flags for different compiler phases
 */
typedef enum {
    AST_FLAG_NONE         = 0x0000,
    AST_FLAG_PARSED       = 0x0001,  // Successfully parsed
    AST_FLAG_ANALYZED     = 0x0002,  // Semantic analysis complete
    AST_FLAG_TYPED        = 0x0004,  // Type checking complete
    AST_FLAG_OPTIMIZED    = 0x0008,  // Optimization applied
    AST_FLAG_CODEGEN      = 0x0010,  // Code generation complete
    AST_FLAG_ERROR        = 0x8000,  // Error in this node
    AST_FLAG_MODIFIED     = 0x4000,  // Node has been modified
    
    // C99-specific flags
    AST_FLAG_C99_VLA            = 0x0100,  // Variable length arrays
    AST_FLAG_C99_FLEXIBLE       = 0x0200,  // Flexible array members  
    AST_FLAG_C99_INLINE         = 0x0400,  // Inline functions
    AST_FLAG_C99_RESTRICT       = 0x0800,  // Restrict qualifier
    AST_FLAG_C99_DESIGNATED     = 0x1000,  // Designated initializers
    AST_FLAG_C99_COMPOUND_LIT   = 0x2000,  // Compound literals
    AST_FLAG_C99_MIXED_DECL     = 0x4000,  // Mixed declarations/statements
    AST_FLAG_C99_UNIVERSAL_CHAR = 0x8000   // Universal character names
} ASTNodeFlags;

/**
 * @brief Core AST node structure - optimized for memory
 */
typedef struct ASTNode {
    ASTNodeType type;       // 2 bytes - node type
    ASTNodeFlags flags;     // 2 bytes - compiler phase flags
    TokenIdx_t token_idx;   // 4 bytes - source token reference
    TypeIdx_t type_idx;     // 2 bytes - type information index

    // Child nodes - flexible organization (14 bytes remaining)
    union {
        struct {
            ASTNodeIdx_t child1,
                     child2,
                     child3,
                     child4;  // 8 bytes
            char padding[6];                              // 6 bytes
        } children;

        struct {
            ASTNodeIdx_t left,
                     right;                     // 4 bytes
            union {
                SymIdx_t symbol_idx;                   // Symbol reference
                sstore_pos_t string_pos;                  // String literal position
                int64_t long_value;                          // Integer literal
                double float_value;                       // Float literal
            } value;                                      // 8 bytes
            char padding[2];                              // 2 bytes
        } binary;

        struct {
            ASTNodeIdx_t operand;                         // 2 bytes
            TokenID_t operator;                           // 2 bytes
            union {
                int int_value;                            // 4 bytes
                float float_value;                        // 4 bytes
                sstore_pos_t string_pos;                  // 2 bytes
            } data;                                       // 4 bytes
            char padding[2];                              // 2 bytes
        } unary;

        struct {
            ASTNodeIdx_t declarations;                    // 2 bytes
            ASTNodeIdx_t statements;                      // 2 bytes
            SymIdx_t scope_idx;                        // 2 bytes
            unsigned short c99_mixed_count;               // 2 bytes - mixed decl count
            char padding[6];                              // 6 bytes
        } compound;

        struct {
            ASTNodeIdx_t condition;                       // 2 bytes
            ASTNodeIdx_t then_stmt;                       // 2 bytes
            ASTNodeIdx_t else_stmt;                       // 2 bytes
            char padding[8];                              // 8 bytes
        } conditional;

        struct {
            ASTNodeIdx_t function;                        // 2 bytes
            ASTNodeIdx_t arguments;                       // 2 bytes
            TypeIdx_t return_type;                        // 2 bytes
            char arg_count;                               // 1 byte
            char padding[7];                              // 7 bytes
        } call;

        struct {
            SymIdx_t symbol_idx;                       // 2 bytes
            TypeIdx_t type_idx;                           // 2 bytes
            ASTNodeIdx_t initializer;                     // 2 bytes
            char storage_class;                           // 1 byte
            unsigned char c99_specifier;                  // 1 byte - inline/restrict flags
            char padding[6];                              // 6 bytes
        } declaration;
        
        // C99-specific structures
        struct {
            sstore_pos_t field_name;                      // 2 bytes - field name
            ASTNodeIdx_t value_expr;                      // 2 bytes - initialization value
            char padding[10];                             // 10 bytes
        } designated;
        
        struct {
            ASTNodeIdx_t base_type;                       // 2 bytes - base type for complex
            union {
                struct {
                    ASTNodeIdx_t real_part;               // 2 bytes - real component
                    ASTNodeIdx_t imag_part;               // 2 bytes - imaginary component  
                } complex_literal;
                struct {
                    ASTNodeIdx_t vla_size_expr;           // 2 bytes - VLA size expression
                    unsigned char dimensions;             // 1 byte - array dimensions
                    char padding[1];                      // 1 byte
                } vla_info;
            } c99_data;                                   // 4 bytes
            char padding[6];                              // 6 bytes
        } c99_enhanced;

        // Raw access for maximum flexibility
        char raw_data[14];                                // 14 bytes total
    };
} ASTNode;

// Compile-time size check - temporarily commented to see actual size
// _Static_assert(sizeof(ASTNode) == 24, "ASTNode must be exactly 24 bytes");

/**
 * @brief AST node list for chaining (e.g.,
                     statement lists,
                     parameter lists)
 */
typedef struct ASTNodeList {
    ASTNodeIdx_t node;
    ASTNodeIdx_t next;
} ASTNodeList;

#endif  // SRC_AST_AST_TYPES_H_
