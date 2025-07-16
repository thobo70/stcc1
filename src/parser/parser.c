



#include "scanner.h"

typedef struct struct_declaration {
  char *name;
  struct struct_declaration *next;
} StructDeclaration_t;

typedef struct typedef_declaration {
  char *name;
  struct typedef_declaration *next;
} TypedefDeclaration_t;

typedef enum DataType_t {
  T_INT,
  T_FLOAT,
  T_CHAR,
  T_VOID,
  T_STRUCT,
  T_TYPEDEF
} DataType_t;

typedef struct type {
  DataType_t type;
  char *name;
  StructDeclaration_t *struct_declaration;
  TypedefDeclaration_t *typedef_declaration;
} Type_t;

typedef struct variable {
  char *name;
  Type_t *type;
  int value;
} Variable_t;


typedef enum SymbolType_t {
  S_VARIABLE,
  S_FUNCTION,
  S_STRUCT,
  S_UNION,
  S_POINTER,
  S_TYPEDEF,
  S_LABEL
} SymbolType_t;

typedef struct symbol {
  char *name;
  SymbolType_t type;
  int value;
} Symbol_t;

// Abstract Syntax Tree (AST) Node
typedef struct ast_node {
  struct ast_node *left;
  struct ast_node *right;
  Token_t *token;
} ASTNode_t;



int parse(FILE *fp) {
  Token_t *token = nextToken(fp);
  while (token->type->id != T_EOF) {
    printf("Token: %s, Lexeme: %s, Line: %d\n", token->type->name, token->lexeme, token->line);
    token = nextToken(fp);
  }
  return 0;
}