/**
 * @file hmapbuf.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Hash Map Buffer - LRU Cache for AST nodes and Symbol Table entries
 * @version 0.1
 * @date 2024-09-29
 *
 * @copyright Copyright (c) 2024
 * 
 * HMAPBUF INTERFACE DESIGN DISCOVERIES:
 * 
 * CORE ARCHITECTURE:
 * - Dual-linked list system: Hash table (hnext/hprev) + LRU list (lnext/lprev)
 * - Fixed pool of HBNNODES (100) nodes shared between AST and Symbol storage
 * - Write-back caching with lazy loading from persistent storage
 * - Hash table with chaining for O(1) average lookup (8 buckets, HMAP_SIZE=8)
 * 
 * CRITICAL INVARIANTS DISCOVERED:
 * 1. node->idx is IMMUTABLE after creation - never modify manually!
 * 2. lnext/lprev are NEVER NULL for active nodes (circular lists)
 * 3. Storage systems (astore/symtab) MUST be initialized before use
 * 4. HBGet() ALWAYS returns a node object (never NULL)
 * 5. Index 0 is invalid but handled gracefully (returns empty data)
 * 
 * LRU CACHE BEHAVIOR:
 * - hbfree: Head of free list (unused nodes)
 * - hblast: Most recently used node (head of LRU list) 
 * - hblast->lprev: Least recently used node (LRU victim)
 * - HBTouched() moves node to hblast position
 * 
 * MEMORY MANAGEMENT:
 * - Nodes cycle: free -> LRU -> victim -> reused
 * - HBNew() allocates new storage indices via HBGetIdx()
 * - HBGet() handles cache misses via HBLoad()
 * - Modified nodes marked with HBMODE_MODIFIED flag
 * 
 * STORAGE INTEGRATION:
 * - HBMODE_AST: Uses astore for ASTNode persistence
 * - HBMODE_SYM: Uses symtab for SymTabEntry persistence  
 * - HBGetIdx() creates default entries (AST_FREE nodes, empty symbols)
 * - Storage uses 1-based indexing (index 0 = invalid)
 */
#ifndef HMAPBUF_H  // NOLINT
#define HMAPBUF_H

#include "../storage/symtab.h"
#include "../storage/astore.h"

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
HBNode *HBGet(HMapIdx_t idx,
                     HBMode_t mode);

// Additional function prototypes
void HBStore(HBNode *node);
void HBLoad(HBNode *node);
HMapIdx_t HBGetIdx(HBMode_t mode);
HBNode *HBFind(HMapIdx_t idx,
                     HBMode_t mode);
void HBAdd(HBNode *node);
void HBRemove(HBNode *node);
HBNode *HBEmpty(void);
void HBDelete(HBNode *node);

#endif  // HMAPBUF_H // NOLINT
