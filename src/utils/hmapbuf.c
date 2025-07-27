/**
 * @file hmapbuf.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-29
 *
 * @copyright Copyright (c) 2024
 *
 * The hash map buffer module provides a simple memory management system for storing
 * symbol table entries and abstract syntax tree nodes. The nodes are stored in a hash
 * table and a linked list ring. The hash table is used to quickly find a node by its ID,
 * while the linked list ring is used to implement a simple memory management system.
 * There are 2 linked list rings, one free list and one least recently used (LRU) list.
 * After initialization, all nodes are in the free list. When a node is accessed, it is
 * moved to the LRU list. When a new node is needed, it is first taken from the free list.
 * If the free list is empty, the least recently used node is taken from the LRU list.
 * Therefore every node is in one of the lists at any time.
 * If a node is released (freed), it is moved back to the free list at hbfree->lprev.
 * The least recently used node is always at hblast->lprev.
 * Once a node is added to the hash table, it will be allways in the hash table,
 * even if it is in the free list. This is necessary to find the node by its ID.
 * If a node is reused, he gets removed from the hash table and added again with the new ID.
 * lnext and lprev are used for the linked list ring, hnext and hprev are used for the hash table.
 * The mode field is used to distinguish between symbol table entries and abstract syntax tree nodes.
 * The idx field is used to store the ID of the node.
 * The union field is used to store the actual data of the node.
 * lnext and lprev are never NULL, hnext and hprev are NULL if the node is not in the hash table.
 */

#include <stdio.h>
#include <string.h>

#include "hmapbuf.h"


static HBNode hbnodes[HBNNODES];
static HBNode *hbhtab[HMAP_SIZE];
static HBNode *hbfree = 0;
static HBNode *hblast = 0;




/**
 * @brief Store a modified node's data to persistent storage
 * 
 * Writes the node's data back to the appropriate storage system (symtab or astore)
 * if the node has been modified. This is part of the write-back caching mechanism.
 * 
 * @param node The node to store (can be NULL - will be ignored)
 * 
 * @note Only stores if node is modified (HBMODE_MODIFIED flag set)
 * @note Clears the HBMODE_MODIFIED flag after storing
 * @note Discovery: Critical for data persistence in LRU cache system
 */
void HBStore(HBNode *node) {
    if (node == NULL) {
        return;     // @todo: error handling
    }
    if (node->mode == HBMODE_UNUSED || !(node->mode & HBMODE_MODIFIED)) {
        return;
    }
    node->mode &= ~HBMODE_MODIFIED;
    switch (node->mode) {
        case HBMODE_SYM:
            symtab_update(node->idx, &node->sym);
            break;
        case HBMODE_AST:
            astore_update(node->idx, &node->ast);
            break;
        default:    // @todo: error handling
            break;
    }
}



/**
 * @brief Load a node's data from persistent storage
 * 
 * Reads the node's data from the appropriate storage system (symtab or astore)
 * based on the node's mode and index. This is part of the lazy loading mechanism.
 * 
 * @param node The node to load data into (can be NULL - will be ignored)
 * 
 * @note Clears the HBMODE_MODIFIED flag after loading
 * @note Discovery: Essential for cache-miss handling in LRU system
 * @note Discovery: Storage systems must be initialized before calling this
 */
void HBLoad(HBNode *node) {
    if (node == NULL) {
        return;     // @todo: error handling
    }
    switch (node->mode) {
        case HBMODE_SYM:
            node->sym = symtab_get(node->idx);
            break;
        case HBMODE_AST:
            node->ast = astore_get(node->idx);
            break;
        default:    // @todo: error handling
            break;
    }
    node->mode &= ~HBMODE_MODIFIED;
}


/**
 * @brief Allocate a new storage index for the specified mode
 * 
 * Creates a default entry in the appropriate storage system and returns its index.
 * This function is crucial for creating new nodes with valid storage backing.
 * 
 * @param mode The mode (HBMODE_SYM or HBMODE_AST) to allocate storage for
 * @return The allocated storage index, or 0 if allocation failed
 * 
 * @note Discovery: Must create valid default entries, not pass NULL to storage systems
 * @note Discovery: For AST mode, creates AST_FREE nodes; for SYM mode, creates empty symbols
 * @note Discovery: Storage systems (astore/symtab) must be initialized before calling
 * @note Discovery: Returns 0 on failure, which is treated as invalid index by callers
 */
