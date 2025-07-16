/**
 * @file tac_store.c
 * @brief TAC instruction file-backed storage implementation
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.1
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#include "tac_store.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Global TAC store instance
static TACStore g_tacstore = {NULL, 0, 0, ""};

/**
 * @brief Initialize TAC store with file-backed storage
 */
int tacstore_init(const char* filename) {
    if (g_tacstore.fp_tac != NULL) {
        tacstore_close();
    }
    
    strncpy(g_tacstore.filename, filename, sizeof(g_tacstore.filename) - 1);
    g_tacstore.filename[sizeof(g_tacstore.filename) - 1] = '\0';
    
    g_tacstore.fp_tac = fopen(filename, "w+b");
    if (g_tacstore.fp_tac == NULL) {
        perror("tacstore_init: Cannot create TAC file");
        return 0;
    }
    
    g_tacstore.current_idx = 0;
    g_tacstore.max_instructions = 65535;  // 16-bit index limit
    
    return 1;
}

/**
 * @brief Open existing TAC store file for reading
 */
int tacstore_open(const char* filename) {
    if (g_tacstore.fp_tac != NULL) {
        tacstore_close();
    }
    
    strncpy(g_tacstore.filename, filename, sizeof(g_tacstore.filename) - 1);
    g_tacstore.filename[sizeof(g_tacstore.filename) - 1] = '\0';
    
    g_tacstore.fp_tac = fopen(filename, "rb");
    if (g_tacstore.fp_tac == NULL) {
        perror("tacstore_open: Cannot open TAC file");
        return 0;
    }
    
    // Determine file size and number of instructions
    fseek(g_tacstore.fp_tac, 0, SEEK_END);
    long file_size = ftell(g_tacstore.fp_tac);
    rewind(g_tacstore.fp_tac);
    
    g_tacstore.current_idx = (TACIdx_t)(file_size / sizeof(TACInstruction));
    g_tacstore.max_instructions = 65535;  // 16-bit index limit
    
    return 1;
}

/**
 * @brief Close TAC store and cleanup
 */
void tacstore_close(void) {
    if (g_tacstore.fp_tac != NULL) {
        fclose(g_tacstore.fp_tac);
        g_tacstore.fp_tac = NULL;
    }
    
    g_tacstore.current_idx = 0;
    g_tacstore.max_instructions = 0;
    g_tacstore.filename[0] = '\0';
}

/**
 * @brief Add a TAC instruction to storage
 */
TACIdx_t tacstore_add(const TACInstruction* instr) {
    if (g_tacstore.fp_tac == NULL) {
        return 0;  // Store not initialized
    }
    
    if (instr == NULL) {
        return 0;  // Invalid instruction
    }
    
    if (g_tacstore.current_idx >= g_tacstore.max_instructions) {
        return 0;  // Store full
    }
    
    // Move to end of file
    if (fseek(g_tacstore.fp_tac, 0, SEEK_END) != 0) {
        perror("tacstore_add: fseek failed");
        return 0;
    }
    
    // Write instruction
    if (fwrite(instr, sizeof(TACInstruction), 1, g_tacstore.fp_tac) != 1) {
        perror("tacstore_add: fwrite failed");
        return 0;
    }
    
    // Flush to ensure data is written
    fflush(g_tacstore.fp_tac);
    
    g_tacstore.current_idx++;
    return g_tacstore.current_idx;  // Return 1-based index
}

/**
 * @brief Get a TAC instruction by index
 */
