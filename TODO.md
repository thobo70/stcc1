# STCC1 Compiler TODO List

## 🎯 Project Overview
This TODO list is based on the grammar comparison between the current CC1 implementation (`src/parser/cc1.c`) and the formal grammar specification (`src/parser/parse.y`). The CC1 parser currently implements ~25% of the intended C grammar.

---

## 🚨 Critical Issues (Must Fix)

### 1. **Expression Parsing Bug** - HIGH PRIORITY
- **Problem**: Binary operators are parsed right-associative instead of left-associative
- **Example**: `a + b + c` parsed as `a + (b + c)` instead of `(a + b) + c`
- **Location**: `src/parser/cc1.c:208-235` in `parse_expression()`
- **Impact**: Incorrect code generation for all mathematical expressions
- **Status**: ❌ BROKEN

### 2. **Missing Function Call Support** - HIGH PRIORITY  
- **Problem**: Cannot parse function calls like `func(a, b, c)`
- **Location**: `src/parser/cc1.c` - needs new parsing function
- **Impact**: Cannot compile any program that calls functions
- **Status**: ❌ NOT IMPLEMENTED

---

## 🔧 Core Language Features (Medium Priority)

### 3. **Pointer Support**
- **Missing**: `int *ptr`, `**ptr`, `&variable`, `*ptr`
- **Location**: `src/parser/cc1.c` - needs pointer declarator parsing
- **Files to modify**: 
  - `parse_declaration()` for pointer declarations
  - `parse_expression()` for dereference/address-of operators
- **Status**: ❌ NOT IMPLEMENTED

### 4. **Array Support**
- **Missing**: `int arr[10]`, `arr[index]`, multi-dimensional arrays
- **Location**: `src/parser/cc1.c` - needs array declarator and subscript parsing
- **Dependencies**: Requires expression parsing improvements
- **Status**: ❌ NOT IMPLEMENTED

### 5. **Complete Operator Support**
- **Missing Unary**: `++`, `--`, `!`, `~`, `+`, `-` (unary)
- **Missing Binary**: All bitwise (`&`, `|`, `^`, `<<`, `>>`), logical (`&&`, `||`)
- **Missing Assignment**: `+=`, `-=`, `*=`, `/=`, `%=`, etc.
- **Missing Other**: `?:` (ternary), `,` (comma operator)
- **Location**: `src/parser/cc1.c:208-235` - expand `parse_expression()`
- **Status**: ❌ MOSTLY MISSING

---

## 🏗️ Data Structures (Medium Priority)

### 6. **Struct/Union Support**
- **Missing**: `struct { int x; float y; }`, `union Data { ... }`
- **Current**: Tokens recognized but not parsed
- **Location**: `src/parser/cc1.c` - needs `parse_struct_declaration()`
- **Status**: ❌ NOT IMPLEMENTED

### 7. **Enum Support**
- **Missing**: `enum Color { RED, BLUE, GREEN }`
- **Current**: Tokens recognized but not parsed  
- **Location**: `src/parser/cc1.c` - needs `parse_enum_declaration()`
- **Status**: ❌ NOT IMPLEMENTED

---

## 🔄 Control Flow (Medium Priority)

### 8. **For Loop Support**
- **Missing**: `for (init; condition; increment) statement`
- **Current**: Only `if` and `while` implemented
- **Location**: `src/parser/cc1.c:245-325` in `parse_statement()`
- **Status**: ❌ NOT IMPLEMENTED

### 9. **Do-While Loop Support**
- **Missing**: `do statement while (condition);`
- **Location**: `src/parser/cc1.c:245-325` in `parse_statement()`
- **Status**: ❌ NOT IMPLEMENTED

### 10. **Switch Statement Support**
- **Missing**: `switch (expr) { case 1: ... default: ... }`
- **Location**: `src/parser/cc1.c:245-325` in `parse_statement()`
- **Dependencies**: Requires label and jump statement support
- **Status**: ❌ NOT IMPLEMENTED

### 11. **Jump Statements**
- **Missing**: `break`, `continue`, `goto label`
- **Current**: Only `return` implemented
- **Location**: `src/parser/cc1.c:245-325` in `parse_statement()`
- **Status**: ❌ NOT IMPLEMENTED

---