HMapIdx_t HBGetIdx(HBMode_t mode) {
    HMapIdx_t idx = 0;
    switch (mode & ~HBMODE_MODIFIED) {
        case HBMODE_SYM:
            {
                // Create a default empty symbol entry
                SymTabEntry default_sym = {0};
                idx = symtab_add(&default_sym);
            }
            break;
        case HBMODE_AST:
            {
                // Create a default free AST node
                ASTNode default_ast = {0};
                default_ast.type = AST_FREE;
                idx = astore_add(&default_ast);
            }
            break;
        default:    // @todo: error handling
            break;
    }
    return idx;
}



void HBInit(void) {
    memset(hbnodes, 0, sizeof(hbnodes));
    memset(hbhtab, 0, sizeof(hbhtab));
    for (int i = 0; i < HBNNODES; i++) {
        hbnodes[i].lnext = (i < HBNNODES - 1) ? &hbnodes[i + 1] : &hbnodes[0];
        hbnodes[i].lprev = (i > 0) ? &hbnodes[i - 1] : &hbnodes[HBNNODES - 1];
    }
    hbfree = &hbnodes[0];
    hblast = NULL;
}



HBNode *HBFind(HMapIdx_t idx, HBMode_t mode) {
    HMapIdx_t hidx = HMAP_IDX(idx);
    HBNode *node = hbhtab[hidx];
    while (node != NULL) {
        if (node->idx == idx && (node->mode & ~HBMODE_MODIFIED) == mode) {
            return node;
        }
        node = node->hnext;
    }
    return NULL;
}



/**
 * @brief Add a node to the hash table for fast index-based lookup
 * 
 * Inserts the node into the hash table bucket determined by HMAP_IDX(node->idx).
 * Uses chaining to handle hash collisions via hnext/hprev pointers.
 * 
 * @param node The node to add to hash table (can be NULL - will be ignored)
 * 
 * @note Discovery: Hash table uses chaining for collision resolution
 * @note Discovery: hnext/hprev are for hash table, lnext/lprev are for LRU list
 * @note Discovery: Node is inserted at head of bucket chain for O(1) insertion
 */
void HBAdd(HBNode *node) {
    if (node == NULL) {
        return;     // @todo: error handling
    }
    HMapIdx_t hidx = HMAP_IDX(node->idx);
    node->hnext = hbhtab[hidx];
    node->hprev = NULL;
    if (node->hnext != NULL)
        node->hnext->hprev = node;
    hbhtab[hidx] = node;
}



/**
 * @brief Remove a node from the hash table
 * 
 * Removes the node from its hash table bucket, updating the chain pointers
 * to maintain the linked list integrity.
 * 
 * @param node The node to remove from hash table (can be NULL - will be ignored)
 * 
 * @note Discovery: Must update bucket head pointer if removing first node
 * @note Discovery: Clears hnext/hprev but preserves lnext/lprev (LRU list intact)
 * @note Discovery: Called when node is being reused with different index
 */
void HBRemove(HBNode *node) {
    if (node == NULL) {
        return;     // @todo: error handling
    }
    HMapIdx_t hidx = HMAP_IDX(node->idx);
    if (hbhtab[hidx] == node) {
        hbhtab[hidx] = node->hnext;
    }
    if (node->hprev != NULL)
        node->hprev->hnext = node->hnext;
    if (node->hnext != NULL)
        node->hnext->hprev = node->hprev;
    node->hnext = NULL;
    node->hprev = NULL;
}



/**
 * @brief Mark a node as recently used and move it to the front of LRU list
 * 
 * This is the core LRU mechanism. When a node is accessed, it's moved to become
 * the most recently used (hblast). Also marks the node as modified.
 * 
 * @param node The node that was accessed (can be NULL - will be ignored)
 * 
 * @note Discovery: CRITICAL - Node must be properly linked (lprev/lnext != NULL)
 * @note Discovery: Segmentation fault occurred when node->lprev was NULL
 * @note Discovery: Node->idx should NEVER be modified after creation - immutable!
 * @note Discovery: Sets HBMODE_MODIFIED flag to track changes
 * @note Discovery: Handles both free list and LRU list node movement
 * @note Discovery: Creates circular linked list when hblast is NULL (first node)
 */
void HBTouched(HBNode *node) {
    if (node == NULL) {
        return;     // @todo: error handling
    }
    
    // Check if node is properly linked before manipulating
    if (node->lprev == NULL || node->lnext == NULL) {
        return;     // Node not properly initialized/linked
    }
    
    // Mark node as modified when touched
    node->mode |= HBMODE_MODIFIED;
    
    if (hblast == node) {
        return;
    }
    if (hbfree == node) {
        hbfree = node->lnext;
        if (hbfree == node) {
            hbfree = NULL;
        }
    }
    node->lprev->lnext = node->lnext;
    node->lnext->lprev = node->lprev;
    if (hblast == NULL) {
        hblast = node;
        node->lnext = node;
        node->lprev = node;
    } else {
        node->lnext = hblast;
        node->lprev = hblast->lprev;
        hblast->lprev->lnext = node;
        hblast->lprev = node;
        hblast = node;
    }
}



