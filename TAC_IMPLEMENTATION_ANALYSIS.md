# STCC1 TAC (Three-Address Code) Implementation Analysis

**Author:** GitHub Copilot  
**Date:** August 3, 2025  
**Version:** 1.0  
**Status:** Investigation Complete - Solutions Proposed

---

## 📋 **Executive Summary**

This document provides a comprehensive analysis of the STCC1 compiler's Three-Address Code (TAC) implementation, including architecture, instruction set, storage integration, and current limitations. The TAC system successfully handles 97% of test cases, with remaining issues focused on function call semantics and parameter handling.

**Key Findings:**
- ✅ Complete TAC instruction set with 30+ opcodes
- ✅ Robust file-backed storage system
- ✅ Integration with compiler storage ecosystem
- ❌ Function parameter handling incomplete
- ❌ Function call generation missing

---

## 🏗️ **System Architecture**

### **TAC Pipeline Overview**

```
┌─────────────────────────────────────────────────────────────────┐
│                    STCC1 TAC SYSTEM ARCHITECTURE                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐          │
│  │   Source    │ →  │     AST     │ →  │     TAC     │          │
│  │   (.c)      │    │  (cc1.out)  │    │  (cc2.out)  │          │
│  └─────────────┘    └─────────────┘    └─────────────┘          │
│                                                                 │
│                     ┌─────────────┐                             │
│                     │ TAC Engine  │ ← Executes TAC Instructions │
│                     │ Validation  │                             │
│                     └─────────────┘                             │
└─────────────────────────────────────────────────────────────────┘
```

### **Storage System Integration**

