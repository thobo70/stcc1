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

ASTNodeIdx_t astore_add(ASTNode *node) {
    if (fpast == NULL) {
        return 0;  // Indicate failure
    }
    ASTNode empty = {0};  // Initialize empty node with default values
    if (node == NULL) {
        node = &empty;
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
    ASTNodeIdx_t idx = pos / sizeof(ASTNode);
    if (fwrite(node, sizeof(ASTNode), 1, fpast) != 1) {
        perror(astfile);
        return 0;  // Indicate failure
    }
    fflush(fpast);  // Ensure data is written to disk
    return idx;  // Indicate success by returning the ID
}

ASTNodeIdx_t astore_update(ASTNodeIdx_t idx, ASTNode *node) {
    if (fpast == NULL) {
        return 0;  // Indicate failure
    }
    if (fseek(fpast, idx * sizeof(ASTNode), SEEK_SET) != 0) {
        perror(astfile);
        return 0;  // Indicate failure
    }
    if (fwrite(node, sizeof(ASTNode), 1, fpast) != 1) {
        perror(astfile);
        return 0;  // Indicate failure
    }
    return idx;  // Indicate success by returning the ID
}

ASTNode astore_get(ASTNodeIdx_t idx) {
    ASTNode node = {0};  // Initialize node with default values
    if (fpast == NULL) {
        return node;
    }
    if (fseek(fpast, idx * sizeof(ASTNode), SEEK_SET) != 0) {
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
