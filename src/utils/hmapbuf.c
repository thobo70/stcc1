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


HMapIdx_t HBGetIdx(HBMode_t mode) {
    HMapIdx_t idx = 0;
    switch (mode & ~HBMODE_MODIFIED) {
        case HBMODE_SYM:
            idx = symtab_add(NULL);
            break;
        case HBMODE_AST:
            idx = astore_add(NULL);
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



void HBTouched(HBNode *node) {
    if (node == NULL) {
        return;     // @todo: error handling
    }
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
