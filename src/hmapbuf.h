/**
 * @file hmapbuf.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-09-29
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef HMAPBUF_H  // NOLINT
#define HMAPBUF_H

#include "symtab.h"
#include "astore.h"

enum {
    HBMODE_UNUSED,
    HBMODE_SYM,
    HBMODE_AST
};

#define HBMODE_MODIFIED 0x8000

#define HBNNODES 100

#define HMAP_SIZE 8
#define HMAP_MASK (HMAP_SIZE - 1)
#define HMAP_IDX(idx) ((idx) & HMAP_MASK)

typedef unsigned short HMapIdx_t;   // NOLINT
typedef unsigned short HBMode_t;    // NOLINT

typedef struct HBNode {
    HMapIdx_t idx;
    HBMode_t mode;
    struct HBNode *hnext;
    struct HBNode *hprev;
    struct HBNode *lnext;
    struct HBNode *lprev;
    union {
        SymTabEntry sym;
        ASTNode ast;
    };
} HBNode;

void HBInit(void);
void HBEnd(void);
HBNode *HBNew(HBMode_t mode);
void HBTouched(HBNode *node);
HBNode *HBGet(HMapIdx_t idx, HBMode_t mode);

#endif  // HMAPBUF_H // NOLINT