void HBEnd(void) {
    if (hbfree != NULL)
        hbfree->lprev->lnext = NULL;
    while (hbfree != NULL) {
        HBStore(hbfree);
        hbfree = hbfree->lnext;
    }
    if (hblast == NULL)
        return;
    hblast->lprev->lnext = NULL;
    while (hblast != NULL) {
        HBStore(hblast);
        hblast = hblast->lnext;
    }
}



/**
 * @brief Create a new node or reuse an existing one for the specified mode
 * 
 * Implements the LRU cache allocation policy. First tries to find a compatible
 * node from the free list, otherwise takes the least recently used node.
 * 
 * @param mode The mode for the new node (HBMODE_SYM or HBMODE_AST)
 * @return Pointer to allocated node (never NULL in normal operation)
 * 
 * @note Discovery: Prefers reusing nodes of the same mode for efficiency
 * @note Discovery: Falls back to any free node if no matching mode found
 * @note Discovery: Takes LRU node if no free nodes available
 * @note Discovery: Always calls HBGetIdx() to allocate storage index
 * @note Discovery: Requires storage systems to be initialized first
 * @note Discovery: AST_FREE check is heuristic for node reusability
 */
HBNode *HBNew(HBMode_t mode) {
    HBNode *node = hbfree;
    if (node != NULL) {
        while (node != NULL && (node->mode & ~HBMODE_MODIFIED) != mode) {
            if (node->mode == HBMODE_UNUSED) {
                break;
            }
            node = node->lnext;
        }
        if (node == NULL) {
            node = hbfree;
        }
    } else {
        node = hblast->lprev;
    }
    HBStore(node);
    if (node->mode != mode || node->ast.type != AST_FREE) {     // @todo find a better way to check if node is unused
        HBRemove(node);
        node->mode = mode | HBMODE_MODIFIED;
        node->idx = HBGetIdx(mode);
        HBAdd(node);
    }
    HBTouched(node);
    return node;
}



HBNode *HBEmpty(void) {
    HBNode *node = hbfree;
    while (node != NULL && node->mode != HBMODE_UNUSED) {
        node = node->lnext;
    }
    if (node == NULL) {
        node = hbfree;
    }
    if (node == NULL) {
        node = hblast->lprev;
    }
    HBStore(node);
    HBRemove(node);
    node->mode = HBMODE_UNUSED;
    node->idx = 0;
    HBTouched(node);
    return node;
}



/**
 * @brief Get a node by index, creating it if it doesn't exist
 * 
 * This is the main cache interface. First checks if the node exists in cache,
 * if not, creates a new node and loads its data from storage.
 * 
 * @param idx The storage index to retrieve
 * @param mode The expected mode (HBMODE_SYM or HBMODE_AST)
 * @return Pointer to the node (never NULL - always returns a node object)
 * 
 * @note Discovery: ALWAYS returns a node object, even for invalid indices
 * @note Discovery: For index 0 (invalid), returns node with default/empty data
 * @note Discovery: Implements cache-miss handling via HBEmpty() + HBLoad()
 * @note Discovery: Storage systems handle invalid indices gracefully
 * @note Discovery: Critical for implementing transparent caching layer
 */
HBNode *HBGet(HMapIdx_t idx, HBMode_t mode) {
    HBNode *node = HBFind(idx, mode);
    if (node != NULL) {
        HBTouched(node);
        return node;
    }
    node = HBEmpty();
    node->mode = mode;
    node->idx = idx;
    HBLoad(node);
    HBAdd(node);
    return node;
}



void HBDelete(HBNode *node) {
    if (node == NULL) {
        return;     // @todo: error handling
    }
    node->ast.type = AST_FREE;
    node->mode &= ~HBMODE_MODIFIED;
    node->lnext->lprev = node->lprev;
    node->lprev->lnext = node->lnext;
    if (hblast == node) {
        hblast = node->lnext;
        if (hblast == node) {
            hblast = NULL;
        }
    }
    node->lnext = hbfree;
    node->lprev = hbfree->lprev;
    hbfree->lprev->lnext = node;
    hbfree->lprev = node;
    hbfree = node;
}
