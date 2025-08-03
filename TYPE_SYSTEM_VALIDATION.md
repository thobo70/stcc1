## STCC1 Type System Validation Report

### Question: "How is the type system managed? Is it covered by the symbol table?"

### Answer: **Distributed Type System Architecture**

STCC1 uses a **distributed type system** rather than a centralized type store. Here's how it works:

## Type System Components

### 1. **Foundation: Lexer Tokens**
- Basic types defined as tokens: `T_INT`, `T_CHAR`, `T_FLOAT`, `T_VOID`, etc.
- Located in: `src/lexer/ctoken.h`
- These token IDs serve directly as TypeIdx_t values for basic types

### 2. **Universal Identifier: TypeIdx_t**
```c
typedef unsigned short TypeIdx_t;   // uint16_t, range 0-65535
```
- **Usage**: Universal type identifier throughout compilation pipeline
- **Value 0**: Represents unspecified/unknown type
- **Other values**: Token IDs for basic types, or indices for complex types

### 3. **Symbol Table Integration**
```c
typedef struct SymTabEntry {
    // ... other fields ...
    TypeIdx_t type_idx;         // Type information for this symbol
    unsigned int flags;         // C99 type qualifiers (const, volatile, etc.)
    // ... other fields ...
} SymTabEntry;
```
- **Role**: Stores type information for every declared symbol
- **C99 Support**: Type qualifier flags (SYM_FLAG_CONST, SYM_FLAG_VOLATILE, etc.)
- **Location**: `src/storage/symtab.h`

### 4. **AST Type Nodes**
- **Type Nodes**: Dedicated AST node types for complex type structures
  - `AST_TYPE_BASIC` (30): Basic types (int, char, float, etc.)
  - `AST_TYPE_POINTER` (31): Pointer types  
  - `AST_TYPE_ARRAY` (32): Array types
  - `AST_TYPE_FUNCTION` (33): Function signatures
  - `AST_TYPE_STRUCT` (34): Struct definitions
  - `AST_TYPE_UNION` (35): Union definitions
  - `AST_TYPE_ENUM` (36): Enum definitions

- **Node Integration**: Every AST node can carry type information via `type_idx` field

### 5. **TAC Integration**
- **Type Propagation**: TypeIdx_t values used in TAC generation for type checking
- **Temporary Variables**: TAC temporaries created with associated TypeIdx_t
- **Location**: `src/ir/tac_builder.c`

## Type Resolution Pipeline

```
1. Lexer:         T_INT, T_CHAR, etc. → Token IDs
2. Parser:        Token IDs → TypeSpecifier_t → TypeIdx_t  
3. Symbol Table:  TypeIdx_t → SymTabEntry.type_idx
4. AST Builder:   TypeIdx_t → ASTNode.type_idx
5. TAC Generator: TypeIdx_t → Type checking & code generation
```

## Example Type Flow
```c
// Source code: int x = 5;
// Flow:
T_INT (token) → TypeIdx_t(T_INT) → SymTabEntry{type_idx: T_INT} → AST_DECLARATION{type_idx: T_INT}
```

## Key Findings

### ✅ **Symbol Table Role**
- **Yes**, the symbol table plays a central role in type management
- Stores `TypeIdx_t` for every symbol in `SymTabEntry.type_idx`
- Handles C99 type qualifiers through flag system

### ✅ **No Separate Type Store**
- STCC1 does **not** have a dedicated "Type Store" 
- Type information is distributed across multiple components
- This was the error in the original documentation

### ✅ **Hybrid Approach**
- **Basic types**: Token-based (T_INT → TypeIdx_t)
- **Complex types**: AST-based (AST_TYPE_* nodes + TypeIdx_t)
- **Integration**: TypeIdx_t unifies both approaches

## Architecture Validation

**Source Evidence**:
- `src/storage/symtab.h`: SymTabEntry structure with type_idx field
- `src/ast/ast_types.h`: TypeIdx_t definition and AST type nodes
- `src/parser/cc1.c`: Type resolution from tokens to TypeIdx_t
- `src/ir/tac_builder.c`: TypeIdx_t usage in TAC generation

**Conclusion**: STCC1's type system is a well-designed distributed architecture that efficiently handles both simple and complex types through a unified TypeIdx_t identifier system, with the symbol table serving as a key component for type storage and retrieval.
