# Three-Address Code Design for STCC1 Compiler

## Table of Contents
1. [Overview](#overview)
2. [Architecture Integration](#architecture-integration)
3. [Instruction Format](#instruction-format)
4. [Instruction Set](#instruction-set)
5. [Data Structures](#data-structures)
6. [Memory Management](#memory-management)
7. [Code Generation](#code-generation)
8. [Optimization Framework](#optimization-framework)
9. [Implementation Plan](#implementation-plan)
10. [Example Translations](#example-translations)

---

## Overview

### Purpose
Design a **Three-Address Code (TAC)** intermediate representation for the STCC1 small C compiler that:
- **Integrates seamlessly** with existing AST and storage systems
- **Maintains memory efficiency** (<100KB RAM target)
- **Enables optimization phases** between parsing and code generation
- **Simplifies target code generation** for multiple architectures

### Design Principles
1. **Memory Efficiency**: File-backed storage, compact representation
2. **AST Integration**: Direct translation from existing AST nodes
3. **Optimization Ready**: SSA-friendly with def-use chains
4. **Simplicity**: Clear instruction format, minimal complexity
5. **Extensibility**: Easy to add new operations and optimizations

---

## Architecture Integration

### File Structure
```
src/ir/                     # New intermediate representation module
├── tac_types.h            # TAC instruction types and structures
├── tac_builder.h/.c       # TAC generation from AST
├── tac_optimizer.h/.c     # TAC optimization passes
├── tac_printer.h/.c       # TAC pretty printing and debugging
└── tac_store.h/.c         # File-backed TAC instruction storage
```

### Integration with Existing Systems
```c
// Build pipeline integration
AST → TAC Generation → TAC Optimization → Code Generation

// Storage system integration
┌─────────────┬──────────────┬─────────────┬─────────────┐
│  sstore     │   tstore     │   astore    │   tacstore  │
│ (strings)   │  (tokens)    │  (AST)      │  (TAC IR)   │
└─────────────┴──────────────┴─────────────┴─────────────┘
```

---

## Instruction Format

### Core TAC Instruction Structure
```c
/**
 * @brief Three-address code instruction
 * Size: 16 bytes (optimized for memory efficiency)
 */
typedef struct TACInstruction {
    TACOpcode opcode;        // 2 bytes - operation type
    TACFlags flags;          // 2 bytes - optimization flags
    
    TACOperand result;       // 4 bytes - destination operand
    TACOperand operand1;     // 4 bytes - first source operand  
    TACOperand operand2;     // 4 bytes - second source operand
} TACInstruction;
```

### Operand Representation
```c
/**
 * @brief TAC operand - flexible addressing
 * Size: 4 bytes
 */
typedef struct TACOperand {
    TACOperandType type;     // 1 byte - operand type
    union {
        struct {
            uint16_t id;     // Variable/temporary ID
            uint8_t scope;   // Scope level (for locals)
        } variable;
        
        struct {
            int32_t value;   // Immediate integer value
        } immediate;
        
        struct {
            uint16_t offset; // Jump target offset
            uint8_t padding;
        } label;
        
        struct {
            uint16_t func_id; // Function identifier
            uint8_t padding;
        } function;
        
        uint32_t raw;        // Raw 32-bit access
    } data;
} TACOperand;
```

### Operand Types
```c
typedef enum TACOperandType {
    TAC_OP_NONE = 0,         // No operand
    TAC_OP_TEMP,             // Temporary variable (t1, t2, ...)
    TAC_OP_VAR,              // Named variable
    TAC_OP_IMMEDIATE,        // Constant value
    TAC_OP_LABEL,            // Jump label
    TAC_OP_FUNCTION,         // Function reference
    TAC_OP_GLOBAL,           // Global variable
    TAC_OP_PARAM,            // Function parameter
    TAC_OP_RETURN_VAL        // Function return value
} TACOperandType;
```

---

## Instruction Set

### Arithmetic Operations
```c
typedef enum TACOpcode {
    // Binary arithmetic (result = op1 OP op2)
    TAC_ADD = 0x10,          // result = operand1 + operand2
    TAC_SUB,                 // result = operand1 - operand2  
    TAC_MUL,                 // result = operand1 * operand2
    TAC_DIV,                 // result = operand1 / operand2
    TAC_MOD,                 // result = operand1 % operand2
    
    // Unary arithmetic (result = OP op1)
    TAC_NEG,                 // result = -operand1
    TAC_NOT,                 // result = !operand1
    TAC_BITWISE_NOT,         // result = ~operand1
    
    // Bitwise operations
    TAC_AND,                 // result = operand1 & operand2
    TAC_OR,                  // result = operand1 | operand2
    TAC_XOR,                 // result = operand1 ^ operand2
    TAC_SHL,                 // result = operand1 << operand2
    TAC_SHR,                 // result = operand1 >> operand2
```

### Comparison and Logical Operations
```c
    // Comparison operations (result = op1 REL op2)
    TAC_EQ = 0x20,           // result = operand1 == operand2
    TAC_NE,                  // result = operand1 != operand2
    TAC_LT,                  // result = operand1 < operand2
    TAC_LE,                  // result = operand1 <= operand2
    TAC_GT,                  // result = operand1 > operand2
    TAC_GE,                  // result = operand1 >= operand2
    
    // Logical operations
    TAC_LOGICAL_AND,         // result = operand1 && operand2
    TAC_LOGICAL_OR,          // result = operand1 || operand2
```

### Memory and Assignment Operations
```c
    // Assignment operations
    TAC_ASSIGN = 0x30,       // result = operand1
    TAC_LOAD,                // result = *operand1 (indirect load)
    TAC_STORE,               // *result = operand1 (indirect store)
    TAC_ADDR,                // result = &operand1 (address of)
    
    // Array and struct operations  
    TAC_INDEX,               // result = operand1[operand2]
    TAC_MEMBER,              // result = operand1.field
    TAC_MEMBER_PTR,          // result = operand1->field
```

### Control Flow Operations
```c
    // Control flow
    TAC_LABEL = 0x40,        // Label definition (no operands)
    TAC_GOTO,                // goto operand1 (unconditional jump)
    TAC_IF_FALSE,            // if (!operand1) goto operand2
    TAC_IF_TRUE,             // if (operand1) goto operand2
    
    // Function operations
    TAC_CALL = 0x50,         // result = call operand1(params...)
    TAC_PARAM,               // Push parameter operand1
    TAC_RETURN,              // return operand1
    TAC_RETURN_VOID,         // return (no value)
    
    // Special operations
    TAC_NOP = 0x00,          // No operation
    TAC_CAST = 0x60,         // result = (type)operand1
    TAC_SIZEOF,              // result = sizeof(operand1)
    TAC_PHI                  // SSA phi function: result = φ(op1, op2)
} TACOpcode;
```

---

## Data Structures

### TAC Instruction Storage
```c
/**
 * @brief TAC instruction store - file-backed like other stores
 */
typedef struct TACStore {
    FILE* fp_tac;            // TAC instruction file
    TACIdx_t current_idx;    // Current instruction index
    TACIdx_t max_instructions; // Maximum instructions
    char filename[256];      // TAC file name
} TACStore;

typedef uint16_t TACIdx_t;   // TAC instruction index type
```

### Basic Block Representation
```c
/**
 * @brief Basic block structure for optimization
 */
typedef struct TACBasicBlock {
    TACIdx_t start_idx;      // First instruction index
    TACIdx_t end_idx;        // Last instruction index
    uint16_t id;             // Basic block ID
    uint16_t predecessor_count; // Number of predecessors
    uint16_t successor_count;   // Number of successors
    TACFlags flags;          // Block optimization flags
} TACBasicBlock;

/**
 * @brief Control flow graph
 */
typedef struct TACCFG {
    TACBasicBlock* blocks;   // Array of basic blocks
    uint16_t block_count;    // Number of blocks
    uint16_t* edges;         // Edge relationships
    uint16_t edge_count;     // Number of edges
} TACCFG;
```

### Function Context
```c
/**
 * @brief TAC function context
 */
typedef struct TACFunction {
    SymTabIdx_t symbol_idx;  // Function symbol
    TACIdx_t start_idx;      // First instruction
    TACIdx_t end_idx;        // Last instruction
    uint16_t temp_count;     // Number of temporaries used
    uint16_t param_count;    // Number of parameters
    uint16_t local_count;    // Number of local variables
    TypeIdx_t return_type;   // Return type
} TACFunction;
```

---

## Memory Management

### Storage Integration
```c
// TAC store API (similar to astore/sstore/tstore)
int tacstore_init(const char* filename);
void tacstore_close(void);
TACIdx_t tacstore_add(const TACInstruction* instr);
TACInstruction tacstore_get(TACIdx_t idx);
TACIdx_t tacstore_getidx(void);
```

### Memory Layout
```
TAC File Layout:
┌─────────────────┬─────────────────┬─────────────────┐
│ Header (64B)    │ Instructions    │ Basic Blocks    │
│ - Version       │ - 16B each      │ - Variable size │
│ - Instruction # │ - Sequential    │ - For CFG       │
│ - Function #    │                 │                 │
└─────────────────┴─────────────────┴─────────────────┘
```

### Temporary Variable Management
```c
/**
 * @brief Temporary variable allocator
 */
typedef struct TACTempManager {
    uint16_t next_temp;      // Next temporary ID
    uint16_t max_temp;       // Maximum concurrent temporaries
    uint8_t* temp_types;     // Type of each temporary
    TACFlags* temp_flags;    // Flags for each temporary
} TACTempManager;

// Temporary allocation API
uint16_t tac_alloc_temp(TACTempManager* mgr, TypeIdx_t type);
void tac_free_temp(TACTempManager* mgr, uint16_t temp_id);
```

---

## Code Generation

### AST to TAC Translation
```c
/**
 * @brief TAC builder context for AST translation
 */
typedef struct TACBuilder {
    TACStore* store;         // TAC instruction store
    TACTempManager* temp_mgr; // Temporary management
    uint16_t label_counter;  // Label generation counter
    ASTBuilder* ast_builder; // AST builder reference
    ErrorHandler* error_handler; // Error reporting
} TACBuilder;

// Core translation functions
TACOperand tac_build_from_ast(TACBuilder* builder, ASTNodeIdx_t node);
TACIdx_t tac_emit_instruction(TACBuilder* builder, TACOpcode op, 
                             TACOperand result, TACOperand op1, TACOperand op2);
uint16_t tac_new_label(TACBuilder* builder);
uint16_t tac_new_temp(TACBuilder* builder, TypeIdx_t type);
```

### Expression Translation
```c
/**
 * @brief Translate binary expression to TAC
 * Example: a + b * c becomes:
 *   t1 = b * c
 *   t2 = a + t1
 */
TACOperand translate_binary_expr(TACBuilder* builder, ASTNodeIdx_t node) {
    HBNode* hb_node = HBGet(node, HBMODE_AST);
    if (!hb_node) return TAC_OPERAND_NONE;
    
    // Translate operands
    TACOperand left = tac_build_from_ast(builder, hb_node->ast.binary.left);
    TACOperand right = tac_build_from_ast(builder, hb_node->ast.binary.right);
    
    // Get operator from token
    Token_t token = tstore_get(hb_node->ast.token_idx);
    TACOpcode opcode = token_to_tac_opcode(token.id);
    
    // Generate result temporary
    TACOperand result = tac_make_temp(builder, hb_node->ast.type_idx);
    
    // Emit instruction
    tac_emit_instruction(builder, opcode, result, left, right);
    
    return result;
}
```

### Control Flow Translation
```c
/**
 * @brief Translate if statement to TAC
 * Example: if (condition) then_stmt else else_stmt
 * Becomes:
 *   t1 = condition
 *   if_false t1 goto L_else
 *   [then_stmt code]
 *   goto L_end
 * L_else:
 *   [else_stmt code]  
 * L_end:
 */
void translate_if_stmt(TACBuilder* builder, ASTNodeIdx_t node) {
    HBNode* hb_node = HBGet(node, HBMODE_AST);
    
    // Generate labels
    uint16_t else_label = tac_new_label(builder);
    uint16_t end_label = tac_new_label(builder);
    
    // Translate condition
    TACOperand cond = tac_build_from_ast(builder, 
                                        hb_node->ast.conditional.condition);
    
    // Conditional jump to else
    tac_emit_instruction(builder, TAC_IF_FALSE, TAC_OPERAND_NONE, 
                        cond, tac_make_label(else_label));
    
    // Then block
    tac_build_from_ast(builder, hb_node->ast.conditional.then_stmt);
    
    // Jump to end (skip else)
    if (hb_node->ast.conditional.else_stmt != 0) {
        tac_emit_instruction(builder, TAC_GOTO, TAC_OPERAND_NONE,
                            tac_make_label(end_label), TAC_OPERAND_NONE);
        
        // Else label and block
        tac_emit_label(builder, else_label);
        tac_build_from_ast(builder, hb_node->ast.conditional.else_stmt);
    }
    
    // End label
    tac_emit_label(builder, end_label);
}
```

---

## Optimization Framework

### Optimization Flags
```c
typedef enum TACFlags {
    TAC_FLAG_NONE         = 0x0000,
    TAC_FLAG_DEAD_CODE    = 0x0001,  // Dead code elimination candidate
    TAC_FLAG_CONST_FOLD   = 0x0002,  // Constant folding applied
    TAC_FLAG_CSE          = 0x0004,  // Common subexpression elimination
    TAC_FLAG_COPY_PROP    = 0x0008,  // Copy propagation applied
    TAC_FLAG_MODIFIED     = 0x0010,  // Instruction modified
    TAC_FLAG_LIVE         = 0x0020,  // Live variable
    TAC_FLAG_OPTIMIZED    = 0x8000   // Optimization complete
} TACFlags;
```

### Optimization Passes
```c
/**
 * @brief Optimization pass function type
 */
typedef int (*TACOptimizeFunc)(TACCFG* cfg, TACFunction* func);

/**
 * @brief Basic optimization passes
 */
// Constant folding: replace operations on constants with results
int tac_optimize_constant_folding(TACCFG* cfg, TACFunction* func);

// Dead code elimination: remove unused instructions
int tac_optimize_dead_code_elimination(TACCFG* cfg, TACFunction* func);

// Copy propagation: replace copies with original values
int tac_optimize_copy_propagation(TACCFG* cfg, TACFunction* func);

// Common subexpression elimination
int tac_optimize_cse(TACCFG* cfg, TACFunction* func);

/**
 * @brief Optimization pipeline
 */
typedef struct TACOptimizer {
    TACOptimizeFunc* passes; // Array of optimization passes
    int pass_count;          // Number of passes
    int max_iterations;      // Maximum optimization iterations
} TACOptimizer;
```

### Data Flow Analysis
```c
/**
 * @brief Live variable analysis
 */
typedef struct TACLiveness {
    uint32_t* live_in;       // Live variables at block entry
    uint32_t* live_out;      // Live variables at block exit
    uint32_t* def;           // Variables defined in block
    uint32_t* use;           // Variables used in block
} TACLiveness;

// Data flow analysis functions
void tac_compute_liveness(TACCFG* cfg, TACLiveness* liveness);
void tac_compute_dominators(TACCFG* cfg, uint16_t* dominators);
void tac_build_def_use_chains(TACCFG* cfg, TACFunction* func);
```

---

## Implementation Plan

### Phase 1: Core Infrastructure (Week 1-2)
1. **TAC Data Structures**
   - Implement `TACInstruction`, `TACOperand` types
   - Create `tacstore.h/.c` file-backed storage
   - Add TAC types to `tac_types.h`

2. **Basic TAC Builder**
   - Implement `TACBuilder` context
   - Create instruction emission functions
   - Add temporary and label management

### Phase 2: AST Translation (Week 3-4)
1. **Expression Translation**
   - Binary and unary operators
   - Literals and identifiers
   - Function calls and array access

2. **Statement Translation**
   - Assignment statements
   - Control flow (if, while, for)
   - Function definitions

3. **Integration Testing**
   - Add TAC generation to existing compiler pipeline
   - Test with existing AST test cases

### Phase 3: Optimization Framework (Week 5-6)
1. **Basic Block Construction**
   - Build control flow graph from TAC
   - Implement basic block identification

2. **Simple Optimizations**
   - Constant folding
   - Dead code elimination
   - Copy propagation

### Phase 4: Advanced Features (Week 7-8)
1. **Data Flow Analysis**
   - Live variable analysis
   - Reaching definitions

2. **Advanced Optimizations**
   - Common subexpression elimination
   - Loop optimizations

3. **Code Generation Interface**
   - TAC to assembly translation
   - Register allocation support

---

## Example Translations

### Simple Arithmetic
```c
// C code
int x = a + b * c;

// AST nodes (simplified)
AST_VAR_DECL
├── AST_EXPR_ASSIGN
    ├── AST_EXPR_IDENTIFIER (x)
    └── AST_EXPR_BINARY_OP (+)
        ├── AST_EXPR_IDENTIFIER (a)
        └── AST_EXPR_BINARY_OP (*)
            ├── AST_EXPR_IDENTIFIER (b)
            └── AST_EXPR_IDENTIFIER (c)

// Generated TAC
t1 = b * c          // TAC_MUL t1, b, c
t2 = a + t1         // TAC_ADD t2, a, t1  
x = t2              // TAC_ASSIGN x, t2, _
```

### Control Flow
```c
// C code
if (x > 0) {
    y = x + 1;
} else {
    y = x - 1;
}

// Generated TAC
t1 = x > 0          // TAC_GT t1, x, 0
if_false t1 goto L1 // TAC_IF_FALSE _, t1, L1
t2 = x + 1          // TAC_ADD t2, x, 1
y = t2              // TAC_ASSIGN y, t2, _
goto L2             // TAC_GOTO _, L2, _
L1:                 // TAC_LABEL L1, _, _
t3 = x - 1          // TAC_SUB t3, x, 1
y = t3              // TAC_ASSIGN y, t3, _
L2:                 // TAC_LABEL L2, _, _
```

### Function Call
```c
// C code
int result = add(x, y + 1);

// Generated TAC
t1 = y + 1          // TAC_ADD t1, y, 1
param t1            // TAC_PARAM _, t1, _
param x             // TAC_PARAM _, x, _
t2 = call add       // TAC_CALL t2, add, _
result = t2         // TAC_ASSIGN result, t2, _
```

### With Optimization
```c
// Before optimization
t1 = 5 + 3          // TAC_ADD t1, 5, 3
t2 = t1 * 2         // TAC_MUL t2, t1, 2  
x = t2              // TAC_ASSIGN x, t2, _

// After constant folding
t2 = 8 * 2          // TAC_MUL t2, 8, 2 (5+3 folded)
x = t2              // TAC_ASSIGN x, t2, _

// After further folding
x = 16              // TAC_ASSIGN x, 16, _ (8*2 folded)
```

---

## Integration Points

### Makefile Integration
```makefile
# Add TAC module to build
TAC_SRC = $(SRCDIR)/ir
TAC_OBJS = $(OBJDIR)/tac_builder.o $(OBJDIR)/tac_store.o \
           $(OBJDIR)/tac_optimizer.o $(OBJDIR)/tac_printer.o

# Update compiler pipeline
$(BINDIR)/cc1: $(PARSER_OBJS) $(AST_OBJS) $(TAC_OBJS) $(STORAGE_OBJS)
```

### Error Handling Integration
```c
// TAC-specific error codes
typedef enum TACErrorCode {
    TAC_ERROR_NONE = 0,
    TAC_ERROR_INVALID_OPERAND,
    TAC_ERROR_TEMP_OVERFLOW,
    TAC_ERROR_INVALID_LABEL,
    TAC_ERROR_CFG_CONSTRUCTION,
    TAC_ERROR_OPTIMIZATION_FAILED
} TACErrorCode;
```

### Testing Integration
```c
// TAC-specific test cases
void test_tac_arithmetic_translation(void);
void test_tac_control_flow_translation(void);
void test_tac_function_translation(void);
void test_tac_optimization_passes(void);
```

---

## Benefits of This Design

1. **Memory Efficient**: 16-byte instructions, file-backed storage
2. **AST Compatible**: Direct translation from existing AST structures  
3. **Optimization Ready**: CFG construction, data flow analysis support
4. **Target Independent**: Clean abstraction for multiple backends
5. **Incremental**: Can be added without disrupting existing code
6. **Debuggable**: Pretty-printing and visualization support
7. **Extensible**: Easy to add new operations and optimization passes

This TAC design provides a solid foundation for optimization and code generation while maintaining the memory efficiency and architectural principles of the STCC1 compiler.
