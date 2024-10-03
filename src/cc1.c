/**
 * @file cc1.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-09-01
 * 
 * @copyright Copyright (c) 2024
 * 
 * @details This file contains the main function for processing AST nodes using
 * the abstract syntax tree store (astore) module.
 * 
 * @note This program reads and writes AST nodes using the astore module.
 * 
 */
#include <stdio.h>

#include "sstore.h"
#include "tstore.h"
#include "hmapbuf.h"

ASTNodeIdx_t astnode = 0;


void printToken(Token_t token) {
  fprintf(stderr, "%s[%u]: ", sstore_get(token.file), token.line);
  fprintf(stderr, "%s\n", sstore_get(token.pos));
}


ASTNodeIdx_t parse_function(TokenIdx_t tidx) {
  tstore_setidx(tidx);
  Token_t token = tstore_next();
  if (token.id != T_INT) {
    return 0;
  }
  HBNode *node = HBNew(HBMODE_AST);
  node->ast.type = AST_FUNCTION;
  node->ast.tid = tidx;
  return node->idx;
}

ASTNodeIdx_t parse_typedef(TokenIdx_t tidx) {
  tstore_setidx(tidx);
  Token_t token = tstore_next();
  if (token.id != T_TYPEDEF) {
    return 0;
  }
  HBNode *node = HBNew(HBMODE_AST);
  node->ast.type = AST_TYPEDEF;
  node->ast.tid = tidx;
  return node->idx;
}

ASTNodeIdx_t parse_declaration(TokenIdx_t tidx) {
  tstore_setidx(tidx);
  Token_t token = tstore_next();
  if (token.id != T_TYPEDEF) {
    return 0;
  }
  HBNode *node = HBNew(HBMODE_AST);
  node->ast.type = AST_DECLARATION;
  node->ast.tid = tidx;
  return node->idx;
}

ASTNodeIdx_t parse_statement(TokenIdx_t tidx) {
  tstore_setidx(tidx);
  Token_t token = tstore_next();
  HBNode *node = HBNew(HBMODE_AST);
  node->ast.type = AST_STATEMENT;
  node->ast.tid = tidx;
  return node->idx;
}

ASTNodeIdx_t parse_eof(TokenIdx_t tidx) {
  tstore_setidx(tidx);
  Token_t token = tstore_next();
  if (token.id == T_EOF) {
    HBNode *node = HBNew(HBMODE_AST);
    node->ast.type = AST_EOF;
    node->ast.tid = tidx;
    return node->idx;
  }
  return 0;
}

ASTNodeIdx_t parse_program(void) {
  HBNode *node = HBNew(HBMODE_AST);
  node->ast.type = AST_PROGRAM;
  do {
    TokenIdx_t tidx = tstore_getidx();
    ASTNodeIdx_t nidx = 0;
    if ((nidx = parse_eof(tidx)) > 0) {
      node->ast.o2 = nidx;
      break;
    }
    if ((nidx = parse_function(tidx)) > 0) {
      node->ast.o2 = nidx;
    } else if ((nidx = parse_typedef(tidx)) > 0) {
      node->ast.o2 = nidx;
    } else if ((nidx = parse_declaration(tidx)) > 0) {
      node->ast.o2 = nidx;
    } else {
      fprintf(stderr, "Error: unexpected token\n");
      tstore_setidx(tidx);
      printToken(tstore_next());
      break;;
    }
    HBNode *node2 = HBNew(HBMODE_AST);
    node2->ast.type = AST_PROGRAM;
    node->ast.o1 = node2->idx;
    node = node2;
  } while (1);
  node->ast.o1 = 0;
  return node->idx;
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    fprintf(stderr, "Usage: %s <sstorefile> <tokenfile> <astfile> <symfile>\n", argv[0]);
    return 1;
  }
  if (sstore_open(argv[1]) != 0) {
    fprintf(stderr, "Error: Cannot open sstorefile %s\n", argv[1]);
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
  HBInit();

  parse_program();

  HBEnd();
  symtab_close();
  astore_close();
  tstore_close();
  sstore_close();

  return 0;
}
