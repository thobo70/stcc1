# STCC1 TAC Function Call Implementation Guide

**Author:** GitHub Copilot  
**Date:** August 3, 2025  
**Purpose:** Detailed implementation guide for TAC function call enhancements  
**Target:** STCC1 Compiler Development Team

---

## 🎯 **Implementation Overview**

This document provides step-by-step implementation instructions for fixing the two critical TAC limitations:

1. **Function Parameter Handling** - Initialize function parameters from call stack
2. **Function Call Generation** - Generate proper param/call instruction sequences

---

## 🔧 **Solution 1: Function Parameter Handling**

### **Problem Analysis**

**Current Issue:**
```c
// C Source
int factorial(int n) {
    int result = 1;
    while (i <= n) {  // n is never initialized!
        // ...
    }
}
```

**Current TAC:**
```
L1:                    // factorial function entry
result = 1             // ✅ Local variables initialized
i = 1                  // ✅ Local variables initialized  
t1 = i le n           // ❌ n defaults to 0 (uninitialized)
```

### **Implementation Steps**

#### **Step 1: Extend TACOperandType**

**File:** `src/ir/tac_types.h`

```c
typedef enum TACOperandType {
    TAC_OP_NONE = 0,
    TAC_OP_TEMP,
    TAC_OP_VAR,
    TAC_OP_IMMEDIATE,
    TAC_OP_LABEL,
    TAC_OP_FUNCTION,
    TAC_OP_GLOBAL,
    TAC_OP_PARAM,
    TAC_OP_RETURN_VAL,
    TAC_OP_PARAM_ACCESS   // NEW: Access function parameters
} TACOperandType;
```

#### **Step 2: Add Parameter Access Operand**

**File:** `src/ir/tac_types.h`

```c
typedef struct TACOperand {
    TACOperandType type;
    union {
        // ... existing operands ...
        
        struct {
            uint16_t param_index;  // Parameter index (0, 1, 2...)
            uint8_t padding;
        } param_access;            // NEW: For accessing call parameters
        
        uint32_t raw;
    } data;
} TACOperand;
```

#### **Step 3: Add Parameter Access Macro**

**File:** `src/ir/tac_types.h`

```c
// Add to convenience macros
#define TAC_MAKE_PARAM_ACCESS(idx) ((TACOperand){TAC_OP_PARAM_ACCESS, {.param_access = {idx, 0}}})
```

#### **Step 4: Implement Parameter Extraction**

**File:** `src/ir/tac_builder.c`

```c
/**
 * @brief Extract function parameters from AST function definition
 */
static ParameterInfo extract_function_parameters(ASTNode* func_node) {
    ParameterInfo params = {0};
    
    // Navigate AST to find parameter list
    // This depends on your AST structure - typically:
    // func_node->children.child2 contains parameter declarations
    
    ASTNodeIdx_t param_list_idx = func_node->children.child2;
    if (param_list_idx == 0) {
        return params; // No parameters
    }
    
    ASTNode param_list = astore_get(param_list_idx);
    
    // Extract each parameter symbol
    ASTNodeIdx_t current_param = param_list.children.child1;
    while (current_param != 0 && params.count < MAX_FUNCTION_PARAMS) {
        ASTNode param_node = astore_get(current_param);
        
        if (param_node.type == AST_DECLARATION) {
            SymIdx_t param_symbol = param_node.declaration.symbol_idx;
            if (param_symbol != 0) {
                params.symbols[params.count] = param_symbol;
                params.count++;
            }
        }
        
        current_param = param_node.children.child2; // Next parameter
    }
    
    return params;
}
```

#### **Step 5: Implement Parameter Prologue Generation**

**File:** `src/ir/tac_builder.c`

```c
/**
 * @brief Generate parameter initialization prologue for function
 */
static void emit_parameter_prologue(TACBuilder* builder, ParameterInfo* params) {
    for (uint32_t i = 0; i < params->count; i++) {
        SymIdx_t param_symbol = params->symbols[i];
        
        // Create variable operand for parameter
        TACOperand param_var = create_symbol_operand(builder, param_symbol);
        
        // Create parameter access operand
        TACOperand param_access = TAC_MAKE_PARAM_ACCESS(i);
        
        // Generate: param_var = parameter[i]
        tac_emit_instruction(builder, TAC_ASSIGN, param_var, param_access, TAC_OPERAND_NONE);
        
        printf("DEBUG: Generated parameter initialization: param_%u = parameter[%u]\n", 
               param_symbol, i);
    }
}
```

#### **Step 6: Update Function Definition Processing**

