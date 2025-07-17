# STCC1 Storage System Architecture

## Overview

The STCC1 compiler uses a modular, file-based storage system designed for memory-constrained environments (<100KB RAM). Each compiler stage produces persistent data files that subsequent stages can read, enabling a pipeline-based compilation process.

## Core Storage Components

### 1. String Store (sstore)
**Purpose**: Centralized string storage with deduplication and hash-based lookup  
**Files**: `.sstore` files  
**Location**: `src/storage/sstore.{c,h}`

**Key Features**:
- Hash-based string deduplication
- Position-based string retrieval
- Compact 16-bit position references
- Shared across all compilation stages

**API**:
```c
int sstore_init(const char *fname);     // Create new string store
int sstore_open(const char *fname);     // Open existing string store
sstore_pos_t sstore_str(const char *str, sstore_len_t length);  // Store string
char *sstore_get(sstore_pos_t pos);     // Retrieve string by position
void sstore_close(void);                // Close and cleanup
```

### 2. Token Store (tstore)
**Purpose**: Persistent token storage for lexical analysis output  
**Files**: `.out` files (token output)  
**Location**: `src/storage/tstore.{c,h}`

**Key Features**:
- Sequential token storage
- Token metadata (line numbers, positions)
- Efficient token stream replay for parser

**API**:
```c
int tstore_init(const char *filename);      // Create new token store
int tstore_open(const char *filename);      // Open existing token store
TokenIdx_t tstore_add(Token_t *token);      // Add token to store
Token_t tstore_get(TokenIdx_t idx);         // Get token by index
TokenIdx_t tstore_getidx(void);             // Get current token count
void tstore_close(void);                    // Close and cleanup
```

### 3. AST Store (astore)
**Purpose**: Abstract Syntax Tree node storage  
**Files**: `.astore` files  
**Location**: `src/storage/astore.{c,h}`

**Key Features**:
- Hierarchical AST node storage
- Type-specific node structures
- Cross-references between nodes
- Memory-efficient node indexing

**API**:
```c
int astore_init(const char *filename);      // Create new AST store
int astore_open(const char *filename);      // Open existing AST store
ASTNodeIdx_t astore_add(ASTNode *node);     // Add AST node
ASTNode astore_get(ASTNodeIdx_t idx);       // Get AST node by index
ASTNodeIdx_t astore_getidx(void);           // Get current node count
void astore_close(void);                    // Close and cleanup
```

### 4. Symbol Table (symtab)
**Purpose**: Symbol information storage with scoping  
**Files**: `.sym` files  
**Location**: `src/storage/symtab.{c,h}`

**Key Features**:
- Hierarchical symbol scoping
- Symbol type information
- Parent/child/sibling relationships
- Line number tracking

**API**:
```c
int symtab_init(const char *filename);      // Create new symbol table
int symtab_open(const char *filename);      // Open existing symbol table
SymIdx_t symtab_add(SymTabEntry *entry);    // Add symbol entry
SymTabEntry symtab_get(SymIdx_t idx);       // Get symbol by index
void symtab_close(void);                    // Close and cleanup
```

### 5. Hash Map Buffer (hmapbuf)
**Purpose**: Memory management and caching layer  
**Files**: No persistent files (memory-only)  
**Location**: `src/utils/hmapbuf.{c,h}`

**Key Features**:
- LRU-based node caching
- Hash table for fast lookups
- Unified interface for AST and symbol nodes
- Automatic persistence to storage files

**Architecture**:
```c
typedef struct HBNode {
    HMapIdx_t idx;              // Storage index
    HBMode_t mode;              // HBMODE_SYM or HBMODE_AST
    struct HBNode *hnext;       // Hash table chain
    struct HBNode *hprev;       // Hash table chain
    struct HBNode *lnext;       // LRU list
    struct HBNode *lprev;       // LRU list
    union {
        SymTabEntry sym;        // Symbol data
        ASTNode ast;            // AST node data
    };
} HBNode;
```

## Compilation Pipeline Data Flow

### Stage 1: Lexical Analysis (cc0)
**Input**: Preprocessed C source (`.i` files)  
**Output**: String store + Token store  
**Usage**:
```bash
cc0 input.i strings.sstore tokens.out
```

**Storage Operations**:
1. `sstore_init()` - Create string store for identifiers, literals
2. `tstore_init()` - Create token store for lexical tokens
3. For each token:
   - Store string content in sstore
   - Store token metadata in tstore
4. `sstore_close()`, `tstore_close()` - Persist to files

### Stage 2: Parsing (cc1)
**Input**: String store + Token store  
**Output**: AST store + Symbol table  
**Usage**:
```bash
cc1 strings.sstore tokens.out ast.astore symbols.sym
```

