/**
 * @file sstore.h
 * @brief String store interface for the STCC1 compiler
 * @author Thomas Boos (tboos70@gmail.com)
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#ifndef SRC_STORAGE_SSTORE_H_
#define SRC_STORAGE_SSTORE_H_

#include <stdint.h>
#include "../utils/hash.h"

// Error code for sstore instead of pos
#define SSTORE_ERR 0xFFFF

typedef uint16_t sstore_pos_t;
typedef uint16_t sstore_len_t;

typedef struct {
  hash_t hash;
  sstore_pos_t pos;
} sstore_entry_t;

int sstore_init(const char *fname);
sstore_pos_t sstore_str(const char *str,
                     sstore_len_t length);

int sstore_open(const char *fname);
char *sstore_get(sstore_pos_t pos);

void sstore_close(void);



#endif  // SRC_STORAGE_SSTORE_H_
