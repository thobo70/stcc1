/**
 * @file ctoken.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef CTOKEN_H  // NOLINT
#define CTOKEN_H

#include "../storage/sstore.h"

// Token IDs
typedef enum TokenID_t {
  T_EOF,
  T_INT,
  T_LONG,
  T_SHORT,
  T_FLOAT,
  T_DOUBLE,
  T_CHAR,
  T_VOID,
  T_RETURN,
  T_IF,
  T_ELSE,
  T_WHILE,
  T_FOR,
  T_DO,
  T_SWITCH,
  T_CASE,
  T_DEFAULT,
  T_BREAK,
  T_CONTINUE,
  T_GOTO,
  T_SIZEOF,
  T_TYPEDEF,
  T_EXTERN,
  T_STATIC,
  T_AUTO,
  T_REGISTER,
  T_CONST,
  T_VOLATILE,
  T_SIGNED,
  T_UNSIGNED,
  T_STRUCT,
  T_UNION,
  T_ENUM,
  T_PLUS,
  T_MINUS,
  T_ASSIGN,
  T_EQ,
  T_NEQ,
  T_LTE,
  T_GTE,
  T_LOGAND,
  T_LOGOR,
  T_NOT,
  T_LPAREN,
  T_RPAREN,
  T_LBRACE,
  T_RBRACE,
  T_LBRACKET,
  T_RBRACKET,
  T_COMMA,
  T_SEMICOLON,
  T_COLON,
  T_ID,
  T_ERROR,
  T_INC,
  T_DEC,
  T_MUL,
  T_DIV,
  T_MOD,
  T_LT,
  T_GT,
  T_AMPERSAND,
  T_PIPE,
  T_CARET,
  T_TILDE,
  T_EXCLAMATION,
  T_QUESTION,
  T_DOT,
  T_ARROW,
  T_LSHIFT,
  T_RSHIFT,
  T_ANDEQ,
  T_OREQ,
  T_XOREQ,
  T_PLUSEQ,
  T_MINUSEQ,
  T_MULEQ,
  T_DIVEQ,
  T_MODEQ,
  T_LSHIFTEQ,
  T_RSHIFTEQ,
  T_LITSTRING,
  T_LITCHAR,
  T_LITINT,
  T_LITFLOAT,
  T_ELLIPSIS,
  T_UNKNOWN
} TokenID_t;

typedef struct Token_t {
  TokenID_t id;
  sstore_pos_t pos;
  sstore_pos_t file;
  unsigned int line;
} Token_t;

typedef unsigned int TokenIdx_t;


#endif  // CTOKEN_H  // NOLINT