TACInstruction tacstore_get(TACIdx_t idx) {
    TACInstruction instr = {TAC_NOP, TAC_FLAG_NONE, 
                           TAC_OPERAND_NONE, TAC_OPERAND_NONE, TAC_OPERAND_NONE};
    
    if (g_tacstore.fp_tac == NULL) {
        return instr;  // Store not initialized
    }
    
    if (idx == 0 || idx > g_tacstore.current_idx) {
        return instr;  // Invalid index
    }
    
    // Calculate file position (1-based index to 0-based file position)
    long pos = (long)(idx - 1) * sizeof(TACInstruction);
    
    if (fseek(g_tacstore.fp_tac, pos, SEEK_SET) != 0) {
        perror("tacstore_get: fseek failed");
        return instr;
    }
    
    if (fread(&instr, sizeof(TACInstruction), 1, g_tacstore.fp_tac) != 1) {
        perror("tacstore_get: fread failed");
        // Return NOP instruction on error
        instr.opcode = TAC_NOP;
        instr.flags = TAC_FLAG_NONE;
    }
    
    return instr;
}

/**
 * @brief Update a TAC instruction at given index
 */
TACIdx_t tacstore_update(TACIdx_t idx, const TACInstruction* instr) {
    if (g_tacstore.fp_tac == NULL) {
        return 0;  // Store not initialized
    }
    
    if (instr == NULL) {
        return 0;  // Invalid instruction
    }
    
    if (idx == 0 || idx > g_tacstore.current_idx) {
        return 0;  // Invalid index
    }
    
    // Calculate file position (1-based index to 0-based file position)
    long pos = (long)(idx - 1) * sizeof(TACInstruction);
    
    if (fseek(g_tacstore.fp_tac, pos, SEEK_SET) != 0) {
        perror("tacstore_update: fseek failed");
        return 0;
    }
    
    if (fwrite(instr, sizeof(TACInstruction), 1, g_tacstore.fp_tac) != 1) {
        perror("tacstore_update: fwrite failed");
        return 0;
    }
    
    fflush(g_tacstore.fp_tac);
    return idx;
}

/**
 * @brief Get current TAC instruction index
 */
TACIdx_t tacstore_getidx(void) {
    return g_tacstore.current_idx;
}

/**
 * @brief Rewind TAC store to beginning
 */
void tacstore_rewind(void) {
    if (g_tacstore.fp_tac != NULL) {
        rewind(g_tacstore.fp_tac);
    }
}

/**
 * @brief Print TAC store statistics
 */
void tacstore_print_stats(void) {
    printf("TAC Store Statistics:\n");
    printf("  File: %s\n", g_tacstore.filename);
    printf("  Instructions: %d / %d\n", g_tacstore.current_idx, g_tacstore.max_instructions);
    printf("  Memory usage: %zu bytes\n", 
           (size_t)g_tacstore.current_idx * sizeof(TACInstruction));
    
    if (g_tacstore.fp_tac != NULL) {
        long pos = ftell(g_tacstore.fp_tac);
        printf("  File position: %ld\n", pos);
        printf("  File size: %zu bytes\n", 
               (size_t)g_tacstore.current_idx * sizeof(TACInstruction));
    }
}

/**
 * @brief Validate TAC store integrity
 */
int tacstore_validate(void) {
    if (g_tacstore.fp_tac == NULL) {
        printf("TAC store validation: Store not initialized\n");
        return 0;
    }
    
    // Check file size consistency
    fseek(g_tacstore.fp_tac, 0, SEEK_END);
    long file_size = ftell(g_tacstore.fp_tac);
    size_t expected_size = (size_t)g_tacstore.current_idx * sizeof(TACInstruction);
    
    if (file_size != (long)expected_size) {
        printf("TAC store validation: File size mismatch (got %ld, expected %zu)\n",
               file_size, expected_size);
        return 0;
    }
    
    // Basic instruction validation
    int valid_count = 0;
    for (TACIdx_t i = 1; i <= g_tacstore.current_idx; i++) {
        TACInstruction instr = tacstore_get(i);
        if (instr.opcode < TAC_PHI + 1) {  // Valid opcode range
            valid_count++;
        }
    }
    
    printf("TAC store validation: %d/%d instructions valid\n", 
           valid_count, g_tacstore.current_idx);
    
    return (valid_count == g_tacstore.current_idx);
}
