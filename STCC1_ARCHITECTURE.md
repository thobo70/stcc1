# STCC1 AST and Symbol Table Architecture

## Overview

The STCC1 compiler uses a sophisticated multi-layered storage architecture that separates concerns between different data types while maintaining efficient cross-references. The system is built around four core storage components that work together to represent C source code structure and semantics.

## Core Architecture Components

### 1. Storage Layer Foundation

```
┌─────────────────────────────────────────────────────────────┐
│                    STCC1 Storage Architecture               │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌──────┐│
│  │   String    │  │    Token    │  │     AST     │  │Symbol││
│  │   Store     │◄─┤    Store    │◄─┤    Store    │◄─┤ Table││
│  │  (sstore)   │  │  (tstore)   │  │  (astore)   │  │(symtab)││
│  └─────────────┘  └─────────────┘  └─────────────┘  └──────┘│
│        ▲                 ▲                 ▲           ▲    │
│        │                 │                 │           │    │
│  ┌─────┴─────────────────┴─────────────────┴───────────┴──┐ │
│  │              Hash Map Buffer (HMAPBUF)                 │ │
│  │         LRU Cache + Unified Memory Management          │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### 2. Data Flow Architecture

```
Source Code
     │
     ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  Lexer      │───►│   Parser    │───►│ TAC Builder │
│   (cc0)     │    │    (cc1)    │    │    (cc2)    │
└─────────────┘    └─────────────┘    └─────────────┘
     │                     │                   │
     ▼                     ▼                   ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│String Store │    │ AST Store   │    │ TAC Store   │
│+ Token Store│    │+ Symbol Tab │    │             │
└─────────────┘    └─────────────┘    └─────────────┘
```

## Detailed Component Analysis

### String Store (sstore)

**Purpose**: Centralized string storage with deduplication and hash-based lookup.

**Structure**:
```c
typedef struct {
    hash_t hash;           // Hash value for fast lookup
    sstore_pos_t pos;      // Position in string buffer
} sstore_entry_t;

typedef uint16_t sstore_pos_t;  // 16-bit position (64KB max)
```

**Key Features**:
- **Deduplication**: Identical strings share the same storage location
- **Hash-based lookup**: O(1) average lookup time using FNV-1a hash
- **Compact references**: 16-bit positions reduce memory usage
- **Immutable storage**: Strings never change once stored

**Usage Patterns**:
- Variable names, function names, string literals
- Referenced by AST nodes, symbol table entries
- Used throughout all compiler phases

### Token Store (tstore)

**Purpose**: Sequential storage of lexical tokens with position tracking.

**Structure**:
```c
typedef struct Token_t {
    TokenID_t id;          // Token type (T_ID, T_LITINT, etc.)
    sstore_pos_t pos;      // Position in string store
    uint16_t line;         // Source line number
    uint16_t column;       // Source column number
} Token_t;

typedef uint32_t TokenIdx_t;  // 32-bit token index
```

**Key Features**:
- **Sequential access**: Tokens stored in parse order
- **Source location tracking**: Line/column for error reporting
- **String reference**: Links to string store for token text
- **Parser state**: Current position tracking for backtracking

### AST Store (astore)

**Purpose**: Hierarchical representation of program structure.

**Core Node Structure**:
```c
typedef struct ASTNode {
    ASTNodeType type;        // Node type (130+ types)
    ASTNodeFlags flags;      // Compiler phase flags
    TokenIdx_t token_idx;    // Source token reference
    TypeIdx_t type_idx;      // Type information
    
    // Flexible child organization (union)
    union {
        // Generic children (for most nodes)
        struct {
            ASTNodeIdx_t child1, child2, child3, child4;
        } children;
        
        // Binary operations (expressions, assignments)
        struct {
            ASTNodeIdx_t left, right;
            union {
                SymTabIdx_t symbol_idx;    // Variable reference
                sstore_pos_t string_pos;   // String literal
                int64_t long_value;        // Integer literal
                double float_value;        // Float literal
            } value;
        } binary;
        
        // Compound statements (blocks)
        struct {
            ASTNodeIdx_t declarations;     // First declaration
            ASTNodeIdx_t statements;       // First statement
            SymTabIdx_t scope_idx;         // Scope reference
        } compound;
        
        // Function calls
        struct {
            ASTNodeIdx_t function;         // Function identifier
            ASTNodeIdx_t arguments;        // Argument list
            TypeIdx_t return_type;         // Return type
            char arg_count;                // Number of arguments
        } call;
        
        // ... other specialized layouts
    };
} ASTNode;
```

**Node Categories**:
```
AST_CAT_SPECIAL      (0-9):   AST_FREE, AST_PROGRAM, AST_EOF
AST_CAT_DECLARATION  (10-29): AST_FUNCTION_DEF, AST_VAR_DECL
AST_CAT_TYPE         (30-49): AST_TYPE_BASIC, AST_TYPE_POINTER
AST_CAT_STATEMENT    (50-79): AST_STMT_COMPOUND, AST_STMT_WHILE
AST_CAT_EXPRESSION   (80-139): AST_EXPR_BINARY_OP, AST_LIT_INTEGER
```

### Symbol Table (symtab)

**Purpose**: Identifier resolution with C99 scoping semantics.

**Structure**:
```c
typedef struct SymTabEntry {
    SymType type;            // Symbol type (variable, function, etc.)
    sstore_pos_t name;       // Name in string store
    SymIdx_t parent;         // Parent scope (unused in C99)
    SymIdx_t next, prev;     // Linked list navigation
    SymIdx_t child, sibling; // Hierarchical navigation
    sstore_pos_t value;      // Additional data
    int line;                // Declaration line
    int scope_depth;         // C99 block scope depth
} SymTabEntry;
```

**Symbol Types**:
```c
typedef enum {
    SYM_VARIABLE,    // Local/global variables
    SYM_FUNCTION,    // Function declarations/definitions
    SYM_TYPEDEF,     // Type aliases
    SYM_LABEL,       // goto labels
    SYM_ENUMERATOR,  // Enum constants
    SYM_STRUCT,      // Struct tags
    SYM_UNION,       // Union tags
    SYM_ENUM,        // Enum tags
    SYM_CONSTANT     // Manifest constants
} SymType;
```

## Cross-Component Relationships

### AST ↔ Symbol Table Integration

```
AST Node (Variable Reference)
├── type: AST_EXPR_IDENTIFIER
├── binary.value.symbol_idx ────┐
└── token_idx: 42               │
                                ▼
                      Symbol Table Entry
                      ├── type: SYM_VARIABLE
                      ├── name: sstore_pos (→ "variable_name")
                      ├── scope_depth: 2
                      └── line: 15