```
┌──────────────────────────────────────────────────────────────────────┐
│                         STCC1 STORAGE ECOSYSTEM                     │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐   │
│  │   SSTORE    │  │   TSTORE    │  │   ASTORE    │  │  TACSTORE   │   │
│  │   Strings   │  │   Tokens    │  │   AST       │  │     TAC     │   │
│  │             │  │             │  │   Nodes     │  │ Instructions│   │
│  │ .sstore.out │  │ .tstore.out │  │ .ast.out    │  │ .tac.out    │   │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘   │
│         │                 │               │               │           │
│         └─────────────────┼───────────────┼───────────────┼────────┐  │
│                           │               │               │        │  │
│  ┌─────────────┐          │               │               │        │  │
│  │   SYMTAB    │ ←────────┴───────────────┴───────────────┴────────┘  │
│  │Symbol Table │  References symbols across all stores               │
│  │ .sym.out    │                                                     │
│  └─────────────┘                                                     │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 🔧 **TAC Instruction Set**

### **Data Types & Operands**

```c
typedef enum TACOperandType {
    TAC_OP_NONE = 0,         // No operand
    TAC_OP_TEMP,             // Temporary variable (t1, t2, ...) 
    TAC_OP_VAR,              // Named variable from symbol table
    TAC_OP_IMMEDIATE,        // Constant value (integers)
    TAC_OP_LABEL,            // Jump labels (L1, L2, ...)
    TAC_OP_FUNCTION,         // Function references
    TAC_OP_GLOBAL,           // Global variables
    TAC_OP_PARAM,            // Function parameters
    TAC_OP_RETURN_VAL        // Function return values
} TACOperandType;
```

### **Complete Instruction Reference**

#### **🧮 Arithmetic Operations**
| Opcode | Format | Description |
|--------|--------|-------------|
| `TAC_ADD` | `result = operand1 + operand2` | Addition |
| `TAC_SUB` | `result = operand1 - operand2` | Subtraction |
| `TAC_MUL` | `result = operand1 * operand2` | Multiplication |
| `TAC_DIV` | `result = operand1 / operand2` | Division |
| `TAC_MOD` | `result = operand1 % operand2` | Modulo |
| `TAC_NEG` | `result = -operand1` | Unary negation |

#### **🔢 Bitwise Operations**
| Opcode | Format | Description |
|--------|--------|-------------|
| `TAC_AND` | `result = operand1 & operand2` | Bitwise AND |
| `TAC_OR` | `result = operand1 \| operand2` | Bitwise OR |
| `TAC_XOR` | `result = operand1 ^ operand2` | Bitwise XOR |
| `TAC_SHL` | `result = operand1 << operand2` | Left shift |
| `TAC_SHR` | `result = operand1 >> operand2` | Right shift |
| `TAC_NOT` | `result = !operand1` | Logical NOT |
| `TAC_BITWISE_NOT` | `result = ~operand1` | Bitwise NOT |

#### **🔍 Comparison Operations**
| Opcode | Format | Description |
|--------|--------|-------------|
| `TAC_EQ` | `result = operand1 == operand2` | Equal |
| `TAC_NE` | `result = operand1 != operand2` | Not equal |
| `TAC_LT` | `result = operand1 < operand2` | Less than |
| `TAC_LE` | `result = operand1 <= operand2` | Less or equal |
| `TAC_GT` | `result = operand1 > operand2` | Greater than |
| `TAC_GE` | `result = operand1 >= operand2` | Greater or equal |

#### **🧠 Logical Operations**
| Opcode | Format | Description |
|--------|--------|-------------|
| `TAC_LOGICAL_AND` | `result = operand1 && operand2` | Logical AND |
| `TAC_LOGICAL_OR` | `result = operand1 \|\| operand2` | Logical OR |

#### **📦 Assignment & Memory Operations**
| Opcode | Format | Description |
|--------|--------|-------------|
| `TAC_ASSIGN` | `result = operand1` | Assignment |
| `TAC_LOAD` | `result = *operand1` | Indirect load |
| `TAC_STORE` | `*result = operand1` | Indirect store |
| `TAC_ADDR` | `result = &operand1` | Address of |
| `TAC_INDEX` | `result = operand1[operand2]` | Array access |
| `TAC_MEMBER` | `result = operand1.field` | Struct member |
| `TAC_MEMBER_PTR` | `result = operand1->field` | Pointer member |

#### **🎯 Control Flow Operations**
| Opcode | Format | Description |
|--------|--------|-------------|
| `TAC_LABEL` | `Label:` | Label definition |
| `TAC_GOTO` | `goto operand1` | Unconditional jump |
| `TAC_IF_FALSE` | `if (!operand1) goto operand2` | Conditional jump |
| `TAC_IF_TRUE` | `if (operand1) goto operand2` | Conditional jump |

#### **📞 Function Operations**
| Opcode | Format | Description |
|--------|--------|-------------|
| `TAC_CALL` | `result = call operand1` | Function call |
| `TAC_PARAM` | `param operand1` | Push parameter |
| `TAC_RETURN` | `return operand1` | Return with value |
| `TAC_RETURN_VOID` | `return` | Return void |

#### **⚙️ Special Operations**
| Opcode | Format | Description |
|--------|--------|-------------|
| `TAC_CAST` | `result = (type)operand1` | Type conversion |
| `TAC_SIZEOF` | `result = sizeof(operand1)` | Size of type |
| `TAC_PHI` | `result = φ(op1, op2)` | SSA phi function |
| `TAC_NOP` | `nop` | No operation |

---

## 🏷️ **Label System**

### **Label Management**

```
┌─────────────────────────────────────────────────────────────────┐
│                      TAC LABEL SYSTEM                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Label Creation:                                                │
│  ┌─────────────────┐                                            │
│  │ label_counter++ │ → L1, L2, L3, L4, ...                      │
│  └─────────────────┘                                            │
│                                                                 │
│  Label Storage:                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ struct { uint16_t offset; uint8_t padding; } label;     │   │
│  │ - offset: contains the label ID (1, 2, 3, 4...)         │   │
│  │ - Used in: jumps, function entries, loop targets        │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
│  Function Table Mapping:                                        │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ function_names[i] → "main", "factorial", ...            │   │
│  │ label_ids[i]      → 4, 1, ...                           │   │
│  │ instruction_addresses[i] → 14, 1, ...                   │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### **Label Usage Examples**

