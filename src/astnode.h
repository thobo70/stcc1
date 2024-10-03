/**
 * @file astnode.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-09-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef _ASTNODE_H  // NOLINT
#define _ASTNODE_H

#include "sstore.h"
#include "ctoken.h"

typedef enum {
    AST_FREE,
    AST_PROGRAM,
    AST_FUNCTION,
    AST_TYPEDEF,
    AST_DECLARATION,
    AST_STATEMENT,
    AST_EXPRESSION,
    AST_LITERAL,
    AST_IDENTIFIER,
    AST_OPERATOR,
    AST_ASSIGNMENT,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_DO,
    AST_SWITCH,
    AST_CASE,
    AST_DEFAULT,
    AST_BREAK,
    AST_CONTINUE,
    AST_GOTO,
    AST_RETURN,
    AST_SIZEOF,
    AST_CALL,
    AST_CAST,
    AST_INDEX,
    AST_MEMBER,
    AST_POINTER,
    AST_DEREFERENCE,
    AST_ADDRESS,
    AST_LOGICAL,
    AST_CONDITIONAL,
    AST_COMMA,
    AST_BLOCK,
    AST_LABEL,
    AST_GOTO_LABEL,
    AST_EMPTY,
    AST_EOF
    // Add more node types as needed
} ASTNodeType;

typedef unsigned short ASTNodeIdx_t;  // NOLINT
typedef unsigned long MAXusigint_t;  // NOLINT
typedef long MAXsigint_t;  // NOLINT

typedef struct ASTNode {
    ASTNodeType type;
    TokenID_t tid;
    union {
        MAXusigint_t uvalue;
        MAXsigint_t svalue;
        ASTNodeIdx_t o1, o2, o3, o4;
    };
} ASTNode;

#endif  // _ASTNODE_H  // NOLINT