**File:** `src/ir/tac_builder.c`

```c
case AST_FUNCTION_DEF:
    {
        // Extract function name (existing code)
        SymIdx_t func_symbol_idx = ast_node.declaration.symbol_idx;
        char* func_name = get_symbol_name(func_symbol_idx);
        
        if (func_name) {
            // Generate function label (existing code)
            uint32_t func_idx = find_function_in_table(builder, func_name);
            if (func_idx != (uint32_t)-1) {
                TACOperand func_label = tac_new_label(builder);
                builder->function_table.label_ids[func_idx] = func_label.data.label.offset;
                
                tac_emit_instruction(builder, TAC_LABEL, func_label, TAC_OPERAND_NONE, TAC_OPERAND_NONE);
                builder->function_table.instruction_addresses[func_idx] = tacstore_getidx();
                
                // NEW: Generate parameter prologue
                ParameterInfo params = extract_function_parameters(&ast_node);
                if (params.count > 0) {
                    emit_parameter_prologue(builder, &params);
                    printf("DEBUG: Generated prologue for %u parameters in function '%s'\n", 
                           params.count, func_name);
                }
            }
        }
        
        // Process function body (existing code)
        // ...
    }
    break;
```

#### **Step 7: Update TAC Printer for New Operand Type**

**File:** `src/ir/tac_printer.c`

```c
void tac_print_operand(TACOperand operand) {
    switch (operand.type) {
        // ... existing cases ...
        
        case TAC_OP_PARAM_ACCESS:
            printf("param[%d]", operand.data.param_access.param_index);
            break;
            
        // ... rest of cases ...
    }
}
```

### **Expected Result**

**New TAC Output:**
```
L1:                    // factorial function entry
n = param[0]           // ✅ Initialize n from first parameter
result = 1             // ✅ Local variable initialization
i = 1                  // ✅ Local variable initialization
L2:                    // Loop start
t1 = i le n           // ✅ n now has correct value from parameter
if_false t1 goto L3
// ... rest of function
```

---

## 🔧 **Solution 2: Function Call Generation**

### **Problem Analysis**

**Current Issue:**
```c
// C Source
int main() {
    int result = factorial(5);
    return result;
}
```

**Current TAC:**
```
main:
// ... variable setup
factorial:             // ❌ Direct flow, no call instruction
// ... function body
```

**Expected TAC:**
```
main:
// ... variable setup
param 5                // ✅ Push parameter
t1 = call L1          // ✅ Function call
result = t1           // ✅ Capture return value
return result         // ✅ Return result
```

### **Implementation Steps**

#### **Step 1: Implement Function Call Detection**

**File:** `src/ir/tac_builder.c`

```c
case AST_FUNCTION_CALL:
    {
        // Extract function name
        char* func_name = extract_function_call_name(&ast_node);
        if (!func_name) {
            builder->error_count++;
            return TAC_OPERAND_NONE;
        }
        
        printf("DEBUG: Processing function call to '%s'\n", func_name);
        
        // Extract arguments
        ArgumentList args = extract_function_arguments(&ast_node);
        
        // Generate parameter pushing sequence
        for (uint32_t i = 0; i < args.count; i++) {
            TACOperand arg = tac_build_from_ast(builder, args.expressions[i]);
            if (arg.type != TAC_OP_NONE) {
                tac_emit_instruction(builder, TAC_PARAM, TAC_OPERAND_NONE, arg, TAC_OPERAND_NONE);
                printf("DEBUG: Generated param instruction for argument %u\n", i);
            }
        }
        
        // Look up function label
        TACOperand func_label = lookup_function_label(builder, func_name);
        if (func_label.type == TAC_OP_NONE) {
            printf("ERROR: Function '%s' not found in function table\n", func_name);
            builder->error_count++;
            return TAC_OPERAND_NONE;
        }
        
        // Generate function call
        TACOperand result_temp = tac_new_temp(builder, ast_node.type_idx);
        tac_emit_instruction(builder, TAC_CALL, result_temp, func_label, TAC_OPERAND_NONE);
        
        printf("DEBUG: Generated call instruction for function '%s'\n", func_name);
        
        return result_temp;
    }
```

#### **Step 2: Implement Function Name Extraction**

**File:** `src/ir/tac_builder.c`

