/**
 * @file symtab.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-09-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <stdio.h>

#include "symtab.h"

static FILE *fpsym = NULL;
static const char *symfile = NULL;

int symtab_init(const char *filename) {
  symfile = filename;
  fpsym = fopen(symfile, "wb");
  if (fpsym == NULL) {
    perror(symfile);
    return 1;  // Indicate failure
  }
  return 0;
}



int symtab_open(const char *filename) {
  symfile = filename;
  fpsym = fopen(symfile, "rb+");
  if (fpsym == NULL) {
    perror(symfile);
    return 1;  // Indicate failure
  }
  return 0;  // Indicate success
}



void symtab_close(void) {
  if (fpsym != NULL) {
    fseek(fpsym, 0, SEEK_END);
    printf("symbols: %ld\n", ftell(fpsym) / sizeof(SymTabEntry));
    fclose(fpsym);
    fpsym = NULL;
  }
}



SymIdx_t symtab_add(SymTabEntry *entry) {
  if (fpsym == NULL) {
    return 0;  // Indicate failure
  }
  SymTabEntry empty = {0};  // Initialize empty entry with default values
  if (entry == NULL) {
    entry = &empty;
  }
  fseek(fpsym, 0, SEEK_END);
  SymIdx_t idx = ftell(fpsym) / sizeof(SymTabEntry);
  if (fwrite(entry, sizeof(SymTabEntry), 1, fpsym) != 1) {
    perror(symfile);
    return 0;  // Indicate failure
  }
  return idx + 1;  // Return 1-based index (0 reserved for failure)
}



SymIdx_t symtab_update(SymIdx_t idx, SymTabEntry *entry) {
  if (fpsym == NULL || idx == 0) {
    return 0;  // Indicate failure
  }
  fseek(fpsym, (idx - 1) * sizeof(SymTabEntry), SEEK_SET);
  if (fwrite(entry, sizeof(SymTabEntry), 1, fpsym) != 1) {
    perror(symfile);
    return 0;  // Indicate failure
  }
  return idx;  // Indicate success by returning the ID
}



SymTabEntry symtab_get(SymIdx_t idx) {
  SymTabEntry entry = {0};  // Initialize entry with default values
  if (fpsym == NULL || idx == 0) {
    return entry;
  }
  fseek(fpsym, (idx - 1) * sizeof(SymTabEntry), SEEK_SET);
  if (fread(&entry, sizeof(SymTabEntry), 1, fpsym) != 1) {
    perror(symfile);
  }
  return entry;
}



