/**
 * @file cc0t.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief test program for the sstore and tstore modules, reproduces the input of cc0
 * @version 0.1
 * @date 2024-09-01
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <stdio.h>

#include "sstore.h"
#include "tstore.h"

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <sstorefile> <tokenfile>\n", argv[0]);
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

  Token_t token;
  unsigned int line = 1;
  sstore_pos_t currfilepos = 0;
  while ((token = tstore_next()).id != T_EOF) {
    if (line + 5 < token.line || token.file != currfilepos) {
      currfilepos = token.file;
      line = token.line;
      printf("\n# %u \"%s\"\n", token.line - 1, sstore_get(currfilepos));
    }
    while (token.line > line) {
      printf("\n");
      line++;
    }
    if (token.id == T_LITSTRING) {
      printf("\"%s\" ", sstore_get(token.pos));
    } else if (token.id == T_LITCHAR) {
      printf("'%s' ", sstore_get(token.pos));
    } else {
      printf("%s ", sstore_get(token.pos));
    }
  }
  printf("\n");

  tstore_close();
  sstore_close();

  return 0;
}