```c
/**
 * @brief Extract function name from function call AST node
 */
static char* extract_function_call_name(ASTNode* call_node) {
    if (call_node->type != AST_FUNCTION_CALL) {
        return NULL;
    }
    
    // Navigate to function identifier
    // Typically: call_node->children.child1 contains the function identifier
    ASTNodeIdx_t func_id_idx = call_node->children.child1;
    if (func_id_idx == 0) {
        return NULL;
    }
    
    ASTNode func_id_node = astore_get(func_id_idx);
    if (func_id_node.type != AST_EXPR_IDENTIFIER) {
        return NULL;
    }
    
    // Get symbol from identifier
    SymIdx_t func_symbol = func_id_node.binary.value.symbol_idx;
    if (func_symbol == 0) {
        return NULL;
    }
    
    // Get function name from symbol table
    SymTabEntry symbol = symtab_get(func_symbol);
    return sstore_get(symbol.name);
}
```

#### **Step 3: Implement Argument Extraction**

**File:** `src/ir/tac_builder.c`

```c
/**
 * @brief Extract function call arguments
 */
static ArgumentList extract_function_arguments(ASTNode* call_node) {
    ArgumentList args = {0};
    
    // Navigate to argument list
    // Typically: call_node->children.child2 contains the argument list
    ASTNodeIdx_t arg_list_idx = call_node->children.child2;
    if (arg_list_idx == 0) {
        return args; // No arguments
    }
    
    ASTNode arg_list = astore_get(arg_list_idx);
    
    // Extract each argument expression
    ASTNodeIdx_t current_arg = arg_list.children.child1;
    while (current_arg != 0 && args.count < MAX_FUNCTION_ARGS) {
        ASTNode arg_node = astore_get(current_arg);
        
        args.expressions[args.count] = current_arg;
        args.count++;
        
        current_arg = arg_node.children.child2; // Next argument
    }
    
    return args;
}
```

#### **Step 4: Implement Function Label Lookup**

**File:** `src/ir/tac_builder.c`

```c
/**
 * @brief Look up function label by name
 */
static TACOperand lookup_function_label(TACBuilder* builder, const char* func_name) {
    for (uint32_t i = 0; i < builder->function_table.count; i++) {
        if (builder->function_table.function_names[i] && 
            strcmp(builder->function_table.function_names[i], func_name) == 0) {
            
            uint32_t label_id = builder->function_table.label_ids[i];
            return TAC_MAKE_LABEL(label_id);
        }
    }
    
    return TAC_OPERAND_NONE; // Function not found
}
```

#### **Step 5: Add Data Structures**

**File:** `src/ir/tac_builder.h`

```c
#define MAX_FUNCTION_PARAMS 16
#define MAX_FUNCTION_ARGS 16

typedef struct {
    SymIdx_t symbols[MAX_FUNCTION_PARAMS];
    uint32_t count;
} ParameterInfo;

typedef struct {
    ASTNodeIdx_t expressions[MAX_FUNCTION_ARGS];
    uint32_t count;
} ArgumentList;
```

### **Expected Result**

**New TAC Output:**
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
dx = x2 sub x1        // ✅ Function body with initialized params
dy = y2 sub y1
// ... rest of function
```

---

## 🔧 **Solution 3: Return Value Standardization**

### **Implementation Steps**

#### **Step 1: Standardize Return Location**

**File:** `src/ir/tac_builder.c`

```c
#define TAC_RETURN_TEMP 0  // Always use temp[0] for return values

case AST_RETURN:
    {
        if (ast_node.children.child1 != 0) {
            // Process return expression
            TACOperand return_value = tac_build_from_ast(builder, ast_node.children.child1);
            
            if (return_value.type != TAC_OP_NONE) {
                // Assign to standard return location
                TACOperand return_temp = TAC_MAKE_TEMP(TAC_RETURN_TEMP);
                tac_emit_instruction(builder, TAC_ASSIGN, return_temp, return_value, TAC_OPERAND_NONE);
                
                printf("DEBUG: Assigned return value to temp[%d]\n", TAC_RETURN_TEMP);
            }
        }
        
        // Emit return instruction
        tac_emit_instruction(builder, TAC_RETURN, TAC_OPERAND_NONE, TAC_OPERAND_NONE, TAC_OPERAND_NONE);
        
        return TAC_OPERAND_NONE;
    }
```

#### **Step 2: Update Function Call Return Handling**

**File:** `src/ir/tac_builder.c`

```c
// In function call processing (after TAC_CALL instruction):

// The call instruction places return value in a temporary
// But we need to ensure it's accessible in the standard location
TACOperand standard_return = TAC_MAKE_TEMP(TAC_RETURN_TEMP);
TACOperand call_result = result_temp; // From the call instruction