```

### String Store References

```
Multiple Components Reference Strings:

Symbol Table Entry          AST Node (String Literal)
├── name: pos_123 ─────┐    ├── binary.value.string_pos: pos_456
└── value: pos_789     │    └── type: AST_LIT_STRING
                       │
                       ▼
               String Store
               ├── [pos_123] = "variable_name"
               ├── [pos_456] = "Hello World"
               └── [pos_789] = "additional_data"
```

### Token Chain Tracking

```
Source: int x = 42;

Token Store:                    AST Integration:
[1] T_INT      → "int"         AST_VAR_DECL (token_idx: 1)
[2] T_ID       → "x"           ├── AST_EXPR_IDENTIFIER (token_idx: 2)
[3] T_ASSIGN   → "="           └── AST_LIT_INTEGER (token_idx: 4)
[4] T_LITINT   → "42"              └── value: 42
[5] T_SEMICOL  → ";"
```

## Memory Management Architecture

### Hash Map Buffer (HMAPBUF)

**Purpose**: Unified LRU cache for AST nodes and symbol table entries.

**Architecture**:
```
┌─────────────────────────────────────────────────────────────┐
│                    HMAPBUF (100 nodes)                      │
├─────────────────────────────────────────────────────────────┤
│  Hash Table (8 buckets)          LRU Chain                  │
│  ┌──────┐                       ┌─────────────────────┐     │
│  │ [0]  │──►Node A◄─────────────┤ hblast (MRU)        │     │
│  │ [1]  │   │                   │         ▲           │     │
│  │ [2]  │──►Node B              │         │           │     │
│  │ [3]  │   │                   │    ┌────┴────┐      │     │
│  │ [4]  │──►Node C              │    │  Node   │      │     │
│  │ [5]  │                       │    │ ┌─────┐ │      │     │
│  │ [6]  │                       │    │ │     │ │      │     │
│  │ [7]  │                       │    └─┴──▲──┴─┘      │     │
│  └──────┘                       │       │  │          │     │
│          hash collision chains  │       │  └────┐     │     │
│          hnext/hprev pointers   │       │       │     │     │
│                                 │       ▼       ▼     │     │
│                                 │   lnext──────lprev  │     │
│                                 │                     │     │
│                                 └─────────────────────┘     │
│                                    hblast->lprev (LRU)      │
└─────────────────────────────────────────────────────────────┘
```

**Node Structure**:
```c
typedef struct HBNode {
    uint32_t idx;              // Immutable storage index
    struct HBNode *hnext, *hprev;  // Hash chain
    struct HBNode *lnext, *lprev;  // LRU chain
    int mode;                  // HBMODE_AST or HBMODE_SYM
    
    union {
        ASTNode ast;           // When mode = HBMODE_AST
        SymTabEntry sym;       // When mode = HBMODE_SYM
    };
} HBNode;
```

## C99 Scoping Implementation

### Scope Depth Tracking

```c
// Parser state for C99 compliance
static struct {
    int scope_depth;           // 0=file, 1=function, 2+=block
    SymIdx_t current_function; // Current function context
} parser_state;
```

### Symbol Resolution Algorithm

```c
// C99 Scoping Rules:
// 1. Innermost scope hides outer scopes
// 2. File scope (depth 0) for globals
// 3. Function scope (depth 1) for parameters
// 4. Block scope (depth 2+) for local variables