## 🎨 Advanced Features (Lower Priority)

### 12. **Type Casting**
- **Missing**: `(int)value`, `(float*)ptr`
- **Location**: `src/parser/cc1.c` - needs cast expression parsing
- **Status**: ❌ NOT IMPLEMENTED

### 13. **sizeof Operator**
- **Missing**: `sizeof(int)`, `sizeof variable`
- **Location**: `src/parser/cc1.c` - needs unary expression parsing
- **Status**: ❌ NOT IMPLEMENTED

### 14. **Storage Class Processing**
- **Current**: Tokens recognized but not processed
- **Missing**: Proper handling of `static`, `extern`, `auto`, `register`
- **Location**: `src/parser/cc1.c:524-640` in `parse_type_specifiers()`
- **Status**: ⚠️ PARTIALLY IMPLEMENTED

### 15. **Type Qualifiers**
- **Missing**: Proper handling of `const`, `volatile`
- **Location**: `src/parser/cc1.c:524-640` in `parse_type_specifiers()`
- **Status**: ⚠️ PARTIALLY IMPLEMENTED

---

## 🔄 Parser Architecture Improvements

### 16. **Implement Proper Operator Precedence**
- **Problem**: Current expression parsing doesn't handle precedence correctly
- **Solution**: Implement precedence climbing or Pratt parser
- **Location**: `src/parser/cc1.c:208-235`
- **References**: Use `parse.y` precedence rules as specification
- **Status**: ❌ NEEDS COMPLETE REWRITE

### 17. **Error Recovery Improvements**
- **Current**: Basic error reporting
- **Needed**: Better synchronization on syntax errors
- **Location**: Throughout `src/parser/cc1.c`
- **Status**: ⚠️ BASIC IMPLEMENTATION

### 18. **Parameter List Parsing**
- **Current**: Very basic parameter parsing in `parse_declaration()`
- **Missing**: Proper parameter type parsing, default values, variadic functions
- **Location**: `src/parser/cc1.c:365-385`
- **Status**: ⚠️ MINIMAL IMPLEMENTATION

---

## 📋 Testing & Validation

### 19. **Comprehensive Test Suite**
- **Missing**: Tests for all grammar features
- **Current**: Basic test files in `src/test/`
- **Needed**: Test cases for each TODO item above
- **Status**: ❌ MINIMAL TESTING

### 20. **Grammar Validation**
- **Task**: Ensure CC1 implementation matches `parse.y` specification
- **Tool**: Create automated comparison between actual vs. expected parsing
- **Status**: ❌ NOT IMPLEMENTED

---

## 🏁 Migration Strategy

### 21. **Consider Using parse.y**
- **Option**: Replace hand-written parser with yacc/bison generated parser
- **Pros**: Complete grammar support, proven correctness
- **Cons**: Changes build system, different AST generation approach
- **Status**: 🤔 DECISION NEEDED

### 22. **Incremental Implementation Plan**
1. **Phase 1**: Fix expression parsing (#1, #5, #16)
2. **Phase 2**: Add function calls (#2)  
3. **Phase 3**: Add pointers and arrays (#3, #4)
4. **Phase 4**: Add remaining control flow (#8, #9, #10, #11)
5. **Phase 5**: Add data structures (#6, #7)
6. **Phase 6**: Add advanced features (#12, #13, #14, #15)

---

## 📊 Current Status Summary

| Category | Implemented | Partially Working | Missing | Total |
|----------|-------------|-------------------|---------|-------|
| **Types** | 5 | 2 | 3 | 10 |
| **Expressions** | 2 | 1 | 8 | 11 |
| **Statements** | 3 | 0 | 5 | 8 |
| **Declarations** | 2 | 2 | 4 | 8 |
| **Total** | **12** | **5** | **20** | **37** |

**Overall Completion**: ~32% (12/37 features fully working)

---

## 🎯 Immediate Next Steps

1. **Fix expression precedence** (Critical bug affecting all math)
2. **Add function call support** (Blocking most real C programs)  
3. **Create test cases** for current functionality
4. **Decide on migration strategy** (keep hand-written vs. use parse.y)

---

*Last updated: July 17, 2025*
*Based on comparison between `src/parser/cc1.c` and `src/parser/parse.y`*
