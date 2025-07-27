/**
 * @file astore.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Implementation of the abstract syntax tree store.
 * @version 0.1
 * @date 2024-09-08
 *
 * @copyright Copyright (c) 2024
 *
 * ASTMemnode is a structure that holds an ASTNode and additional information
 * for the abstract syntax tree store.
 *
 */

#include <stdio.h>

#include "astore.h"

static FILE *fpast = NULL;
static const char *astfile = NULL;

ASTNodeIdx_t astlastidx = 0;



int astore_init(const char *filename) {
    astfile = filename;
    fpast = fopen(astfile, "w+b");
    if (fpast == NULL) {
        perror(astfile);
        return 1;  // Indicate failure
    }
    return 0;  // Indicate success
}



int astore_open(const char *filename) {
    astfile = filename;
    fpast = fopen(astfile, "rb+");
    if (fpast == NULL) {
        perror(astfile);
        return 1;  // Indicate failure
    }
    if (fseek(fpast, 0, SEEK_END) != 0) {
        perror(astfile);
        fclose(fpast);
        fpast = NULL;
        return 1;  // Indicate failure
    }
    long pos = ftell(fpast);
    if (pos < 0) {
        perror(astfile);
        fclose(fpast);
        fpast = NULL;
        return 1;  // Indicate failure
    }
    astlastidx = pos / sizeof(ASTNode);
    return 0;  // Indicate success
}

void astore_close(void) {
    if (fpast != NULL) {
        fseek(fpast, 0, SEEK_END);
        printf("nodes: %ld\n", ftell(fpast) / sizeof(ASTNode));
        fclose(fpast);
        fpast = NULL;
    }
}

/**
 * @brief Add a new AST node to persistent storage
 * 
 * Appends the AST node to the end of the storage file and returns its 1-based index.
 * This is the primary allocation interface for the hmapbuf cache system.
 * 
 * @param node Pointer to ASTNode to store (cannot be NULL)
 * @return 1-based index of stored node, or 0 on failure
 * 
 * @note Discovery: Uses 1-based indexing - index 0 is reserved for invalid/error
 * @note Discovery: Returns 0 for NULL input (graceful failure handling)
 * @note Discovery: Critical for HBGetIdx() in hmapbuf - must accept valid default nodes
 * @note Discovery: Always appends to end of file for sequential allocation
 * @note Discovery: Flushes immediately to ensure persistence
 */
ASTNodeIdx_t astore_add(ASTNode *node) {
    if (fpast == NULL) {
        return 0;  // Indicate failure
    }
    if (node == NULL) {
        return 0;  // Fail gracefully for NULL input
    }
    if (fseek(fpast, 0, SEEK_END) != 0) {
        perror(astfile);
        return 0;  // Indicate failure
    }
    long pos = ftell(fpast);
    if (pos < 0) {
        perror(astfile);
        return 0;  // Indicate failure
    }
    // Use 1-based indexing (index 0 reserved for invalid/error)
    ASTNodeIdx_t idx = (pos / sizeof(ASTNode)) + 1;
    if (fwrite(node, sizeof(ASTNode), 1, fpast) != 1) {
        perror(astfile);
        return 0;  // Indicate failure
    }
    fflush(fpast);  // Ensure data is written to disk
    return idx;  // Indicate success by returning the ID
}

/**
 * @brief Update an existing AST node in storage
 * 
 * Overwrites the AST node at the specified index with new data.
 * Used by hmapbuf's write-back cache mechanism (HBStore).
 * 
 * @param idx 1-based index of node to update (0 is invalid)
 * @param node Pointer to new ASTNode data
 * @return The index on success, 0 on failure
 * 
 * @note Discovery: Validates idx != 0 (1-based indexing)
 * @note Discovery: Converts to 0-based file positioning internally
 * @note Discovery: Critical for cache write-back in HBMODE_MODIFIED nodes
 */
ASTNodeIdx_t astore_update(ASTNodeIdx_t idx, ASTNode *node) {
    if (fpast == NULL || idx == 0) {  // idx == 0 is invalid (1-based indexing)
        return 0;  // Indicate failure
    }
    // Convert from 1-based to 0-based indexing for file positioning
    if (fseek(fpast, (idx - 1) * sizeof(ASTNode), SEEK_SET) != 0) {
        perror(astfile);
        return 0;  // Indicate failure
    }
    if (fwrite(node, sizeof(ASTNode), 1, fpast) != 1) {
        perror(astfile);
        return 0;  // Indicate failure
    }
    return idx;  // Indicate success by returning the ID
}

/**
 * @brief Retrieve an AST node from storage by index
 * 
 * Loads an AST node from the specified 1-based index. Returns a default
 * zero-initialized node for invalid indices or errors.
 * 
 * @param idx 1-based index of node to retrieve (0 is invalid)
 * @return ASTNode structure (zero-initialized if invalid/error)
 * 
 * @note Discovery: ALWAYS returns a valid ASTNode structure (never fails)
 * @note Discovery: Index 0 returns default/empty node (graceful handling)
 * @note Discovery: Critical for HBLoad() cache-miss handling
 * @note Discovery: Zero-initialized node has type=0, which is safe default
 * @note Discovery: Converts to 0-based file positioning internally
 */
ASTNode astore_get(ASTNodeIdx_t idx) {
    ASTNode node = {0};  // Initialize node with default values
    if (fpast == NULL || idx == 0) {  // idx == 0 is invalid (1-based indexing)
        return node;
    }
    // Convert from 1-based to 0-based indexing for file positioning
    if (fseek(fpast, (idx - 1) * sizeof(ASTNode), SEEK_SET) != 0) {
        perror(astfile);
        return node;  // Return empty node on error
    }
    if (fread(&node, sizeof(ASTNode), 1, fpast) != 1) {
        perror(astfile);
    }
    return node;
}

int astore_setidx(ASTNodeIdx_t idx) {
    if (fpast == NULL) {
        return 1;  // Indicate failure
    }
    fseek(fpast, idx * sizeof(ASTNode), SEEK_SET);
    return 0;  // Indicate success
}

ASTNodeIdx_t astore_getidx(void) {
    if (fpast == NULL) {
        return 0;  // Indicate failure
    }
    return ftell(fpast) / sizeof(ASTNode);
}
