# AST Node Types Reference

This document provides a comprehensive reference for all AST node types in the STCC1 compiler, including their content, parameters, meaning, and linking relationships.

## Verified Type Information

All type sizes and indexing schemes have been verified against the STCC1 source code:

| Type | Size | Underlying Type | Indexing | Error Value |
|------|------|-----------------|----------|-------------|
| `sstore_pos_t` | 2 bytes | `uint16_t` | 0-based | `SSTORE_ERR` (0xFFFF) |
| `SymIdx_t` | 2 bytes | `unsigned short` | 1-based | 0 (invalid) |
| `ASTNodeIdx_t` | 2 bytes | `uint16_t` | 1-based | 0 (null node) |
| `TokenIdx_t` | 4 bytes | `unsigned int` | 0-based | `TSTORE_ERR` (0xFFFF) |
| `TACIdx_t` | 2 bytes | `uint16_t` | 0-based | N/A |

**Important Notes**:
- Symbol table and AST store use 1-based indexing where 0 indicates error/null
- String store and token store use 0-based indexing where 0xFFFF indicates error
- TAC store uses 0-based indexing

## Node Structure Overview

All AST nodes in STCC1 are **exactly 24 bytes** and share a common structure that enables efficient memory management and fast traversal. Each node consists of:

1. **Common Header (10 bytes)**: Shared by all node types
2. **Union Data (14 bytes)**: Type-specific data structures

