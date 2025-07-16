/**
 * @file symtab.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-10
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef SYMTAB_H  // NOLINT
#define SYMTAB_H

#include "sstore.h"

typedef enum {
    SYM_FREE,
    SYM_VARIABLE,
    SYM_FUNCTION,
    SYM_TYPEDEF,
    SYM_LABEL,
    SYM_ENUMERATOR,
    SYM_STRUCT,
    SYM_UNION,
    SYM_ENUM,
    SYM_CONSTANT,
    SYM_UNKOWN
    // Add more symbol types as needed
} SymType;

typedef unsigned short SymIdx_t;    // NOLINT

typedef struct SymTabEntry {
    SymType type;
    sstore_pos_t name;
    SymIdx_t parent;
    SymIdx_t next;
    SymIdx_t prev;
    SymIdx_t child;
    SymIdx_t sibling;
    sstore_pos_t value;
    int line;
} SymTabEntry;



int symtab_init(const char *filename);
int symtab_open(const char *filename);
void symtab_close(void);
SymIdx_t symtab_add(SymTabEntry *entry);
SymIdx_t symtab_update(SymIdx_t idx, SymTabEntry *entry);
SymTabEntry symtab_get(SymIdx_t idx);
SymIdx_t symtab_get_count(void);


#endif  // SYMTAB_H  // NOLINT