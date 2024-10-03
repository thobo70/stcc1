#ifndef SSTORE_H
#define SSTORE_H

#include "hash.h"

// Error code for sstore instead of pos
#define SSTORE_ERR 0xFFFF

typedef unsigned short sstore_pos_t;
typedef unsigned short sstore_len_t;

typedef struct {
  hash_t hash;
  sstore_pos_t pos;
} sstore_entry_t;

int sstore_init(const char *fname);
sstore_pos_t sstore_str(const char *str, sstore_len_t length);

int sstore_open(const char *fname);
char *sstore_get(sstore_pos_t pos);

void sstore_close();



#endif  // SSTORE_H
