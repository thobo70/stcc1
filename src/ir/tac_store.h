/**
 * @file tac_store.h
 * @brief TAC instruction file-backed storage system
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.1
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#ifndef SRC_IR_TAC_STORE_H_
#define SRC_IR_TAC_STORE_H_

#include "tac_types.h"
#include <stdio.h>

/**
 * @brief TAC instruction store - file-backed like other stores
 */
typedef struct TACStore {
    FILE* fp_tac;            // TAC instruction file
    TACIdx_t current_idx;    // Current instruction index
    TACIdx_t max_instructions; // Maximum instructions
    char filename[256];      // TAC file name
} TACStore;

// TAC store API (similar to astore/sstore/tstore)
int tacstore_init(const char* filename);
void tacstore_close(void);
TACIdx_t tacstore_add(const TACInstruction* instr);
TACInstruction tacstore_get(TACIdx_t idx);
TACIdx_t tacstore_update(TACIdx_t idx, const TACInstruction* instr);
TACIdx_t tacstore_getidx(void);
void tacstore_rewind(void);

// Debug and utility functions
void tacstore_print_stats(void);
int tacstore_validate(void);

#endif  // SRC_IR_TAC_STORE_H_