```c
// Function labels
L1:                    // Function entry point
    result = 1
    // ... function body

// Loop labels  
L2:                    // Loop start
    t1 = i le n
    if_false t1 goto L3
    // ... loop body
    goto L2

L3:                    // Loop exit
    return result
```

---

## 💾 **File Format Specification**

### **Binary Format (.tac.out)**

```
┌─────────────────────────────────────────────────────────────────┐
│                    TAC BINARY FILE FORMAT                      │
│                         (.tac.out)                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  File Structure: Array of TACInstruction structs               │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ TACInstruction[0] │ 16 bytes │ Instruction 1           │    │
│  ├─────────────────────────────────────────────────────────┤    │
│  │ TACInstruction[1] │ 16 bytes │ Instruction 2           │    │
│  ├─────────────────────────────────────────────────────────┤    │
│  │ TACInstruction[2] │ 16 bytes │ Instruction 3           │    │
│  ├─────────────────────────────────────────────────────────┤    │
│  │      ...          │   ...    │    ...                  │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                 │
│  Each TACInstruction (16 bytes):                                │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ TACOpcode opcode;     // 2 bytes - operation type       │   │
│  │ TACFlags flags;       // 2 bytes - optimization flags   │   │
│  │ TACOperand result;    // 4 bytes - destination          │   │
│  │ TACOperand operand1;  // 4 bytes - first source         │   │
│  │ TACOperand operand2;  // 4 bytes - second source        │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
│  Each TACOperand (4 bytes):                                     │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ TACOperandType type;  // 1 byte - operand type          │   │
│  │ union data {          // 3 bytes - operand data         │   │
│  │   variable: {id, scope}     // For vars/temps           │   │
│  │   immediate: {value}        // For constants            │   │
│  │   label: {offset}           // For jump targets         │   │
│  │   function: {func_id}       // For function calls       │   │
│  │ }                                                        │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### **Text Format (Human-Readable)**

```
; TAC Instructions - Generated by STCC1
; Total instructions: 18
; MAIN_LABEL: 4

[   1] L1:
[   2] result = 1
[   3] i = 1
[   4] L2:
[   5] t1 = i le n
[   6] if_false t1 goto L3
[   7] t2 = result mul i
[   8] result = t2
[   9] t3 = i add 1
[  10] i = t3
[  11] goto L2
[  12] L3:
[  13] return result
[  14] L4:
[  15] param 5
[  16] t4 = call L1
[  17] f5 = t4
[  18] return f5
```

---

## 🔗 **Store Integration**

### **Cross-Store Relationships**

```
┌─────────────────────────────────────────────────────────────────┐
│                   TAC STORE RELATIONSHIPS                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  TAC ←→ SYMTAB: Variable name resolution                        │
│  ├─ TAC operands reference SymIdx_t                             │
│  ├─ Function names stored in symbol table                       │
│  └─ Variable scoping and type information                       │
│                                                                 │
│  TAC ←→ SSTORE: String literals and identifiers                 │
│  ├─ Function names stored as strings                            │
│  ├─ String constants in expressions                             │
│  └─ Debug information and error messages                        │
│                                                                 │
│  TAC ←→ ASTORE: Source AST for TAC generation                   │
│  ├─ AST nodes traversed to generate TAC                         │
│  ├─ Type information from AST nodes                             │
│  └─ Source location mapping                                     │
│                                                                 │
│  TAC ←→ TSTORE: Original tokens for debugging                   │
│  ├─ Token source positions                                      │
│  ├─ Original operator representations                           │
│  └─ Lexical analysis information                                │
└─────────────────────────────────────────────────────────────────┘
```

### **API Integration Points**

| Store | TAC Usage | Data Flow |
|-------|-----------|-----------|
| **SYMTAB** | Variable resolution | `SymIdx_t` → Variable names |
| **SSTORE** | String storage | Function names, literals |
| **ASTORE** | AST traversal | AST nodes → TAC instructions |
| **TSTORE** | Debug info | Source positions, operators |

---

## 🚀 **Generation Pipeline**

### **TAC Build Process**

```
┌─────────────────────────────────────────────────────────────────┐
│                    TAC GENERATION PIPELINE                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. AST Traversal                                               │
│     ├─ tac_build_from_ast(builder, node)                       │
│     ├─ Recursive descent through AST                           │
│     └─ Pattern matching on node types                          │
│                                                                 │
│  2. Instruction Emission                                        │
│     ├─ tac_emit_instruction(builder, op, result, op1, op2)     │
│     ├─ Temporary variable allocation                            │
│     ├─ Label generation and management                          │
│     └─ Function table construction                              │
│                                                                 │
│  3. Binary Output                                               │
│     ├─ tacstore_add() writes to binary file                    │
│     ├─ 16-byte aligned TACInstruction structs                  │
│     └─ Sequential instruction storage                           │
│                                                                 │
│  4. Text Output                                                 │
│     ├─ tac_write_to_file() generates human-readable            │
│     ├─ Function table metadata (MAIN_LABEL)                    │
│     └─ Pretty-printed instruction format                       │
└─────────────────────────────────────────────────────────────────┘
```

### **Key Generation Functions**

```c
// Core TAC generation entry point
TACOperand tac_build_from_ast(TACBuilder* builder, ASTNodeIdx_t node);

