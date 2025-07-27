# STCC1 Interface Design Discoveries

This document captures the critical interface design discoveries made during the comprehensive test debugging session that achieved 100% test pass rate.

## Overview

The STCC1 compiler uses a sophisticated LRU cache system (hmapbuf) that provides transparent caching for AST nodes and symbol table entries. During debugging, we discovered several critical design constraints and behaviors that are essential for correct usage.

## Critical Interface Discoveries

### 1. HMapBuf LRU Cache System

**Core Architecture:**
- Dual-linked list system: Hash table (hnext/hprev) + LRU list (lnext/lprev)
- Fixed pool of 100 nodes shared between AST and Symbol storage
- Write-back caching with lazy loading from persistent storage
- Hash table with chaining for O(1) average lookup

**Critical Invariants:**
- `node->idx` is **IMMUTABLE** after creation - never modify manually!
- `lnext/lprev` are **NEVER NULL** for active nodes (segfault risk!)
- `HBGet()` **ALWAYS** returns a node object (never NULL)
- Storage systems (astore/symtab) **MUST** be initialized before use

**LRU Behavior:**
- `hbfree`: Head of free list (unused nodes)
- `hblast`: Most recently used node (head of LRU list)
- `hblast->lprev`: Least recently used node (LRU victim)
- `HBTouched()` moves node to `hblast` position

### 2. Storage Systems Integration

**Index Management:**
- All storage uses **1-based indexing** (index 0 = invalid/error)
- `astore_get(0)` and `symtab_get(0)` return default/empty data gracefully
- `HBGetIdx()` creates valid default entries (AST_FREE nodes, empty symbols)

**Storage API Behavior:**
- Functions handle NULL/invalid inputs gracefully (return 0 or default data)
- All `add()` functions return 0 on failure, valid index on success
- All `get()` functions return valid structures (never fail/crash)

**Write-Back Caching:**
- Modified nodes marked with `HBMODE_MODIFIED` flag
- `HBStore()` writes back to storage and clears flag
- `HBLoad()` reads from storage on cache miss

### 3. Memory Management Patterns

**Node Lifecycle:**
1. Nodes start in free list (`hbfree`)
2. `HBNew()` allocates storage via `HBGetIdx()`
3. `HBTouched()` moves to LRU list (`hblast`)
4. LRU victim (`hblast->lprev`) gets reused when needed
5. `HBStore()` writes back before reuse

**Safe Usage Patterns:**
```c
// CORRECT: Initialize storage first
astore_init("file.out");
HBInit();

// CORRECT: Let system assign indices
HBNode* node = HBNew(HBMODE_AST);
// node->idx is now immutable!

// CORRECT: Always check for valid data
HBNode* retrieved = HBGet(some_idx, HBMODE_AST);
// retrieved is never NULL, but data might be default/empty

// WRONG: Never modify idx after creation
// node->idx = 0xFFFF;  // VIOLATES API DESIGN!
```

### 4. Error Handling Philosophy

**Graceful Degradation:**
- Invalid indices return default/empty data (not errors)
- NULL inputs are handled safely (no crashes)
- Storage errors return safe default values

**Test Design Insights:**
- Test API behavior, not internal implementation
- Focus on boundary conditions (index 0, high indices)
- Verify graceful handling of invalid inputs
- Never modify immutable fields in tests

## Key Functions and Their Contracts

### HBGet(idx, mode)
- **Always** returns a valid HBNode pointer
- For invalid idx (0), returns node with default/empty data
- Handles cache hits via hash table lookup
- Handles cache misses via `HBEmpty()` + `HBLoad()` + `HBAdd()`

### HBNew(mode)
- Allocates new storage index via `HBGetIdx()`
- Returns reused node or LRU victim
- **Requires** storage system to be initialized
- Sets `HBMODE_MODIFIED` flag

### HBTouched(node)
- **Critical**: Node must be properly linked (lprev/lnext != NULL)
- Moves node to most-recently-used position
- Sets `HBMODE_MODIFIED` flag
- Core of LRU mechanism

### Storage Functions (astore_*, symtab_*)
- Use 1-based indexing consistently
- Return 0 for failures, valid index for success
- Handle NULL inputs gracefully
- Always return valid structures (never crash)

## Debugging Lessons Learned

1. **Segmentation Fault Root Cause**: `HBTouched()` dereferencing NULL `lprev` pointer
2. **Storage Integration**: Must create valid default entries, not pass NULL
3. **API Design Violation**: Tests manually modifying immutable `idx` field
4. **Initialization Dependencies**: Storage systems must be initialized before cache use

## Testing Methodology

**Effective Test Patterns:**
- Test boundary conditions (index 0, max values)
- Verify graceful handling of invalid inputs
- Test API contracts, not implementation details
- Follow "fix the code, not the test" principle

**Anti-Patterns to Avoid:**
- Modifying immutable fields (`node->idx`)
- Assuming functions can return NULL when they can't
- Testing without proper initialization
- Weakening tests to make them pass

This comprehensive understanding enabled achieving 100% test pass rate while maintaining robust, defensive API design.