### Common Header Fields
- `type` (ASTNodeType, 2 bytes): The specific node type (see [Node Type Categories](#node-type-categories))
- `flags` (ASTNodeFlags, 2 bytes): Compilation phase flags and status bits
- `token_idx` (TokenIdx_t, 4 bytes): Reference to source token for error reporting and debugging
- `type_idx` (TypeIdx_t, 2 bytes): Type information index (populated during semantic analysis)

### Union Data Structures
The remaining 14 bytes use one of several union structures optimized for different node categories:
- **`children`**: Generic 4-child structure for complex nodes
- **`binary`**: Two-child structure with additional value data
- **`unary`**: Single-child structure with operator and data
- **`compound`**: Specialized for block statements with scope information
- **`conditional`**: Specialized for if/while/switch statements
- **`call`**: Specialized for function calls with argument metadata
- **`declaration`**: Specialized for variable/function declarations

---

## Storage Systems and Data References

STCC1 uses multiple storage systems to efficiently manage different types of data. Understanding these systems is crucial for correct AST node processing.

### Storage System Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           STCC1 Storage Architecture                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐        │
│  │   String Store  │    │   Symbol Table  │    │    AST Store    │        │
│  │    (sstore)     │    │    (symtab)     │    │    (astore)     │        │
│  │                 │    │                 │    │                 │        │
│  │ "main"          │◄───┤ symbol_idx: 1   │◄───┤ AST_FUNCTION_DEF│        │
│  │ "printf"        │    │ name_pos: 0     │    │ symbol_idx: 1   │        │
│  │ "x"             │    │ type: FUNCTION  │    │ type_idx: T_INT │        │
│  │ "hello world"   │    │ scope: 1        │    │                 │        │
│  │                 │    │ type_idx: T_INT │    │                 │        │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘        │
│         ▲                                                ▲                │
│         │                                                │                │
│  ┌─────────────────┐                           ┌─────────────────┐        │
│  │   Token Store   │                           │    TAC Store    │        │
│  │    (tstore)     │                           │   (tacstore)    │        │
│  │                 │                           │                 │        │
│  │ T_IDENTIFIER    │                           │ TAC_ASSIGN      │        │
│  │ T_INT, T_CHAR   │                           │ operand1: var_1 │        │
│  │ string_pos: 8   │                           │ operand2: imm_42│        │
│  │ line: 5         │                           │ type: T_INT     │        │
│  └─────────────────┘                           └─────────────────┘        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1. String Store (sstore)

**Purpose**: Centralized storage for all string literals and identifiers in the source code.

**Data Type**: `sstore_pos_t` (2 bytes, uint16_t)

**Access**: `sstore_get(sstore_pos_t pos)` → `char*`

**Error Value**: `SSTORE_ERR` (0xFFFF) indicates invalid position

⚠️ **CRITICAL**: The returned pointer points to a **static internal buffer** (`static char buf[1024]`) that **WILL be overwritten** by the next `sstore_get()` call. If you need to preserve the string, copy it immediately:
```c
char *name = sstore_get(name_pos);
char *safe_copy = strdup(name);  // Safe copy
// or
char buffer[256];
strcpy(buffer, name);  // Safe copy
```

**Contains**:
- Variable names (`"x"`, `"count"`, `"buffer"`)
- Function names (`"main"`, `"printf"`, `"malloc"`)
- String literals (`"Hello, World!"`, `"Error: %s\n"`)
- Type names (`"struct point"`, `"FILE"`)

**Usage in AST Nodes**:
```c
// For unresolved identifiers (during parsing)
AST_EXPR_IDENTIFIER
├─ left = 0
├─ right = 0  
└─ value.string_pos = 15  → sstore_get(15) = "variable_name"

// For string literals
AST_LIT_STRING
├─ left = 0
├─ right = 0
└─ value.string_pos = 42  → sstore_get(42) = "Hello, World!"
```

### 2. Symbol Table (symtab)

**Purpose**: Manages symbol information including scope, type, and semantic properties.

**Data Type**: `SymIdx_t` (2 bytes, unsigned short)

**Access**: `symtab_get(SymIdx_t idx)` → `SymTabEntry`

✅ **Safe**: `symtab_get()` returns the structure **by value** (not pointer), so the returned `SymTabEntry` is automatically a safe copy that won't be affected by subsequent symbol table operations.

**Symbol Table Entry Structure**:
```c
typedef struct {
    sstore_pos_t name_pos;      // Position in string store
    SymType type;               // VARIABLE, FUNCTION, TYPE, etc.
    SymIdx_t parent_idx;     // Parent scope (0 for global)
    SymIdx_t next_idx;       // Next symbol in same scope
    SymIdx_t prev_idx;       // Previous symbol in same scope
    SymIdx_t child_idx;      // First child scope
    SymIdx_t sibling_idx;    // Next sibling scope
    TypeIdx_t type_idx;         // Type information
    uint32_t value;             // Symbol value (for constants)
    uint16_t line_number;       // Source line number
    uint16_t scope_depth;       // Nesting level
    uint8_t flags;              // Symbol flags (C99, static, etc.)
} SymTabEntry;
```

**Symbol Types**:
- `SYM_VARIABLE`: Local/global variables
- `SYM_FUNCTION`: Function definitions/declarations
- `SYM_PARAMETER`: Function parameters
- `SYM_TYPE`: Type definitions (typedef, struct, union, enum)
- `SYM_LABEL`: Statement labels
- `SYM_CONSTANT`: Enumeration constants

**Indexing Scheme**: 1-based indexing (index 0 = invalid/error)
**Error Value**: Symbol table operations return 0 for errors

**Usage in AST Nodes**:
```c
// For resolved identifiers (after semantic analysis)
AST_EXPR_IDENTIFIER
├─ left = 0
├─ right = 0  
└─ value.symbol_idx = 5  → symtab_get(5) = SymTabEntry for "x"

// For variable declarations
AST_VAR_DECL
├─ symbol_idx = 5        → Points to symbol table entry
├─ type_idx = 2          → Points to type information
├─ initializer = 15      → Points to initialization expression
└─ storage_class = 0     → auto storage class
```

### 3. AST Store (astore)

**Purpose**: Manages the AST nodes themselves.

**Data Type**: `ASTNodeIdx_t` (2 bytes, uint16_t)

**Access**: `astore_get(ASTNodeIdx_t idx)` → `ASTNode`

**Indexing Scheme**: 1-based indexing (index 0 = invalid/null node)

**Error Handling**: Functions return 0 for errors

⚠️ **Note**: `astore_get()` returns the node **by value** (not pointer), so the returned `ASTNode` is automatically a safe copy that won't be affected by subsequent AST operations.

**Usage**: All child references in AST nodes use ASTNodeIdx_t values.

### 4. Token Store (tstore)

**Purpose**: Stores tokenized source code with position and type information.

**Data Type**: `TokenIdx_t` (4 bytes, unsigned int)

**Access**: `tstore_get(TokenIdx_t idx)` → `Token`

**Indexing Scheme**: 0-based indexing (index 0 = first token)

**Error Value**: `TSTORE_ERR` (0xFFFF) indicates invalid token

⚠️ **Note**: `tstore_get()` returns the token **by value** (not pointer), so the returned `Token` is automatically a safe copy that won't be affected by subsequent tokenizer operations.

**Token Structure**:
```c
typedef struct {
    TokenID_t id;              // T_IDENTIFIER, T_NUMBER, etc.
    sstore_pos_t string_pos;   // Position in string store (for identifiers/strings)
    uint32_t line_number;      // Source line
    uint32_t column_number;    // Source column
    uint32_t length;           // Token length
} Token;
```

### 5. TAC Store (tacstore)

**Purpose**: Stores Three-Address Code instructions for intermediate representation.

**Data Type**: `TACIdx_t` (2 bytes, uint16_t)

**Access**: `tacstore_get(TACIdx_t idx)` → `TACInstruction`

✅ **Safe**: `tacstore_get()` returns the instruction **by value** (not pointer), so the returned `TACInstruction` is automatically a safe copy that won't be affected by subsequent TAC operations.

**Usage**: Used during code generation phase to store intermediate representation.

### 6. Type System (embedded in Symbol Table and AST)

**Purpose**: Manages type information distributed across multiple components.

**Data Type**: `TypeIdx_t` (uint16_t, 2 bytes) - universal type identifier

**Implementation**: STCC1 uses a **distributed type system** with no separate type store:

**Components**:
- **Lexer tokens**: `T_INT`, `T_CHAR`, `T_FLOAT`, `T_VOID` etc. serve as basic type identifiers
- **Symbol table**: `SymTabEntry.type_idx` stores type information for each symbol
- **AST type nodes**: `AST_TYPE_*` nodes represent complex type structures
- **C99 qualifiers**: Stored as flags in symbol table (`SYM_FLAG_CONST`, `SYM_FLAG_VOLATILE`, etc.)

**Type Resolution**:
- **Basic types**: Token IDs → TypeIdx_t → Symbol table → AST nodes
- **Complex types**: Built as AST subtrees (pointers, arrays, functions, structs)
- **Type checking**: Performed during TAC generation using TypeIdx_t values

**Key insight**: Types flow through the entire compilation pipeline via TypeIdx_t, unifying lexer tokens, symbol table entries, AST nodes, and TAC operations.

---

## Symbol Table Detailed Reference

### Symbol Resolution Process

1. **Parsing Phase**: Identifiers stored with `string_pos` reference
2. **Semantic Analysis**: Identifiers resolved to `symbol_idx` reference
3. **Code Generation**: Symbols accessed via `symbol_idx` for TAC generation

### Scope Management

**Scope Hierarchy Example**:
```c
int global_var;              // Scope 0 (global)

int main() {                 // Scope 1 (function)
    int local_var;           // Scope 2 (function body)
    
    if (condition) {         // Scope 3 (if block)
        int block_var;       // Scope 3
    }
    
    while (loop) {           // Scope 4 (while block)
        int loop_var;        // Scope 4
    }
}
```

**Symbol Table Relationships**:
```
Global Scope (0)
├─ global_var (type: VARIABLE, scope: 0)
└─ main (type: FUNCTION, scope: 0)
   └─ Function Scope (1)
      ├─ local_var (type: VARIABLE, scope: 2)
      ├─ If Scope (3)
      │  └─ block_var (type: VARIABLE, scope: 3)
      └─ While Scope (4)
         └─ loop_var (type: VARIABLE, scope: 4)
```

### Symbol Lookup Algorithm

```c
SymIdx_t resolve_identifier(sstore_pos_t name_pos, SymIdx_t current_scope) {
    // Start from current scope and work upward
    SymIdx_t scope = current_scope;
    
    while (scope != 0) {
        // Search all symbols in current scope
        SymIdx_t sym_idx = get_first_symbol_in_scope(scope);
        
        while (sym_idx != 0) {
            SymTabEntry entry = symtab_get(sym_idx);
            
            if (entry.name_pos == name_pos) {
                return sym_idx;  // Found!
            }
            
            sym_idx = entry.next_idx;
        }
        
        // Move to parent scope
        scope = get_parent_scope(scope);
    }
    
    return 0;  // Not found
}
```

### C99 Symbol Features

**Mixed Declarations**: C99 allows declarations anywhere in blocks
```c
{
    int x = 5;          // Declaration
    printf("%d", x);    // Statement
    int y = x * 2;      // Declaration (mixed with statements)  
    printf("%d", y);    // Statement
}
```

**Symbol Table Handling**:
- Declarations can appear at any point in compound statements
- Each declaration creates a new symbol table entry
- Scope rules still apply (inner scopes hide outer scopes)

---

## Data Reference Resolution Guide

### Phase 1: Parsing (Unresolved References)

During parsing, identifiers are stored with string store references:

```c
// Source: x = 42;
// During parsing:
AST_EXPR_ASSIGN
├─ left  → AST_EXPR_IDENTIFIER
│          └─ value.string_pos = 15  // "x" in string store
└─ right → AST_LIT_INTEGER
           └─ value.long_value = 42
```

### Phase 2: Semantic Analysis (Symbol Resolution)

During semantic analysis, string references are resolved to symbol table entries:

```c
// After semantic analysis:
AST_EXPR_ASSIGN  
├─ left  → AST_EXPR_IDENTIFIER
│          └─ value.symbol_idx = 3   // Symbol table entry for "x"
└─ right → AST_LIT_INTEGER
           └─ value.long_value = 42
```

### Phase 3: Code Generation (TAC Generation)

During TAC generation, symbol table entries provide type and storage information:

```c
// Symbol table entry 3:
SymTabEntry {
    name_pos: 15,        // "x" in string store
    type: SYM_VARIABLE,
    type_idx: 1,         // int type
    scope_depth: 2,      // local variable
    // ... other fields
}

// Generated TAC:
// x = 42
TAC_ASSIGN
├─ result   → TAC_VAR(symbol_idx: 3)
├─ operand1 → TAC_IMM(value: 42)
└─ operand2 → TAC_NONE
```

### Implementation Guidelines

1. **Always check resolution state**: Use `symbol_idx != 0` to determine if identifier is resolved
2. **Preserve string references**: Keep `string_pos` for error reporting even after resolution
3. **Handle forward references**: Some identifiers may not be resolvable until later passes
4. **Scope-aware processing**: Always consider current scope when resolving identifiers
5. **Type consistency**: Ensure `type_idx` in AST nodes matches symbol table `type_idx`

---

## Practical Implementation Patterns

### Pattern 1: Processing Identifiers

```c
void process_identifier(ASTNode *node) {
    if (node->type != AST_EXPR_IDENTIFIER) return;
    
    // Check if identifier is resolved
    if (node->binary.value.symbol_idx != 0) {
        // Resolved - use symbol table
        SymTabEntry symbol = symtab_get(node->binary.value.symbol_idx);
        char *name = sstore_get(symbol.name_pos);
        
        printf("Resolved identifier: %s (type: %d, scope: %d)\n", 
               name, symbol.type, symbol.scope_depth);
               
        // Use symbol information for code generation
        generate_variable_access(&symbol);
        
    } else if (node->binary.value.string_pos != 0) {
        // Unresolved - try to resolve now
        char *name = sstore_get(node->binary.value.string_pos);
        
        SymIdx_t sym_idx = resolve_identifier(node->binary.value.string_pos, 
                                                current_scope);
        if (sym_idx != 0) {
            // Resolution successful
            node->binary.value.symbol_idx = sym_idx;
            process_identifier(node);  // Recursive call with resolved symbol
        } else {
            // Resolution failed - error handling
            error("Undefined identifier: %s", name);
        }
    } else {
        error("Invalid identifier node - no string_pos or symbol_idx");
    }
}
```

### Pattern 2: Processing Declarations

```c
void process_variable_declaration(ASTNode *decl_node) {
    if (decl_node->type != AST_VAR_DECL) return;
    
    // Get symbol table entry
    SymIdx_t sym_idx = decl_node->declaration.symbol_idx;
    SymTabEntry symbol = symtab_get(sym_idx);
    
    // Get variable name for debugging
    char *name = sstore_get(symbol.name_pos);
    
    // Process type information
    TypeIdx_t type_idx = decl_node->declaration.type_idx;
    // ... type processing ...
    
    // Process initializer if present
    ASTNodeIdx_t init_idx = decl_node->declaration.initializer;
    if (init_idx != 0) {
        ASTNode init_node = astore_get(init_idx);
        
        // Generate initialization code
        TACOperand init_value = process_expression(&init_node);
        TACOperand var_operand = create_variable_operand(sym_idx);
        
        tac_emit_assign(var_operand, init_value);
    }
    
    printf("Processed variable declaration: %s\n", name);
}
```

### Pattern 3: Scope-Aware Processing

```c
void process_compound_statement(ASTNode *compound) {
    if (compound->type != AST_STMT_COMPOUND) return;
    
    // Enter the compound statement's scope
    SymIdx_t scope_idx = compound->compound.scope_idx;
    enter_scope(scope_idx);
    
    // Process all statements in the compound
    ASTNodeIdx_t stmt_idx = compound->compound.statements;
    while (stmt_idx != 0) {
        ASTNode stmt = astore_get(stmt_idx);
        
        // Process current statement
        process_statement(&stmt);
        
        // Move to next statement in chain
        stmt_idx = stmt.children.child2;
    }
    
    // Exit the compound statement's scope
    exit_scope();
}
```

### Pattern 4: Function Call Processing

```c
void process_function_call(ASTNode *call_node) {
    if (call_node->type != AST_EXPR_CALL) return;
    
    // Get function being called
    ASTNodeIdx_t func_idx = call_node->call.function;
    ASTNode func_node = astore_get(func_idx);
    
    if (func_node.type == AST_EXPR_IDENTIFIER) {
        // Direct function call
        if (func_node.binary.value.symbol_idx != 0) {
            SymTabEntry func_symbol = symtab_get(func_node.binary.value.symbol_idx);
            char *func_name = sstore_get(func_symbol.name_pos);
            
            // Verify it's actually a function
            if (func_symbol.type != SYM_FUNCTION) {
                error("'%s' is not a function", func_name);
                return;
            }
            
            // Process arguments
            TACOperand args[MAX_ARGS];
            int arg_count = 0;
            
            ASTNodeIdx_t arg_idx = call_node->call.arguments;
            while (arg_idx != 0 && arg_count < MAX_ARGS) {
                ASTNode arg_node = astore_get(arg_idx);
                args[arg_count++] = process_expression(&arg_node);
                arg_idx = arg_node.children.child2;  // Next argument
            }
            
            // Generate function call TAC
            TACOperand result = tac_emit_call(&func_symbol, args, arg_count);
            
            printf("Processed function call: %s(%d args)\n", func_name, arg_count);
        } else {
            error("Unresolved function identifier");
        }
    }
}
```

---

## Common Implementation Pitfalls

### Pitfall 1: Not Checking Resolution State

```c
// WRONG - assumes identifier is always resolved
void bad_identifier_processing(ASTNode *node) {
    SymTabEntry symbol = symtab_get(node->binary.value.symbol_idx);  // Will get invalid data if symbol_idx is 0!
    // ... crashes or gets garbage data if symbol_idx is 0
}

// CORRECT - check resolution state first
void good_identifier_processing(ASTNode *node) {
    if (node->binary.value.symbol_idx != 0) {
        SymTabEntry symbol = symtab_get(node->binary.value.symbol_idx);
        // ... safe to use symbol
    } else {
        // Handle unresolved identifier
        char *name = sstore_get(node->binary.value.string_pos);
        error("Unresolved identifier: %s", name);
    }
}
```

### Pitfall 2: Incorrect Union Field Access

```c
// WRONG - using wrong union field for node type
void bad_if_processing(ASTNode *if_node) {
    // AST_STMT_IF uses 'conditional' structure, not 'children'
    ASTNodeIdx_t condition = if_node->children.child1;  // WRONG!
}

// CORRECT - use proper union field
void good_if_processing(ASTNode *if_node) {
    // AST_STMT_IF uses 'conditional' structure
    ASTNodeIdx_t condition = if_node->conditional.condition;  // CORRECT
    ASTNodeIdx_t then_stmt = if_node->conditional.then_stmt;
    ASTNodeIdx_t else_stmt = if_node->conditional.else_stmt;
}
```

### Pitfall 3: Ignoring Scope Context

```c
// WRONG - resolving identifier without scope context
SymIdx_t bad_resolve(sstore_pos_t name_pos) {
    // Searches entire symbol table without considering scope
    return global_symbol_search(name_pos);  // May return wrong symbol
}

// CORRECT - scope-aware resolution
SymIdx_t good_resolve(sstore_pos_t name_pos, SymIdx_t current_scope) {
    // Start from current scope and work upward
    return resolve_identifier_in_scope(name_pos, current_scope);
}
```

### Pitfall 4: Not Handling Statement Chaining

```c
// WRONG - processes only first statement
void bad_compound_processing(ASTNode *compound) {
    ASTNodeIdx_t stmt_idx = compound->compound.statements;
    if (stmt_idx != 0) {
        ASTNode stmt = astore_get(stmt_idx);
        process_statement(&stmt);  // Only processes first statement!
    }
}

// CORRECT - processes entire statement chain
void good_compound_processing(ASTNode *compound) {
    ASTNodeIdx_t stmt_idx = compound->compound.statements;
    while (stmt_idx != 0) {
        ASTNode stmt = astore_get(stmt_idx);
        process_statement(&stmt);
        stmt_idx = stmt.children.child2;  // Follow the chain
    }
}
```

---

## Debugging and Validation Techniques

### Technique 1: AST Structure Validation

```c
bool validate_ast_node(ASTNodeIdx_t node_idx) {
    if (node_idx == 0) return true;  // NULL reference is valid
    
    ASTNode node = astore_get(node_idx);
    
    // Validate node type is in valid range
    if (node.type < 0 || node.type > MAX_AST_NODE_TYPE) {
        printf("Invalid node type: %d\n", node.type);
        return false;
    }
    
    // Validate token reference
    if (node.token_idx != 0) {
        Token token = tstore_get(node.token_idx);
        if (token.id == T_INVALID) {
            printf("Invalid token reference in node %d\n", node_idx);
            return false;
        }
    }
    
    // Type-specific validation
    switch (node.type) {
        case AST_EXPR_IDENTIFIER:
            // Must have either string_pos or symbol_idx
            if (node.binary.value.string_pos == 0 && 
                node.binary.value.symbol_idx == 0) {
                printf("Identifier node has no name reference\n");
                return false;
            }
            break;
            
        case AST_STMT_IF:
            // Must have condition and then_stmt
            if (node.conditional.condition == 0 || 
                node.conditional.then_stmt == 0) {
                printf("IF statement missing required children\n");
                return false;
            }
            break;
            
        // ... other node types
    }
    
    return true;
}
```

### Technique 2: Symbol Table Consistency Check

```c
bool validate_symbol_consistency(ASTNode *node) {
    if (node->type == AST_EXPR_IDENTIFIER && 
        node->binary.value.symbol_idx != 0) {
        
        SymTabEntry symbol = symtab_get(node->binary.value.symbol_idx);
        char *ast_name = sstore_get(node->binary.value.string_pos);
        char *sym_name = sstore_get(symbol.name_pos);
        
        if (strcmp(ast_name, sym_name) != 0) {
            printf("Symbol table inconsistency: AST has '%s', symbol table has '%s'\n",
                   ast_name, sym_name);
            return false;
        }
    }
    
    return true;
}
```

This comprehensive documentation should provide everything needed to correctly implement AST node processing, including understanding the relationships between different storage systems, proper symbol resolution, and common implementation patterns.

## Node Type Categories

AST nodes are organized into logical categories based on their role in the language:

```
AST Node Types (0-255)
├── Special Nodes (0-9)        → Program structure, EOF, errors
├── Declaration Nodes (10-29)  → Functions, variables, types
├── Type Nodes (30-49)        → Type system representation
├── Statement Nodes (50-79)   → Control flow, blocks, jumps
├── Expression Nodes (80-129) → Operations, calls, literals
└── Literal Nodes (130-139)   → Constant values
```

---

## AST Node Memory Layout

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           AST Node (24 bytes total)                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                          Common Header (10 bytes)                          │
├──────────────────┬──────────────────┬──────────────────┬──────────────────┤
│ type             │ flags            │ token_idx        │ type_idx         │
│ (ASTNodeType)    │ (ASTNodeFlags)   │ (TokenIdx_t)     │ (TypeIdx_t)      │
│ 2 bytes          │ 2 bytes          │ 4 bytes          │ 2 bytes          │
├──────────────────┴──────────────────┴──────────────────┴──────────────────┤
│                       Union Data (14 bytes)                                │
│                    [One of the following structures]                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Visual Guide to Union Structures

Each node type uses one of these specialized data layouts:

#### 1. `children` Structure (Generic Multi-Child)
```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          'children' Structure                              │
├──────────────────┬──────────────────┬──────────────────┬──────────────────┤
│ child1           │ child2           │ child3           │ child4           │
│ (ASTNodeIdx_t)   │ (ASTNodeIdx_t)   │ (ASTNodeIdx_t)   │ (ASTNodeIdx_t)   │
│ 2 bytes          │ 2 bytes          │ 2 bytes          │ 2 bytes          │
├──────────────────┴──────────────────┴──────────────────┴──────────────────┤
│                           padding (6 bytes)                                │
└─────────────────────────────────────────────────────────────────────────────┘
```
**Used by**: AST_STMT_FOR, AST_EXPR_CONDITIONAL, AST_STMT_LABEL, AST_STMT_RETURN

#### 2. `binary` Structure (Two Children + Value)
```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           'binary' Structure                               │
├──────────────────┬──────────────────┬─────────────────────────────────────┤
│ left             │ right            │            value (union)            │
│ (ASTNodeIdx_t)   │ (ASTNodeIdx_t)   │              8 bytes                │
│ 2 bytes          │ 2 bytes          │  ┌─────────────────────────────────┐ │
├──────────────────┼──────────────────┤  │ symbol_idx (SymIdx_t)        │ │
│     padding      │                  │  │ string_pos (sstore_pos_t)       │ │
│    (2 bytes)     │                  │  │ long_value (int64_t)            │ │
│                  │                  │  │ float_value (double)            │ │
│                  │                  │  └─────────────────────────────────┘ │
└──────────────────┴──────────────────┴─────────────────────────────────────┘
```
**Used by**: AST_EXPR_BINARY_OP, AST_EXPR_ASSIGN, AST_LIT_*, AST_EXPR_IDENTIFIER

#### 3. `unary` Structure (Single Child + Operator)
```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           'unary' Structure                                │
├──────────────────┬──────────────────┬─────────────────┬───────────────────┤
│ operand          │ operator         │ data (union)    │     padding       │
│ (ASTNodeIdx_t)   │ (TokenID_t)      │    4 bytes      │    (6 bytes)      │
│ 2 bytes          │ 2 bytes          │ ┌─────────────┐ │                   │
│                  │                  │ │ int_value   │ │                   │
│                  │                  │ │ float_value │ │                   │
│                  │                  │ │ string_pos  │ │                   │
│                  │                  │ └─────────────┘ │                   │
└──────────────────┴──────────────────┴─────────────────┴───────────────────┘
```
**Used by**: AST_EXPR_UNARY_OP, AST_EXPR_SIZEOF, AST_TYPE_POINTER, AST_TYPE_QUALIFIER

#### 4. `compound` Structure (Block Statements)
```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         'compound' Structure                               │
├──────────────────┬──────────────────┬──────────────────┬──────────────────┤
│ declarations     │ statements       │ scope_idx        │     padding      │
│ (ASTNodeIdx_t)   │ (ASTNodeIdx_t)   │ (SymIdx_t)    │    (8 bytes)     │
│ 2 bytes          │ 2 bytes          │ 2 bytes          │                  │
│ [unused in C99]  │ → first stmt     │ → scope depth    │                  │
└──────────────────┴──────────────────┴──────────────────┴──────────────────┘
```
**Used by**: AST_STMT_COMPOUND

#### 5. `conditional` Structure (Control Flow)
```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       'conditional' Structure                              │
├──────────────────┬──────────────────┬──────────────────┬──────────────────┤
│ condition        │ then_stmt        │ else_stmt        │     padding      │
│ (ASTNodeIdx_t)   │ (ASTNodeIdx_t)   │ (ASTNodeIdx_t)   │    (8 bytes)     │
│ 2 bytes          │ 2 bytes          │ 2 bytes          │                  │
│ → bool expr      │ → true branch    │ → false branch   │                  │
└──────────────────┴──────────────────┴──────────────────┴──────────────────┘
```
**Used by**: AST_STMT_IF, AST_STMT_WHILE, AST_STMT_SWITCH

#### 6. `call` Structure (Function Calls)
```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           'call' Structure                                 │
├──────────────────┬──────────────────┬──────────────────┬─────┬─────────────┤
│ function         │ arguments        │ return_type      │ arg │   padding   │
│ (ASTNodeIdx_t)   │ (ASTNodeIdx_t)   │ (TypeIdx_t)      │count│  (7 bytes)  │
│ 2 bytes          │ 2 bytes          │ 2 bytes          │1 by │             │
│ → func name/ptr  │ → first arg      │ → return type    │     │             │
└──────────────────┴──────────────────┴──────────────────┴─────┴─────────────┘
```
**Used by**: AST_EXPR_CALL, AST_TYPE_FUNCTION

#### 7. `declaration` Structure (Variable/Function Declarations)
```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       'declaration' Structure                              │
├──────────────────┬──────────────────┬──────────────────┬─────┬─────────────┤
│ symbol_idx       │ type_idx         │ initializer      │stor │   padding   │
│ (SymIdx_t)    │ (TypeIdx_t)      │ (ASTNodeIdx_t)   │class│  (7 bytes)  │
│ 2 bytes          │ 2 bytes          │ 2 bytes          │1 by │             │
│ → symbol table   │ → type info      │ → init expr      │     │             │
└──────────────────┴──────────────────┴──────────────────┴─────┴─────────────┘
```
**Used by**: AST_VAR_DECL, AST_FUNCTION_DEF, AST_PARAM_DECL, AST_TYPEDEF_DECL
```

---

## AST Construction Patterns

### Pattern 1: Simple Expression Tree
```c
// Source: x + 5
AST_EXPR_BINARY_OP (root)
├─ left  → AST_EXPR_IDENTIFIER (x)
└─ right → AST_LIT_INTEGER (5)
```

### Pattern 2: Function Call
```c
// Source: printf("hello", x)
AST_EXPR_CALL
├─ function   → AST_EXPR_IDENTIFIER (printf)
├─ arguments  → AST_LIT_STRING ("hello")
│               └─ child2 → AST_EXPR_IDENTIFIER (x)
└─ arg_count  = 2
```

### Pattern 3: If Statement
```c
// Source: if (x > 0) { y = 1; } else { y = 0; }
AST_STMT_IF
├─ condition  → AST_EXPR_BINARY_OP (x > 0)
├─ then_stmt  → AST_STMT_COMPOUND
│               └─ statements → AST_EXPR_ASSIGN (y = 1)
└─ else_stmt  → AST_STMT_COMPOUND
                └─ statements → AST_EXPR_ASSIGN (y = 0)
```

### Pattern 4: Function Definition
```c
// Source: int add(int a, int b) { return a + b; }
AST_FUNCTION_DEF
├─ symbol_idx   → Symbol table entry for "add"
├─ type_idx     → Function type information
└─ initializer  → AST_STMT_COMPOUND (function body)
                  └─ statements → AST_STMT_RETURN
                                  └─ child1 → AST_EXPR_BINARY_OP (a + b)
```

---

## Union Structure Definitions

AST nodes use different union structures to efficiently pack data based on their specific needs:

### `children` Structure
**Size**: 14 bytes  
**Fields**:
- `child1` (ASTNodeIdx_t, 2 bytes): First child node
- `child2` (ASTNodeIdx_t, 2 bytes): Second child node  
- `child3` (ASTNodeIdx_t, 2 bytes): Third child node
- `child4` (ASTNodeIdx_t, 2 bytes): Fourth child node
- `padding` (6 bytes): Unused space

**Usage**: Generic nodes that need up to 4 child references (compound statements, conditional expressions, etc.)

### `binary` Structure  
**Size**: 14 bytes  
**Fields**:
- `left` (ASTNodeIdx_t, 2 bytes): Left operand/child
- `right` (ASTNodeIdx_t, 2 bytes): Right operand/child
- `value` (union, 8 bytes): Node-specific value data
  - `symbol_idx` (SymIdx_t): Symbol table reference
  - `string_pos` (sstore_pos_t): String store position
  - `long_value` (int64_t): Integer value
  - `float_value` (double): Floating-point value
- `padding` (2 bytes): Unused space

**Usage**: Binary operations, assignments, literals, identifiers

### `unary` Structure
**Size**: 14 bytes  
**Fields**:
- `operand` (ASTNodeIdx_t, 2 bytes): Operand node
- `operator` (TokenID_t, 2 bytes): Operator type
- `data` (union, 4 bytes): Additional data
  - `int_value` (int, 4 bytes): Integer data
  - `float_value` (float, 4 bytes): Float data  
  - `string_pos` (sstore_pos_t, 2 bytes): String position
- `padding` (6 bytes): Unused space

**Usage**: Unary operations, type qualifiers, storage specifiers

### `compound` Structure
**Size**: 14 bytes  
**Fields**:
- `declarations` (ASTNodeIdx_t, 2 bytes): First declaration (C89 style, unused in C99)
- `statements` (ASTNodeIdx_t, 2 bytes): First statement in block
- `scope_idx` (SymIdx_t, 2 bytes): Scope depth/identifier
- `padding` (8 bytes): Unused space

**Usage**: Compound statements (blocks with curly braces)

### `conditional` Structure  
**Size**: 14 bytes  
**Fields**:
- `condition` (ASTNodeIdx_t, 2 bytes): Condition expression
- `then_stmt` (ASTNodeIdx_t, 2 bytes): True/body statement
- `else_stmt` (ASTNodeIdx_t, 2 bytes): False statement (optional)
- `padding` (8 bytes): Unused space

**Usage**: If statements, while loops, switch statements

### `call` Structure
**Size**: 14 bytes  
**Fields**:
- `function` (ASTNodeIdx_t, 2 bytes): Function being called
- `arguments` (ASTNodeIdx_t, 2 bytes): First argument
- `return_type` (TypeIdx_t, 2 bytes): Expected return type
- `arg_count` (char, 1 byte): Number of arguments
- `padding` (7 bytes): Unused space

**Usage**: Function calls, function type definitions

### `declaration` Structure
**Size**: 14 bytes  
**Fields**:
- `symbol_idx` (SymIdx_t, 2 bytes): Symbol table entry
- `type_idx` (TypeIdx_t, 2 bytes): Type information
- `initializer` (ASTNodeIdx_t, 2 bytes): Initialization expression
- `storage_class` (char, 1 byte): Storage class specifier
- `padding` (7 bytes): Unused space

**Usage**: Variable declarations, function declarations, parameters

---

## Special Nodes (0-9)

These nodes handle program structure and special cases.

### AST_FREE (0)
**Meaning**: Unused/free node slot in the AST store  
**Structure**: N/A  
**Content**: All fields should be zero  
**Links**: None  
**Usage**: Internal memory management, not part of actual AST

### AST_PROGRAM (1)
**Meaning**: Root node of the entire program/translation unit  
**Structure**: [`children`](#1-children-structure-generic-multi-child)  
**Parameters**:
- `child1`: First external declaration (function/variable)
- `child2-4`: Unused (0)
**Links**: 
- `child1` → First function/variable declaration in program
- External declarations are chained via their `child2` fields
**Example**:
```c
// Program with main function
AST_PROGRAM
└─ child1 → AST_FUNCTION_DEF (main)
```

### AST_TRANSLATION_UNIT (2)  
**Meaning**: Alternative root for translation units (C99 standard term)  
**Structure**: [`children`](#1-children-structure-generic-multi-child)  
**Parameters**: Same as AST_PROGRAM  
**Content**: Similar to AST_PROGRAM  
**Links**: Links to external declarations  

### AST_EOF (3)
**Meaning**: End of file marker during parsing  
**Structure**: [`children`](#1-children-structure-generic-multi-child)  
**Parameters**: All fields typically 0  
**Content**: Token reference to EOF token  
**Links**: None  

### AST_ERROR (4)
**Meaning**: Error recovery node for syntax errors  
**Structure**: [`children`](#1-children-structure-generic-multi-child)  
**Parameters**: May contain references to partially parsed subtrees  
**Content**: Error information in token reference  
**Links**: May link to partially parsed subtrees for error recovery  

---

## Declaration Nodes (10-29)

These nodes represent all forms of declarations in C: functions, variables, types, etc.

### AST_FUNCTION_DECL (10)
**Meaning**: Function declaration (prototype only, no body)  
**Structure**: [`declaration`](#7-declaration-structure-variablefunction-declarations)  
**Parameters**:
- `symbol_idx` → Symbol table entry for function name
- `type_idx` → Function type information (return type + parameters)
- `initializer`: 0 (no function body)
- `storage_class`: Storage class specifier (static, extern, etc.)
**Links**: None (declaration only)
**Example**:
```c
// int printf(char *fmt, ...);
AST_FUNCTION_DECL
├─ symbol_idx   → "printf" in symbol table
├─ type_idx     → Function type (int, char*, ...)
├─ initializer  = 0 (no body)
└─ storage_class = extern
```

### AST_FUNCTION_DEF (11)
**Meaning**: Function definition with body  
**Structure**: [`declaration`](#7-declaration-structure-variablefunction-declarations)  
**Parameters**:
- `symbol_idx` → Symbol table entry for function name
- `type_idx` → Function type information
- `initializer` → Function body (AST_STMT_COMPOUND)
- `storage_class`: Storage class specifier
**Links**:
- `initializer` → AST_STMT_COMPOUND (function body)
**Example**:
```c
// int main() { return 0; }
AST_FUNCTION_DEF
├─ symbol_idx   → "main" in symbol table
├─ type_idx     → Function type (int, void)
├─ initializer  → AST_STMT_COMPOUND (body)
└─ storage_class = 0 (default)
```

### AST_VAR_DECL (12)
**Meaning**: Variable declaration  
**Structure**: [`declaration`](#7-declaration-structure-variablefunction-declarations)  
**Parameters**:
- `symbol_idx` → Symbol table entry for variable
- `type_idx` → Variable type information
- `initializer` → Initialization expression (0 if none)
- `storage_class`: Storage class (auto, static, extern, register)
**Links**:
- `initializer` → Expression node for initialization (optional)
- Chained via `child2` in compound statements
**Example**:
```c
// int x = 42;
AST_VAR_DECL
├─ symbol_idx   → "x" in symbol table
├─ type_idx     → int type
├─ initializer  → AST_LIT_INTEGER (42)
└─ storage_class = auto
```

### AST_PARAM_DECL (13)
**Meaning**: Function parameter declaration  
**Structure**: [`declaration`](#7-declaration-structure-variablefunction-declarations)  
**Parameters**:
- `symbol_idx` → Parameter symbol in local scope
- `type_idx` → Parameter type
- `initializer`: 0 (parameters can't have initializers)
- `storage_class`: Usually 0 (register is allowed)
**Links**: Chained in parameter lists via `child2`
**Example**:
```c
// void func(int count, char *name)
//           ^^^^^^^^^  ^^^^^^^^^^^
//           param 1    param 2 (chained)
AST_PARAM_DECL
├─ symbol_idx   → "count" in symbol table
├─ type_idx     → int type
├─ initializer  = 0
├─ storage_class = 0
└─ child2       → AST_PARAM_DECL ("name")
```

### AST_FIELD_DECL (14)
**Meaning**: Struct/union field declaration  
**Structure**: [`declaration`](#7-declaration-structure-variablefunction-declarations)  
**Parameters**:
- `symbol_idx` → Field name in symbol table
- `type_idx` → Field type
- `initializer`: 0 (fields can't have initializers)
- `storage_class`: 0 (not applicable to fields)
**Links**: Chained in struct/union definitions

### AST_TYPEDEF_DECL (15)
**Meaning**: Type definition (typedef)  
**Structure**: [`declaration`](#7-declaration-structure-variablefunction-declarations)  
**Parameters**:
- `symbol_idx` → New type name in symbol table
- `type_idx` → Actual type being aliased
- `initializer`: 0 (not applicable)
- `storage_class`: 0 (typedef is not a storage class)
**Links**: Links to aliased type through type system

### AST_STRUCT_DECL (16)
**Meaning**: Structure declaration/definition  
**Structure**: [`declaration`](#7-declaration-structure-variablefunction-declarations)  
**Parameters**:
- `symbol_idx` → Struct tag name (0 for anonymous)
- `type_idx` → Struct type information
- `initializer` → First field declaration (0 for forward declaration)
- `storage_class`: 0 (not applicable)
**Links**: Links to field declarations

### AST_UNION_DECL (17)
**Meaning**: Union declaration/definition  
**Structure**: [`declaration`](#7-declaration-structure-variablefunction-declarations)  
**Parameters**: Similar to AST_STRUCT_DECL
**Links**: Links to member declarations

### AST_ENUM_DECL (18)
**Meaning**: Enumeration declaration/definition  
**Structure**: [`declaration`](#7-declaration-structure-variablefunction-declarations)  
**Parameters**:
- `symbol_idx` → Enum tag name (0 for anonymous)
- `type_idx` → Enum type (usually int)
- `initializer` → First enum constant
- `storage_class`: 0
**Links**: Links to enum constants

### AST_ENUM_CONSTANT (19)
**Meaning**: Enumeration constant  
**Structure**: [`declaration`](#7-declaration-structure-variablefunction-declarations)  
**Parameters**:
- `symbol_idx` → Constant name in symbol table
- `type_idx` → int type (enums are int in C)
- `initializer` → Value expression (0 for auto-increment)
- `storage_class`: 0
**Links**: Chained in enum declarations

---

## Type Nodes (30-49)

### AST_TYPE_BASIC (30)
**Meaning**: Basic types (int, char, float, etc.)  
**Structure**: [`unary`](#unary-structure)  
**Content**: Base type information  
**Links**: None (leaf nodes)

### AST_TYPE_POINTER (31)
**Meaning**: Pointer type  
**Structure**: [`unary`](#unary-structure)  
**Content**:
- `operand` → Pointed-to type
**Links**:
- `operand` → Target type node

### AST_TYPE_ARRAY (32)
**Meaning**: Array type  
**Structure**: [`binary`](#binary-structure)  
**Content**:
- `left` → Element type
- `right` → Size expression
**Links**:
- `left` → Element type node  
- `right` → Size expression node

### AST_TYPE_FUNCTION (33)
**Meaning**: Function type  
**Structure**: [`call`](#call-structure)  
**Content**:
- `function` → Return type
- `arguments` → Parameter types
**Links**:
- `function` → Return type node
- `arguments` → Parameter type list

### AST_TYPE_STRUCT (34)
**Meaning**: Structure type  
**Structure**: [`declaration`](#declaration-structure)  
**Content**: Struct type metadata  
**Links**: Links to struct declaration

### AST_TYPE_UNION (35)
**Meaning**: Union type  
**Structure**: [`declaration`](#declaration-structure)  
**Content**: Union type metadata  
**Links**: Links to union declaration

### AST_TYPE_ENUM (36)
**Meaning**: Enumeration type  
**Structure**: [`declaration`](#declaration-structure)  
**Content**: Enum type metadata  
**Links**: Links to enum declaration

### AST_TYPE_TYPEDEF (37)
**Meaning**: Typedef type  
**Structure**: [`declaration`](#declaration-structure)  
**Content**: Typedef metadata  
**Links**: Links to actual type

### AST_TYPE_QUALIFIER (38)
**Meaning**: Type qualifiers (const, volatile)  
**Structure**: [`unary`](#unary-structure)  
**Content**:
- `operand` → Qualified type
- `operator` → Qualifier type
**Links**:
- `operand` → Base type node

### AST_TYPE_STORAGE (39)
**Meaning**: Storage class specifiers (static, extern, etc.)  
**Structure**: [`unary`](#unary-structure)  
**Content**: Storage class information  
**Links**: Links to type being modified

---

## Statement Nodes (50-79)

These nodes represent executable statements and control flow constructs.

### AST_STMT_COMPOUND (50)
**Meaning**: Block statement with curly braces `{ ... }`  
**Structure**: [`compound`](#4-compound-structure-block-statements)  
**Parameters**:
- `declarations`: 0 (unused in C99 mixed mode)
- `statements` → First statement/declaration in block
- `scope_idx` → Scope depth for symbol table
**Links**:
- `statements` → First statement/declaration in block
- Statements chained via their `child2` fields
**Example**:
```c
// { int x = 5; printf("%d", x); }
AST_STMT_COMPOUND
├─ declarations = 0 (C99)
├─ statements   → AST_VAR_DECL (int x = 5)
│                 └─ child2 → AST_EXPR_CALL (printf)
└─ scope_idx    = 2 (nested scope)
```

### AST_STMT_EXPRESSION (51)
**Meaning**: Expression statement (expression followed by semicolon)  
**Structure**: [`children`](#1-children-structure-generic-multi-child)  
**Parameters**:
- `child1` → Expression to evaluate
- `child2` → Next statement (for chaining)
- `child3-4`: 0 (unused)
**Links**:
- `child1` → Expression node
- `child2` → Next statement in sequence
**Example**:
```c
// x = 42;
AST_STMT_EXPRESSION
├─ child1 → AST_EXPR_ASSIGN (x = 42)
└─ child2 → next statement (chaining)
```

### AST_STMT_IF (52)
**Meaning**: If statement with optional else clause  
**Structure**: [`conditional`](#5-conditional-structure-control-flow)  
**Parameters**:
- `condition` → Boolean expression to test
- `then_stmt` → Statement executed if condition is true
- `else_stmt` → Statement executed if condition is false (0 if no else)
**Links**:
- `condition` → Expression node (must evaluate to scalar)
- `then_stmt` → Statement node (often AST_STMT_COMPOUND)
- `else_stmt` → Statement node (optional)
**Example**:
```c
// if (x > 0) y = 1; else y = 0;
AST_STMT_IF
├─ condition  → AST_EXPR_BINARY_OP (x > 0)
├─ then_stmt  → AST_EXPR_ASSIGN (y = 1)
└─ else_stmt  → AST_EXPR_ASSIGN (y = 0)
```

### AST_STMT_WHILE (53)
**Meaning**: While loop  
**Structure**: [`conditional`](#5-conditional-structure-control-flow)  
**Parameters**:
- `condition` → Loop continuation condition
- `then_stmt` → Loop body statement
- `else_stmt`: 0 (unused for while loops)
**Links**:
- `condition` → Expression node (loop test)
- `then_stmt` → Statement node (loop body)
**Example**:
```c
// while (i < 10) { i++; }
AST_STMT_WHILE
├─ condition  → AST_EXPR_BINARY_OP (i < 10)
├─ then_stmt  → AST_STMT_COMPOUND ({ i++; })
└─ else_stmt  = 0
```

### AST_STMT_FOR (54)
**Meaning**: For loop with init, condition, and increment  
**Structure**: [`children`](#1-children-structure-generic-multi-child)  
**Parameters**:
- `child1` → Initialization statement/expression (can be declaration)
- `child2` → Loop condition expression
- `child3` → Increment/update expression
- `child4` → Loop body statement
**Links**:
- `child1` → Init expression/declaration (0 if empty)
- `child2` → Condition expression (0 for infinite loop)
- `child3` → Increment expression (0 if empty)
- `child4` → Loop body statement
**Example**:
```c
// for (int i = 0; i < 10; i++) sum += i;
AST_STMT_FOR
├─ child1 → AST_VAR_DECL (int i = 0)
├─ child2 → AST_EXPR_BINARY_OP (i < 10)
├─ child3 → AST_EXPR_UNARY_OP (i++)
└─ child4 → AST_EXPR_ASSIGN (sum += i)
```

### AST_STMT_DO_WHILE (55)
**Meaning**: Do-while loop (body executes at least once)  
**Structure**: [`conditional`](#5-conditional-structure-control-flow)  
**Parameters**:
- `condition` → Loop continuation condition (tested after body)
- `then_stmt` → Loop body statement
- `else_stmt`: 0 (unused)
**Links**:
- `then_stmt` → Statement node (loop body)
- `condition` → Expression node (post-test condition)

### AST_STMT_SWITCH (56)
**Meaning**: Switch statement  
**Structure**: [`conditional`](#5-conditional-structure-control-flow)  
**Parameters**:
- `condition` → Switch expression (must be integer type)
- `then_stmt` → Switch body (compound statement with case labels)
- `else_stmt`: 0 (unused)
**Links**:
- `condition` → Expression node (switch value)
- `then_stmt` → Compound statement containing case labels

### AST_STMT_CASE (57)
**Meaning**: Case label in switch statement  
**Structure**: [`conditional`](#5-conditional-structure-control-flow)  
**Parameters**:
- `condition` → Case value (constant expression)
- `then_stmt` → Statement following the case label
- `else_stmt`: 0 (unused)
**Links**:
- `condition` → Constant expression
- `then_stmt` → Following statement

### AST_STMT_DEFAULT (58)
**Meaning**: Default label in switch statement  
**Structure**: [`children`](#1-children-structure-generic-multi-child)  
**Parameters**:
- `child1` → Statement following the default label
- `child2-4`: 0 (unused)
**Links**:
- `child1` → Following statement

### AST_STMT_BREAK (59)
**Meaning**: Break statement (exit loop or switch)  
**Structure**: [`children`](#1-children-structure-generic-multi-child)  
**Parameters**: All children 0 (no additional data needed)
**Content**: Token reference for source location
**Links**: None (control flow statement)

### AST_STMT_CONTINUE (60)
**Meaning**: Continue statement (skip to next loop iteration)  
**Structure**: [`children`](#1-children-structure-generic-multi-child)  
**Parameters**: All children 0
**Content**: Token reference for source location  
**Links**: None

### AST_STMT_RETURN (61)
**Meaning**: Return statement with optional value  
**Structure**: [`children`](#1-children-structure-generic-multi-child)  
**Parameters**:
- `child1` → Return expression (0 for void return)
- `child2-4`: 0 (unused)
**Links**:
- `child1` → Expression node (optional)
**Example**:
```c
// return x + 1;
AST_STMT_RETURN
├─ child1 → AST_EXPR_BINARY_OP (x + 1)
└─ child2-4 = 0

// return; (void)
AST_STMT_RETURN
└─ child1-4 = 0
```

### AST_STMT_GOTO (62)
**Meaning**: Goto statement  
**Structure**: [`children`](#1-children-structure-generic-multi-child)  
**Parameters**:
- `child1` → Target label identifier
- `child2-4`: 0
**Links**:
- `child1` → Label identifier node

### AST_STMT_LABEL (63)
**Meaning**: Statement label for goto  
**Structure**: [`children`](#1-children-structure-generic-multi-child)  
**Parameters**:
- `child1` → Labeled statement
- `child2-4`: 0
**Content**: Label name in token reference
**Links**:
- `child1` → Following statement

### AST_STMT_EMPTY (64)
**Meaning**: Empty statement (just semicolon `;`)  
**Structure**: [`children`](#1-children-structure-generic-multi-child)  
**Parameters**: All children 0
**Content**: Token reference to semicolon
**Links**: None

---

## Expression Nodes (80-129)

### AST_EXPR_LITERAL (80)
**Meaning**: Generic literal expression  
**Structure**: [`binary`](#binary-structure)  
**Content**: Literal value  
**Links**: None (leaf nodes)

### AST_EXPR_IDENTIFIER (81)
**Meaning**: Variable/function identifier  
**Structure**: [`binary`](#binary-structure)  
**Content**:
- `value.symbol_idx` → Symbol table entry (if resolved)
- `value.string_pos` → Name in string store (if unresolved)
**Links**:
- Links to symbol table entry

### AST_EXPR_BINARY_OP (82)
**Meaning**: Binary operators (+, -, *, /, <, >, etc.)  
**Structure**: [`binary`](#binary-structure)  
**Content**:
- `left` → Left operand
- `right` → Right operand
**Links**:
- `left` → Expression node
- `right` → Expression node

### AST_EXPR_UNARY_OP (83)
**Meaning**: Unary operators (+, -, !, ~, ++, --, etc.)  
**Structure**: [`unary`](#unary-structure)  
**Content**:
- `operand` → Operand expression
- `operator` → Operator type (TokenID)
**Links**:
- `operand` → Expression node

### AST_EXPR_ASSIGN (84)
**Meaning**: Assignment expressions (=, +=, -=, etc.)  
**Structure**: [`binary`](#binary-structure)  
**Content**:
- `left` → Left-hand side (lvalue)
- `right` → Right-hand side (rvalue)
**Links**:
- `left` → Expression node (assignable)
- `right` → Expression node

### AST_EXPR_CALL (85)
**Meaning**: Function call  
**Structure**: [`call`](#call-structure)  
**Content**:
- `function` → Function being called
- `arguments` → First argument
- `arg_count` → Number of arguments
- `return_type` → Expected return type
**Links**:
- `function` → Expression node (function identifier/pointer)
- `arguments` → First argument expression
- Arguments chained via `child2` fields

### AST_EXPR_MEMBER (86)
**Meaning**: Member access with dot operator (.)  
**Structure**: [`binary`](#binary-structure)  
**Content**:
- `left` → Structure/union expression
- `right` → Member identifier
**Links**:
- `left` → Expression node
- `right` → Identifier node

### AST_EXPR_MEMBER_PTR (87)
**Meaning**: Member access with arrow operator (->)  
**Structure**: [`binary`](#binary-structure)  
**Content**:
- `left` → Pointer expression
- `right` → Member identifier
**Links**:
- `left` → Expression node
- `right` → Identifier node

### AST_EXPR_INDEX (88)
**Meaning**: Array subscript operator ([])  
**Structure**: [`binary`](#binary-structure)  
**Content**:
- `left` → Array expression
- `right` → Index expression
**Links**:
- `left` → Expression node
- `right` → Expression node

### AST_EXPR_CAST (89)
**Meaning**: Type cast expression  
**Structure**: [`binary`](#binary-structure)  
**Content**:
- `left` → Target type
- `right` → Expression to cast
**Links**:
- `left` → Type node
- `right` → Expression node

### AST_EXPR_SIZEOF (90)
**Meaning**: Sizeof operator  
**Structure**: [`unary`](#unary-structure)  
**Content**:
- `operand` → Type or expression
**Links**:
- `operand` → Type or expression node

### AST_EXPR_CONDITIONAL (91)
**Meaning**: Ternary conditional operator (? :)  
**Structure**: [`children`](#children-structure)  
**Content**:
- `child1` → Condition
- `child2` → True expression
- `child3` → False expression
**Links**:
- `child1` → Expression node
- `child2` → Expression node
- `child3` → Expression node

### AST_EXPR_COMMA (92)
**Meaning**: Comma operator  
**Structure**: [`binary`](#binary-structure)  
**Content**:
- `left` → Left expression
- `right` → Right expression
**Links**:
- `left` → Expression node
- `right` → Expression node

### AST_EXPR_INIT_LIST (93)
**Meaning**: Initializer list {1, 2, 3}  
**Structure**: [`children`](#children-structure)  
**Content**:
- `child1` → First initializer
**Links**:
- `child1` → First expression in list
- List elements chained via `child2` fields

### AST_EXPR_COMPOUND_LITERAL (94)
**Meaning**: Compound literal (Type){init}  
**Structure**: [`binary`](#binary-structure)  
**Content**:
- `left` → Type
- `right` → Initializer list
**Links**:
- `left` → Type node
- `right` → Initializer list node

---

## Literal Nodes (130-139)

### AST_LIT_INTEGER (130)
**Meaning**: Integer literal  
**Structure**: [`binary`](#binary-structure)  
**Content**:
- `value.long_value` → Integer value
**Links**: None (leaf node)

### AST_LIT_FLOAT (131)
**Meaning**: Floating-point literal  
**Structure**: [`binary`](#binary-structure)  
**Content**:
- `value.float_value` → Float value
**Links**: None (leaf node)

### AST_LIT_CHAR (132)
**Meaning**: Character literal  
**Structure**: [`binary`](#binary-structure)  
**Content**:
- `value.long_value` → Character value (as int)
**Links**: None (leaf node)

### AST_LIT_STRING (133)
**Meaning**: String literal  
**Structure**: [`binary`](#binary-structure)  
**Content**:
- `value.string_pos` → String position in string store
**Links**: Links to string store entry

---

## AST Traversal and Debugging Guide

### Common Traversal Patterns

#### 1. Depth-First Traversal (Expression Trees)
```c
void traverse_expression(ASTNodeIdx_t node_idx) {
    if (node_idx == 0) return;
    
    ASTNode node = ast_get(node_idx);
    
    switch (node.type) {
        case AST_EXPR_BINARY_OP:
            traverse_expression(node.binary.left);   // Left operand
            traverse_expression(node.binary.right);  // Right operand
            process_binary_op(&node);               // Post-order processing
            break;
            
        case AST_EXPR_UNARY_OP:
            traverse_expression(node.unary.operand); // Operand first
            process_unary_op(&node);                // Then operator
            break;
            
        case AST_EXPR_IDENTIFIER:
        case AST_LIT_INTEGER:
            process_leaf_node(&node);               // Leaf nodes
            break;
    }
}
```

#### 2. Statement Chain Traversal
```c
void traverse_statement_chain(ASTNodeIdx_t stmt_idx) {
    ASTNodeIdx_t current = stmt_idx;
    
    while (current != 0) {
        ASTNode stmt = ast_get(current);
        
        // Process current statement
        process_statement(&stmt);
        
        // Move to next statement in chain
        current = stmt.children.child2;  // Most statements chain via child2
    }
}
```

#### 3. Function Body Traversal
```c
void traverse_function(ASTNodeIdx_t func_idx) {
    ASTNode func = ast_get(func_idx);
    
    if (func.type == AST_FUNCTION_DEF) {
        // Function body is in initializer field
        ASTNodeIdx_t body_idx = func.declaration.initializer;
        
        if (body_idx != 0) {
            ASTNode body = ast_get(body_idx);
            
            if (body.type == AST_STMT_COMPOUND) {
                // Enter function scope
                enter_scope(body.compound.scope_idx);
                
                // Traverse all statements in function body
                traverse_statement_chain(body.compound.statements);
                
                // Exit function scope
                exit_scope();
            }
        }
    }
}
```

### Debugging with cc1t

The `cc1t` tool provides visual AST inspection. Key features:

1. **Tree Structure View**: Shows parent-child relationships
2. **Flat Node View**: Lists all nodes with their parameters
3. **Cycle Detection**: Identifies corrupted AST structures
4. **Symbol Integration**: Shows symbol table references

### Common AST Issues and Fixes

#### Issue 1: Cycle Detection
```
├─ [15] STMT_COMPOUND (decls:0, stmts:20, scope:3)
│  └─ [20] EXPR_ASSIGN (16,19,0,0)
└─ [20] **CYCLE DETECTED**
```
**Cause**: Parser creating circular references  
**Fix**: Check statement chaining logic in parser

#### Issue 2: Missing Child Links
```
├─ [14] STMT_IF (7,0,15,0)  ← then_stmt is 0 (missing)
```
**Cause**: Parser not setting required child fields  
**Fix**: Ensure all required fields are populated during parsing

#### Issue 3: Wrong Structure Type
```
AST_STMT_IF using 'children' instead of 'conditional'
```
**Cause**: Node created with wrong structure type  
**Fix**: Use correct create_ast_node() call with proper structure

### Memory Layout Verification

To verify correct AST structure:

1. **Size Check**: All nodes must be exactly 24 bytes
2. **Alignment**: Nodes should be properly aligned in memory
3. **Index Validity**: All ASTNodeIdx_t values should be valid indices
4. **Type Consistency**: Node type should match its union structure usage

---

## Node Linking Patterns

### Sequential Chaining
- **Statements in compound blocks**: Linked via `child2` field
- **Function arguments**: Linked via `child2` field  
- **Parameter lists**: Linked via `child2` field
- **External declarations**: Linked via `child2` field

### Hierarchical Linking
- **Program structure**: AST_PROGRAM → declarations → statements → expressions
- **Function definitions**: AST_FUNCTION_DEF → AST_STMT_COMPOUND → statements
- **Control flow**: AST_STMT_IF → condition + then_stmt + else_stmt

### Symbol Table Integration
- **Variable references**: AST_EXPR_IDENTIFIER.symbol_idx → SymTabEntry
- **Function calls**: AST_EXPR_CALL.function → identifier → symbol
- **Declarations**: AST_VAR_DECL.symbol_idx → SymTabEntry

## Type System Architecture

**VALIDATED**: STCC1's type system is embedded across multiple components rather than centralized.

### How Types Are Managed

The type system in STCC1 operates through **distributed type information** across several components:

#### 1. **Token-Based Type Foundation**
- **Basic Types**: Defined as lexer tokens (`T_INT`, `T_CHAR`, `T_FLOAT`, `T_VOID`, etc.)
- **Direct Mapping**: Token IDs serve as the foundation for TypeIdx_t values
- **Location**: `src/lexer/ctoken.h`

#### 2. **TypeIdx_t System**  
- **Definition**: `typedef unsigned short TypeIdx_t` (uint16_t)
- **Usage**: Universal type identifier throughout compilation pipeline
- **Range**: 0-65535 (0 = unspecified type)

#### 3. **Symbol Table Type Storage**
```c
typedef struct SymTabEntry {
    // ... other fields ...
    TypeIdx_t type_idx;         // Detailed type information index
    unsigned int flags;         // C99 type qualifiers (const, volatile, etc.)
    // ... other fields ...
} SymTabEntry;
```

#### 4. **AST Node Type Integration**
- **Type Nodes**: Specialized AST nodes for type information
  - `AST_TYPE_BASIC` (30): Basic types (int, char, float, etc.)
  - `AST_TYPE_POINTER` (31): Pointer types
  - `AST_TYPE_ARRAY` (32): Array types  
  - `AST_TYPE_FUNCTION` (33): Function types
  - `AST_TYPE_STRUCT` (34): Struct types
  - `AST_TYPE_UNION` (35): Union types
  - `AST_TYPE_ENUM` (36): Enum types

- **Type Fields**: Every AST node can carry type information
```c
typedef struct ASTNode {
    // ... other fields ...
    TypeIdx_t type_idx;     // Type information index
    // ... other fields ...
} ASTNode;
```

### Type Resolution Pipeline

```
1. Lexer: T_INT, T_CHAR, etc. → Token IDs
2. Parser: Token IDs → TypeSpecifier_t → TypeIdx_t  
3. Symbol Table: TypeIdx_t stored in SymTabEntry.type_idx
4. AST Builder: TypeIdx_t assigned to AST nodes
5. TAC Generator: TypeIdx_t used for type checking and code generation
```

### Type System Components

| Component | Role | Type Information |
|-----------|------|------------------|
| **Lexer** | Type Recognition | Token IDs (T_INT, T_CHAR, etc.) |
| **Symbol Table** | Type Storage | TypeIdx_t + C99 flags |
| **AST Nodes** | Type Representation | AST_TYPE_* nodes + TypeIdx_t |
| **TAC Generator** | Type Usage | TypeIdx_t for temporaries and operations |

### Example Type Flow
```c
// Source: int x = 5;
T_INT → TypeIdx_t(T_INT) → SymTabEntry{type_idx: T_INT} → AST_DECLARATION{type_idx: T_INT}
```

**Key Insight**: STCC1 uses a **hybrid approach** where basic types are token-based, complex types are AST-based, and all type information is unified through TypeIdx_t identifiers.

### Integration with AST Nodes
- **Type information**: Node.type_idx → TypeIdx (semantic analysis phase)
- **Cast expressions**: AST_EXPR_CAST.left → type information
- **Function types**: Return and parameter type linking

---

## Memory Layout Notes

- All nodes are exactly 24 bytes (when properly aligned)
- Union structures allow efficient memory usage
- Token references enable source location tracking
- Index-based linking avoids pointer management issues
- Phase flags track compilation progress

This reference should be consulted when implementing AST traversal, code generation, or debugging tools like cc1t.