// Instruction emission
TACIdx_t tac_emit_instruction(TACBuilder* builder, TACOpcode op,
                             TACOperand result, TACOperand op1, TACOperand op2);

// Resource management
TACOperand tac_new_temp(TACBuilder* builder, TypeIdx_t type);
TACOperand tac_new_label(TACBuilder* builder);

// Storage operations
TACIdx_t tacstore_add(const TACInstruction* instr);
TACInstruction tacstore_get(TACIdx_t idx);
```

---

## ⚠️ **Current Limitations**

### **1. Function Parameter Handling**

**Problem:** Function parameters are not properly initialized at function entry.

**Current Behavior:**
```c
// C source
int factorial(int n) {
    int result = 1;
    // ... n is never initialized from parameter
}
```

**Generated TAC:**
```
L1:                    // factorial function entry
result = 1             // ✅ Local variable initialization
i = 1                  // ✅ Local variable initialization
t1 = i le n           // ❌ n is uninitialized (defaults to 0)
```

**Impact:** Functions with parameters return incorrect results because parameters default to 0.

### **2. Function Call Generation**

**Problem:** Function calls are not generating proper `param`/`call` instruction sequences.

**Current Behavior:**
```c
// C source
int main() {
    int result = factorial(5);
    return result;
}
```

**Expected TAC:**
```
main:
param 5                // ✅ Push parameter
result = call factorial  // ✅ Function call
return result          // ✅ Return result
```

**Generated TAC:**
```
main:
x1 = 0                 // ❌ Wrong variable assignments
// ... missing call sequence
factorial:             // ❌ Direct flow instead of call
```

**Impact:** Multi-function programs execute as single code blocks without proper call semantics.

### **3. Return Value Propagation**

**Problem:** Function return values are not properly propagated to calling context.

**Impact:** Calling functions may not receive correct return values, affecting program correctness.

---

## 💡 **Proposed Solutions**

### **Solution 1: Function Parameter Initialization**

#### **Implementation Plan**

1. **Enhance Function Definition Processing**
   ```c
   case AST_FUNCTION_DEF:
       // Emit function label
       emit_function_label(builder, func_name);
       
       // NEW: Emit parameter initialization
       emit_parameter_prologue(builder, func_node);
       
       // Process function body
       process_function_body(builder, func_node);
   ```

2. **Add Parameter Prologue Generation**
   ```c
   void emit_parameter_prologue(TACBuilder* builder, ASTNode* func_node) {
       // Extract parameter list from function declaration
       ParameterList* params = extract_parameters(func_node);
       
       for (int i = 0; i < params->count; i++) {
           SymIdx_t param_symbol = params->symbols[i];
           TACOperand param_var = create_variable_operand(param_symbol);
           TACOperand param_ref = TAC_MAKE_PARAM(i);
           
           // Generate: param_var = parameter[i]
           tac_emit_instruction(builder, TAC_ASSIGN, param_var, param_ref, TAC_OPERAND_NONE);
       }
   }
   ```

3. **Extend Parameter Operand Type**
   ```c
   // Add parameter access operand
   typedef struct {
       uint16_t param_index;  // Parameter index (0, 1, 2...)
       uint8_t padding;
   } parameter;
   
   // Update operand union
   union {
       // ... existing operands
       parameter param;  // For accessing function parameters
   } data;
   ```

#### **Expected Output**
```
L1:                    // factorial function entry
n = param[0]           // ✅ Initialize n from first parameter
result = 1             // ✅ Local variable initialization
i = 1                  // ✅ Local variable initialization
// ... rest of function with n properly initialized
```

### **Solution 2: Function Call Sequence Generation**

#### **Implementation Plan**

1. **Enhance Function Call Processing**
   ```c
   case AST_FUNCTION_CALL:
       // Extract function name and arguments
       char* func_name = extract_function_name(call_node);
       ArgumentList* args = extract_arguments(call_node);
       
       // Generate parameter pushing sequence
       for (int i = 0; i < args->count; i++) {
           TACOperand arg = tac_build_from_ast(builder, args->expressions[i]);
           tac_emit_instruction(builder, TAC_PARAM, TAC_OPERAND_NONE, arg, TAC_OPERAND_NONE);
       }
       
       // Generate function call
       TACOperand func_label = lookup_function_label(builder, func_name);
       TACOperand result_temp = tac_new_temp(builder, call_node->type_idx);
       tac_emit_instruction(builder, TAC_CALL, result_temp, func_label, TAC_OPERAND_NONE);
       
       return result_temp;
   ```

2. **Implement Function Label Lookup**
   ```c
   TACOperand lookup_function_label(TACBuilder* builder, const char* func_name) {
       for (uint32_t i = 0; i < builder->function_table.count; i++) {
           if (strcmp(builder->function_table.function_names[i], func_name) == 0) {
               uint32_t label_id = builder->function_table.label_ids[i];
               return TAC_MAKE_LABEL(label_id);
           }
       }
       // Handle error: function not found
       builder->error_count++;
       return TAC_OPERAND_NONE;
   }
   ```

3. **Add Function Call Validation**
   ```c
   void validate_function_call(TACBuilder* builder, const char* func_name, int arg_count) {
       // Verify function exists in symbol table
       // Verify argument count matches parameter count
       // Generate appropriate error messages
   }
   ```

#### **Expected Output**
```
main:
x1 = 0
y1 = 0
x2 = 3
y2 = 4
param x1               // ✅ Push parameters in order
param y1
param x2
param y2
t1 = call L1          // ✅ Proper function call
dist_sq = t1          // ✅ Capture return value
return dist_sq        // ✅ Return result