**Storage Operations**:
1. `sstore_open()` - Read existing strings
2. `tstore_open()` - Read token stream
3. `astore_init()` - Create AST store
4. `symtab_init()` - Create symbol table
5. `HBInit()` - Initialize hash map buffer for caching
6. Parse tokens into AST nodes and symbols:
   - Use `HBNew()` to get cached nodes
   - Build AST structure with cross-references
   - Maintain symbol scoping relationships
7. Transfer final nodes to persistent storage
8. Close all stores

### Stage 3: Code Generation (cc2)
**Input**: All previous stores (strings, tokens, AST, symbols)  
**Output**: TAC (Three Address Code)  
**Usage**:
```bash
cc2 strings.sstore tokens.out ast.astore symbols.sym tac.out [output.tac]
```

**Storage Operations**:
1. `sstore_open()` - Read strings for identifiers
2. `tstore_open()` - Access tokens if needed
3. `astore_open()` - Read AST structure
4. `symtab_open()` - Read symbol information
5. Generate TAC by traversing AST
6. Output to TAC files

## Hash Map Buffer (hmapbuf) Memory Management

### Purpose
The hmapbuf provides a sophisticated memory management layer that:
- Caches frequently accessed AST nodes and symbol entries
- Provides LRU-based eviction for memory efficiency
- Automatically syncs with persistent storage
- Maintains hash tables for O(1) lookups

### Node Lifecycle
1. **Allocation**: `HBNew(mode)` gets a free node or evicts LRU
2. **Access**: `HBGet(idx, mode)` retrieves node (loads from storage if needed)
3. **Modification**: `HBTouched(node)` marks node as modified
4. **Persistence**: `HBStore(node)` syncs to storage file
5. **Eviction**: LRU nodes automatically stored and reused

### Hash Table Organization
- 8-slot hash table with chaining
- Nodes remain in hash table even when in free list
- Hash based on storage index for consistent lookup
- Separate chains for hash collisions

### LRU Lists
- **Free List**: Unused nodes ready for allocation
- **LRU List**: Active nodes ordered by recent access
- All nodes always in exactly one list
- Automatic migration between lists based on usage

## Memory Efficiency Features

### Compact Data Types
- 16-bit indices for all references (max 65K entities)
- Packed structures to minimize memory footprint
- Union types in hmapbuf for symbol/AST node sharing

### File-Based Persistence
- No requirement to keep entire structures in memory
- On-demand loading through hmapbuf caching
- Streaming processing for large source files

### Storage Optimization
- String deduplication reduces memory usage
- Hash-based string lookup prevents duplicates
- Incremental storage growth as needed

## Usage Guidelines by Compiler Stage

### CC0 (Lexer) Usage
```c
// Initialize storage
sstore_init("output.sstore");
tstore_init("output.out");

// For each token
sstore_pos_t str_pos = sstore_str(lexeme, strlen(lexeme));
Token_t token = {id, str_pos, file_pos, line_num};
tstore_add(&token);

// Cleanup
sstore_close();
tstore_close();
```

### CC1 (Parser) Usage
```c
// Open inputs, create outputs
sstore_open("input.sstore");
tstore_open("input.out");
astore_init("output.astore");
symtab_init("output.sym");
HBInit();

// Parse using cached nodes
HBNode *ast_node = HBNew(HBMODE_AST);
ast_node->ast.type = AST_EXPR_BINARY_OP;
ast_node->ast.binary.left = left_idx;
ast_node->ast.binary.right = right_idx;
HBTouched(ast_node);

// Cleanup transfers cached nodes to storage
HBEnd();
astore_close();
symtab_close();
```

### CC2 (Code Generator) Usage
```c
// Open all input stores
sstore_open("input.sstore");
astore_open("input.astore");
symtab_open("input.sym");

// Traverse AST for code generation
ASTNode root = astore_get(1);
// Generate TAC based on AST structure

// Close inputs
astore_close();
symtab_close();
sstore_close();
```

## Error Handling

### Storage Errors
- All storage functions return error codes
- Check return values for initialization failures
- Handle file I/O errors gracefully

### Memory Constraints
- hmapbuf automatically manages memory limits
- LRU eviction prevents memory exhaustion
- Monitor HBNNODES (100) limit for active nodes

## Performance Characteristics

### Time Complexity
- Hash table lookups: O(1) average
- LRU operations: O(1)
- Storage I/O: O(1) per operation

### Space Complexity
- hmapbuf: Fixed 100 nodes maximum
- Storage files: Linear with source size
- String store: Deduplicated, sub-linear growth

## Best Practices

1. **Always check return codes** from storage operations
2. **Use hmapbuf** for frequent AST/symbol access in cc1
3. **Close stores properly** to ensure data persistence
4. **Minimize string creation** to leverage deduplication
5. **Use appropriate storage modes** (init vs open) per stage
6. **Handle memory pressure** by checking hmapbuf limits

This storage architecture enables STCC1 to compile large C programs while maintaining strict memory constraints through intelligent caching, file-based persistence, and efficient data structures.