SymIdx_t lookup_symbol_c99(sstore_pos_t name) {
    SymIdx_t best_match = 0;
    int best_depth = -1;
    
    for (SymIdx_t i = 1; i <= symbol_count; i++) {
        SymTabEntry entry = symtab_get(i);
        
        if (entry.name == name && 
            entry.scope_depth <= current_scope_depth &&
            entry.scope_depth > best_depth) {
            best_match = i;
            best_depth = entry.scope_depth;
        }
    }
    
    return best_match;
}
```

## AST Construction Patterns

### Expression Trees

```
C Code: sum = sum + i;

AST Structure:
AST_EXPR_ASSIGN
├── left: AST_EXPR_IDENTIFIER
│   └── binary.value.symbol_idx → SymTab["sum"]
└── right: AST_EXPR_BINARY_OP (ADD)
    ├── left: AST_EXPR_IDENTIFIER
    │   └── binary.value.symbol_idx → SymTab["sum"]
    └── right: AST_EXPR_IDENTIFIER
        └── binary.value.symbol_idx → SymTab["i"]
```

### Statement Chaining

```
C Code: { stmt1; stmt2; stmt3; }

AST Structure:
AST_STMT_COMPOUND
├── compound.declarations → first declaration
└── compound.statements → first statement
                          ├── children.child2 → second statement
                          │   └── children.child2 → third statement
                          └── children.child2 → 0 (end)
```

### Function Definitions

```
C Code: int func(int a, int b) { return a + b; }

AST Structure:
AST_FUNCTION_DEF
├── children.child1 → Return Type (AST_TYPE_BASIC)
├── children.child2 → Function Name (AST_EXPR_IDENTIFIER)
├── children.child3 → Parameter List
└── children.child4 → Function Body (AST_STMT_COMPOUND)

Symbol Table:
├── [1] SYM_FUNCTION name:"func" scope_depth:0
├── [2] SYM_VARIABLE name:"a"    scope_depth:1
└── [3] SYM_VARIABLE name:"b"    scope_depth:1
```

## Error Handling and Recovery

### Error Propagation Chain

```
Parse Error
     │
     ▼
AST_FLAG_ERROR set on node
     │
     ▼
Error core reporting system
     │
     ▼
Symbol table entry marked invalid
     │
     ▼
TAC generation skips erroneous nodes
```

### Recovery Mechanisms

1. **Graceful Degradation**: Invalid nodes return empty/default values
2. **Error Isolation**: Errors don't corrupt surrounding valid nodes
3. **State Consistency**: Parser state remains valid after errors
4. **Resource Cleanup**: Failed allocations don't leak memory

## Performance Characteristics

### Time Complexity
- **String lookup**: O(1) average (hash-based)
- **Symbol resolution**: O(n) worst case (linear search)
- **AST traversal**: O(n) (each node visited once)
- **Cache access**: O(1) for cached nodes, O(1) for cache misses

### Space Complexity
- **String store**: O(total_string_length)
- **AST store**: O(number_of_nodes) × 24 bytes/node
- **Symbol table**: O(number_of_symbols) × 32 bytes/symbol
- **Cache overhead**: 100 nodes × 64 bytes = 6.4KB fixed

### Memory Usage Optimization
- **Compact representations**: 16-bit indices where possible
- **Union-based layouts**: Minimize memory per AST node
- **LRU caching**: Keep frequently accessed data in memory
- **Write-back caching**: Reduce disk I/O operations

## Integration with TAC Generation

### Symbol Resolution in TAC

```c
// TAC Builder resolves AST references:
AST_EXPR_IDENTIFIER node
├── binary.value.symbol_idx: 42
└── TAC Operand Generation:
    ├── Look up SymTabEntry[42]
    ├── Get name from string store
    └── Create TAC_OP_VAR operand
```

### Type System Integration

```c
// Future type checking support:
AST Node
├── type_idx → Type Table Entry
├── Type checking validates operations
└── TAC generation uses type information
```

## Architectural Strengths

1. **Modularity**: Clear separation between lexical, syntactic, and semantic data
2. **Efficiency**: Hash-based lookups, LRU caching, compact representations
3. **Scalability**: Fixed-size caches with good locality of reference
4. **Maintainability**: Well-defined interfaces between components
5. **C99 Compliance**: Proper scoping semantics and identifier resolution
6. **Error Resilience**: Graceful handling of malformed input

## Design Trade-offs

### Advantages
- **Memory efficiency**: Compact node representations
- **Fast access**: O(1) cache hits for recently used data
- **Consistency**: Unified caching for all data types
- **Flexibility**: Union-based layouts adapt to node types

### Limitations
- **Cache size**: Fixed 100-node limit may cause thrashing
- **Symbol lookup**: Linear search for scope resolution
- **16-bit limits**: String store and indices limited to 64K
- **Complexity**: Multiple indirection levels for data access

This architecture provides a solid foundation for a C99-compliant compiler with efficient memory usage and clear separation of concerns between lexical, syntactic, and semantic analysis phases.
