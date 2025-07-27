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
  fpsym = fopen(symfile, "w+b");  // Read/write binary mode
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



/**
 * @brief Add a new symbol table entry to persistent storage
 * 
 * Appends the symbol entry to the end of the storage file and returns its 1-based index.
 * This is the primary allocation interface for symbol storage in hmapbuf.
 * 
 * @param entry Pointer to SymTabEntry to store (cannot be NULL)
 * @return 1-based index of stored entry, or 0 on failure
 * 
 * @note Discovery: Uses 1-based indexing - index 0 is reserved for invalid/error
 * @note Discovery: Returns 0 for NULL input (graceful failure handling)
 * @note Discovery: Critical for HBGetIdx() in hmapbuf HBMODE_SYM mode
 * @note Discovery: Always appends to end of file for sequential allocation
 * @note Discovery: Flushes immediately to ensure persistence
 */
SymIdx_t symtab_add(SymTabEntry *entry) {
  if (fpsym == NULL) {
    return 0;  // Indicate failure
  }
  if (entry == NULL) {
    return 0;  // Fail gracefully for NULL input, consistent with other storage systems
  }
  fseek(fpsym, 0, SEEK_END);
  SymIdx_t idx = ftell(fpsym) / sizeof(SymTabEntry);
  if (fwrite(entry, sizeof(SymTabEntry), 1, fpsym) != 1) {
    perror(symfile);
    return 0;  // Indicate failure
  }
  fflush(fpsym);  // Ensure data is written to disk
  return idx + 1;  // Return 1-based index (0 reserved for failure)
}



/**
 * @brief Update an existing symbol table entry in storage
 * 
 * Overwrites the symbol entry at the specified index with new data.
 * Used by hmapbuf's write-back cache mechanism (HBStore).
 * 
 * @param idx 1-based index of entry to update (0 is invalid)
 * @param entry Pointer to new SymTabEntry data
 * @return The index on success, 0 on failure
 * 
 * @note Discovery: Validates idx != 0 (1-based indexing)
 * @note Discovery: Converts to 0-based file positioning internally
 * @note Discovery: Critical for cache write-back in HBMODE_MODIFIED nodes
 */
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



/**
 * @brief Retrieve a symbol table entry from storage by index
 * 
 * Loads a symbol entry from the specified 1-based index. Returns a default
 * zero-initialized entry for invalid indices or errors.
 * 
 * @param idx 1-based index of entry to retrieve (0 is invalid)
 * @return SymTabEntry structure (zero-initialized if invalid/error)
 * 
 * @note Discovery: ALWAYS returns a valid SymTabEntry structure (never fails)
 * @note Discovery: Index 0 returns default/empty entry (graceful handling)
 * @note Discovery: Critical for HBLoad() cache-miss handling in HBMODE_SYM
 * @note Discovery: Zero-initialized entry is safe default state
 * @note Discovery: Converts to 0-based file positioning internally
 */
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

SymIdx_t symtab_get_count(void) {
  if (fpsym == NULL) {
    return 0;
  }
  long current_pos = ftell(fpsym);
  fseek(fpsym, 0, SEEK_END);
  long end_pos = ftell(fpsym);
  fseek(fpsym, current_pos, SEEK_SET);  // Restore position
  return (SymIdx_t)(end_pos / sizeof(SymTabEntry));
}



