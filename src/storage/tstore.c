/**
 * @file tstore.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Token store implementation file
 * @version 0.1
 * @date 2024-09-08
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <stdio.h>

#include "tstore.h"

FILE *fptoken = NULL;
const char *tokenfile = NULL;

int tstore_init(const char *filename) {
  tokenfile = filename;
  fptoken = fopen(tokenfile, "wb");
  if (fptoken == NULL) {
    perror(tokenfile);
    return 1;  // Indicate failure
  }
  return 0;  // Indicate success
}

int tstore_open(const char *filename) {
  tokenfile = filename;
  fptoken = fopen(tokenfile, "rb");
  if (fptoken == NULL) {
    perror(tokenfile);
    return 1;  // Indicate failure
  }
  return 0;  // Indicate success
}

void tstore_close(void) {
  if (fptoken != NULL) {
    fseek(fptoken, 0, SEEK_END);
    printf("tokens: %ld\n", ftell(fptoken) / sizeof(Token_t));
    fclose(fptoken);
    fptoken = NULL;
  }
}

TokenIdx_t tstore_add(Token_t *token) {
  if (fptoken == NULL) {
    return 1;  // Indicate failure
  }
  fseek(fptoken, 0, SEEK_END);
  TokenIdx_t idx = ftell(fptoken) / sizeof(Token_t);
  if (fwrite(token, sizeof(Token_t), 1, fptoken) != 1) {
    perror(tokenfile);
    return 1;  // Indicate failure
  }
  return idx;  // Indicate success by returning the index
}

Token_t tstore_get(TokenIdx_t idx) {
  Token_t token = {T_EOF, 0, 0, 0};  // Default to EOF token
  if (fptoken == NULL) {
    return token;
  }
  fseek(fptoken, idx * sizeof(Token_t), SEEK_SET);
  if (ferror(fptoken)) {
    perror(tokenfile);
    return token;
  }

  // Check if we're at EOF before trying to read
  if (feof(fptoken)) {
    return token;  // Return EOF token
  }

  size_t read_count = fread(&token, sizeof(Token_t), 1, fptoken);
  if (read_count != 1) {
    // If we couldn't read a complete token, return EOF
    token.id = T_EOF;
    token.pos = 0;
    token.file = 0;
    token.line = 0;
  }

  return token;
}

Token_t tstore_next(void) {
  Token_t token = {T_EOF, 0, 0, 0};  // Default to EOF token
  if (fptoken == NULL) {
    return token;
  }
  if (ferror(fptoken)) {
    perror(tokenfile);
    return token;
  }

  // Check if we're at EOF before trying to read
  if (feof(fptoken)) {
    return token;  // Return EOF token
  }

  size_t read_count = fread(&token, sizeof(Token_t), 1, fptoken);
  if (read_count != 1) {
    // If we couldn't read a complete token, return EOF
    token.id = T_EOF;
    token.pos = 0;
    token.file = 0;
    token.line = 0;
  }

  return token;
}

int tstore_setidx(TokenIdx_t idx) {
  if (fptoken == NULL) {
    return 1;  // Indicate failure
  }
  fseek(fptoken, idx * sizeof(Token_t), SEEK_SET);
  return 0;  // Indicate success
}

TokenIdx_t tstore_getidx(void) {
  if (fptoken == NULL) {
    return 1;  // Indicate failure
  }
  return ftell(fptoken) / sizeof(Token_t);
}
