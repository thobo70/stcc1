# STCC1 Storage System Analysis Report

## Overview
Analysis of storage system usage across all compiler stages (cc0, cc1, cc2) and diagnostic tools (cc0t, cc1t, cc2t) to verify proper implementation according to the documented architecture.

## ✅ Stage 1: Lexical Analysis (cc0) - CORRECT

### Storage Usage:
```c
// Proper initialization
sstore_init(argv[2]);  // Create string store
tstore_init(argv[3]);  // Create token store

// Proper token processing
sstore_str(lexeme, strlen(lexeme));  // Store strings with deduplication
tstore_add(&token);                  // Add tokens to store

// Proper cleanup
sstore_close();
tstore_close();
```

### Assessment: ✅ **COMPLIANT**
- Correctly uses `init()` functions to create new storage files
- Proper string deduplication in operator/keyword tables
- Appropriate cleanup and error handling
- Follows documented cc0 usage pattern exactly

## ⚠️ Stage 2: Parsing (cc1) - ISSUES FOUND

### Storage Usage:
```c
// Correct input handling
sstore_open(argv[1]);  // Open existing strings ✅
tstore_open(argv[2]);  // Open existing tokens ✅

// Correct output initialization  
astore_init(argv[3]);  // Create AST store ✅
symtab_init(argv[4]);  // Create symbol table ✅

// HMapBuf usage
HBInit();  // Initialize buffer ✅
// ... parsing using HBNew(), HBGet(), HBTouched() ✅

// PROBLEM: Missing HBEnd() call
parser_cleanup();  // Does NOT call HBEnd() ❌
```

### Issues Found:

#### 🔴 **Critical Issue: Missing HBEnd() Call**
**Location**: `src/parser/cc1.c:534` in `parser_cleanup()`  
**Problem**: The `HBEnd()` function is never called, which means:
- Modified AST/symbol nodes in hmapbuf cache are not flushed to storage
- Data loss occurs when cc1 exits without persisting cached changes
- Violates the documented cleanup pattern

**Fix Required**:
```c
static void parser_cleanup(void) {
    HBEnd();  // ADD THIS LINE - flush all cached nodes to storage
    
    if (error_core_has_errors()) {
        error_core_print_summary();
    }
    error_core_cleanup();
}
```

#### 🟡 **Suboptimal: Manual AST Transfer**
**Location**: `src/parser/cc1.c:723-750`  
**Problem**: Manual AST node transfer loop instead of relying on `HBEnd()`
- Hardcoded range (1-5) instead of systematic transfer
- Bypasses hmapbuf automatic persistence mechanism
- Risk of missing nodes outside the hardcoded range

**Current Code**:
```c
// Manual transfer - suboptimal
for (ASTNodeIdx_t idx = 1; idx <= 5; idx++) {
    HBNode *hb_node = HBGet(idx, HBMODE_AST);
    // ... manual copying to astore_add()
}
```

**Recommended Fix**: Remove manual transfer, rely on `HBEnd()` for automatic persistence.

### Assessment: ⚠️ **NON-COMPLIANT** - Critical data persistence issue

## ✅ Stage 3: Code Generation (cc2) - CORRECT

### Storage Usage:
```c
// Proper input handling
sstore_open(sstore_file);  // Read strings ✅
tstore_open(token_file);   // Read tokens ✅  
astore_open(ast_file);     // Read AST ✅
symtab_open(sym_file);     // Read symbols ✅

// Proper cleanup
symtab_close();
astore_close(); 
tstore_close();
sstore_close();
```

### Assessment: ✅ **COMPLIANT**
- Correctly uses `open()` functions for reading existing data
- No hmapbuf usage (appropriate for read-only access)
- Proper cleanup and error handling
- Follows documented cc2 usage pattern

## ⚠️ Diagnostic Tools Analysis

### cc0t (Token Viewer) - NOT ANALYZED
**Status**: Tool not examined in detail
**Expected**: Should use `sstore_open()` + `tstore_open()` for read-only access

### cc1t (AST Viewer) - POTENTIAL ISSUE

### Storage Usage:
```c
// Read-only access
sstore_open(argv[1]);  // Read strings ✅
astore_open(argv[2]);  // Read AST ✅  
symtab_open(argv[3]);  // Read symbols ✅

// Direct storage access - no hmapbuf
ASTNode node = astore_get(i);  // Direct read ✅
```

### Assessment: ✅ **COMPLIANT**  
- Correctly uses `open()` for read-only access
- No hmapbuf usage (appropriate for diagnostic tool)
- Direct storage reads are efficient for inspection

### cc2t (TAC Viewer) - NOT ANALYZED
**Status**: Tool not examined in detail
**Expected**: Should read TAC files directly

## Storage System Architecture Issues

### 1. hmapbuf Lifecycle Management
**Issue**: Incomplete lifecycle in cc1
- `HBInit()` ✅ Called correctly
- `HBEnd()` ❌ Missing - critical data loss risk  
- `HBStore()` ❌ Not used explicitly (relies on `HBEnd()`)

### 2. Storage Consistency
**Issue**: Mixed persistence strategies
- cc1 uses both hmapbuf AND manual AST transfer
- Risk of data inconsistency between cached and stored nodes
- Violates "single source of truth" principle

### 3. Memory Management
**Current**: 100-node LRU cache with automatic eviction
**Risk**: Without `HBEnd()`, dirty nodes may be lost on eviction

## Critical Fixes Required

### Priority 1: Fix cc1 HBEnd() Call
```c
// In src/parser/cc1.c, modify parser_cleanup():
static void parser_cleanup(void) {
    HBEnd();  // CRITICAL: Add this line
    
    if (error_core_has_errors()) {
        error_core_print_summary();
    }
    error_core_cleanup();
}
```

### Priority 2: Remove Manual AST Transfer
```c
// In src/parser/cc1.c main(), remove lines 723-750:
// DELETE the manual transfer loop - HBEnd() handles this automatically
```

### Priority 3: Verify hmapbuf Integration
- Ensure all AST node creation uses `HBNew(HBMODE_AST)`
- Verify `HBTouched()` is called after node modifications  
- Confirm automatic persistence works correctly

## Recommendations

1. **Immediate Action**: Fix the missing `HBEnd()` call in cc1
2. **Testing**: Verify AST persistence after compilation  
3. **Code Review**: Audit all hmapbuf usage patterns
4. **Documentation**: Update storage documentation with proper lifecycle
5. **Validation**: Create tests to verify storage consistency

## Summary

The storage system architecture is well-designed, but **cc1 has a critical implementation bug** that prevents proper data persistence. This violates the documented architecture and risks data loss. The fix is straightforward but essential for correct operation.

**Compliance Status**:
- ✅ cc0 (Lexer): Fully compliant
- ❌ cc1 (Parser): Critical issue - missing HBEnd()  
- ✅ cc2 (Code Generator): Fully compliant
- ✅ cc1t (Diagnostic): Compliant for read-only access