L1:                   // distance_squared function
x1 = param[0]         // ✅ Initialize parameters
y1 = param[1]
x2 = param[2]
y2 = param[3]
// ... function body
```

### **Solution 3: Return Value Enhancement**

#### **Implementation Plan**

1. **Standardize Return Value Location**
   ```c
   // Always use temp[0] for return values
   #define TAC_RETURN_TEMP 0
   
   case AST_RETURN:
       if (return_node->expression != 0) {
           TACOperand return_value = tac_build_from_ast(builder, return_node->expression);
           TACOperand return_temp = TAC_MAKE_TEMP(TAC_RETURN_TEMP);
           
           // Assign to standard return location
           tac_emit_instruction(builder, TAC_ASSIGN, return_temp, return_value, TAC_OPERAND_NONE);
       }
       tac_emit_instruction(builder, TAC_RETURN, TAC_OPERAND_NONE, TAC_OPERAND_NONE, TAC_OPERAND_NONE);
   ```

2. **Update Function Call Return Handling**
   ```c
   // After TAC_CALL instruction
   TACOperand return_temp = TAC_MAKE_TEMP(TAC_RETURN_TEMP);
   TACOperand result_var = tac_new_temp(builder, call_node->type_idx);
   
   // Copy return value to result variable
   tac_emit_instruction(builder, TAC_ASSIGN, result_var, return_temp, TAC_OPERAND_NONE);
   ```

### **Solution 4: Implementation Roadmap**

#### **Phase 1: Foundation (Week 1)**
- [ ] Extend operand types for parameter access
- [ ] Implement parameter prologue generation
- [ ] Add function parameter extraction from AST
- [ ] Update TAC instruction emission for parameters

#### **Phase 2: Function Calls (Week 2)**
- [ ] Implement function call sequence generation
- [ ] Add function label lookup and validation
- [ ] Update function call AST processing
- [ ] Test simple function call scenarios

#### **Phase 3: Integration (Week 3)**
- [ ] Standardize return value handling
- [ ] Integrate parameter and call systems
- [ ] Update TAC engine for new instruction patterns
- [ ] Comprehensive testing of multi-function programs

#### **Phase 4: Validation (Week 4)**
- [ ] Fix failing test cases (factorial, distance_squared)
- [ ] Add regression tests for function calls
- [ ] Performance testing and optimization
- [ ] Documentation updates

#### **Success Metrics**
- [ ] `test_integration_iterative_algorithm` passes (factorial(5) = 120)
- [ ] `test_integration_mixed_declarations_and_scoping` passes (distance = 25)
- [ ] All existing tests continue to pass (maintain 97%+ pass rate)
- [ ] TAC engine correctly executes multi-function programs

---

## 🔧 **Implementation Files to Modify**

### **Core TAC Generation**
- `src/ir/tac_builder.c` - Add parameter and call generation
- `src/ir/tac_builder.h` - Update function signatures
- `src/ir/tac_types.h` - Extend operand types

### **Storage Integration**
- `src/storage/symtab.h` - Parameter extraction utilities
- `src/ir/tac_store.c` - Potential instruction format updates

### **Testing Framework**
- `tests/test_common.c` - Update validation functions if needed
- `tests/integration/test_integration.c` - Monitor test results

### **Tools and Debugging**
- `src/parser/cc2t.c` - Update for new operand types
- `src/ir/tac_printer.c` - Pretty print parameter instructions

---

## 📈 **Expected Outcomes**

### **Test Results Improvement**
- **Current**: 76/78 tests passing (97.4%)
- **Target**: 78/78 tests passing (100%)
- **Fixed Tests**: 
  - `test_integration_iterative_algorithm` 
  - `test_integration_mixed_declarations_and_scoping`

### **TAC Quality Enhancement**
- **Complete function call semantics**
- **Proper parameter passing**
- **Correct return value propagation**
- **Multi-function program support**

### **System Robustness**
- **Better error handling for invalid function calls**
- **Parameter count validation**
- **Type checking for function arguments**
- **Comprehensive test coverage for function scenarios**

---

## 🎯 **Conclusion**

The STCC1 TAC implementation provides a solid foundation with comprehensive instruction set, robust storage integration, and effective debugging tools. The identified limitations are well-defined and addressable through systematic implementation of function call semantics and parameter handling.

The proposed solutions follow established compiler design patterns and integrate cleanly with the existing architecture. Implementation of these enhancements will complete the TAC system and achieve 100% test pass rate while maintaining system performance and reliability.

**Priority Focus:** Function parameter initialization and call sequence generation are the critical path items that will unlock the remaining test failures and complete the TAC implementation.

---

**Document Status:** ✅ Complete  
**Next Action:** Begin Phase 1 implementation of parameter handling enhancement  
**Review Date:** August 10, 2025