// Ensure return value is in standard location if needed
if (result_temp.data.variable.id != TAC_RETURN_TEMP) {
    tac_emit_instruction(builder, TAC_ASSIGN, standard_return, call_result, TAC_OPERAND_NONE);
}

return call_result;
```

---

## 🧪 **Testing Implementation**

### **Test Case 1: Simple Function Call**

**Input:**
```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(3, 5);
    return result;
}
```

**Expected TAC:**
```
L1:                    // add function
a = param[0]
b = param[1]
t1 = a add b
t0 = t1               // Return value to temp[0]
return

L2:                   // main function
param 3
param 5
t2 = call L1
result = t2
t0 = result
return
```

### **Test Case 2: Recursive Function**

**Input:**
```c
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main() {
    return factorial(5);
}
```

**Expected TAC:**
```
L1:                    // factorial function
n = param[0]
t1 = n le 1
if_false t1 goto L2
t0 = 1                // Return 1
return
L2:
t2 = n sub 1
param t2
t3 = call L1          // Recursive call
t4 = n mul t3
t0 = t4               // Return result
return

L3:                   // main function  
param 5
t5 = call L1
t0 = t5
return
```

---

## 📋 **Implementation Checklist**

### **Phase 1: Foundation**
- [ ] Add `TAC_OP_PARAM_ACCESS` operand type
- [ ] Add parameter access data structure
- [ ] Add `TAC_MAKE_PARAM_ACCESS` macro
- [ ] Add `ParameterInfo` and `ArgumentList` structures

### **Phase 2: Parameter Handling**
- [ ] Implement `extract_function_parameters()`
- [ ] Implement `emit_parameter_prologue()`
- [ ] Update `AST_FUNCTION_DEF` case in `tac_build_from_ast()`
- [ ] Update TAC printer for new operand type

### **Phase 3: Function Calls**
- [ ] Implement `extract_function_call_name()`
- [ ] Implement `extract_function_arguments()`
- [ ] Implement `lookup_function_label()`
- [ ] Update `AST_FUNCTION_CALL` case in `tac_build_from_ast()`

### **Phase 4: Return Values**
- [ ] Standardize return value location (temp[0])
- [ ] Update `AST_RETURN` case processing
- [ ] Update function call return value handling

### **Phase 5: Testing**
- [ ] Test simple function calls
- [ ] Test functions with multiple parameters
- [ ] Test recursive functions
- [ ] Verify existing tests still pass
- [ ] Test factorial(5) = 120
- [ ] Test distance_squared(0,0,3,4) = 25

### **Phase 6: Integration**
- [ ] Update TAC engine for new operand types (if needed)
- [ ] Update cc2t tool for new instruction patterns
- [ ] Performance testing and optimization
- [ ] Documentation updates

---

## 🎯 **Success Criteria**

1. **`test_integration_iterative_algorithm` passes** - factorial(5) returns 120
2. **`test_integration_mixed_declarations_and_scoping` passes** - distance calculation returns 25
3. **All existing tests continue to pass** - maintain 97%+ pass rate
4. **TAC output includes proper function call sequences** - param/call/return pattern
5. **Function parameters are properly initialized** - no more uninitialized variable usage

---

## 🚨 **Potential Issues & Solutions**

### **Issue 1: AST Structure Variations**
**Problem:** AST node structure may vary from assumptions  
**Solution:** Add robust AST navigation with null checks and error handling

### **Issue 2: Symbol Table Integration**
**Problem:** Parameter symbols may not be in expected locations  
**Solution:** Add comprehensive symbol table lookup and validation

### **Issue 3: TAC Engine Compatibility**
**Problem:** TAC engine may not support new operand types  
**Solution:** Verify TAC engine can handle `TAC_OP_PARAM_ACCESS` or add support

### **Issue 4: Memory Management**
**Problem:** Dynamic allocation for parameter/argument lists  
**Solution:** Use fixed-size arrays with reasonable limits (16 parameters/arguments)

---

## 📝 **Summary**

This implementation guide provides concrete steps to fix the two critical TAC limitations:

1. **Function parameters will be properly initialized** from the call stack using `param[n]` operands
2. **Function calls will generate proper instruction sequences** with param/call/return semantics

The implementation is designed to integrate cleanly with the existing TAC system while maintaining compatibility with current functionality. Upon completion, the STCC1 compiler will achieve 100% test pass rate and full support for multi-function C programs.

**Estimated Implementation Time:** 2-3 weeks  
**Risk Level:** Medium (requires careful AST navigation and TAC engine integration)  
**Impact:** High (fixes remaining 3% of test failures and completes TAC system)
