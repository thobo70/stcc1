# Modular AST System Restructuring

## Overview

The AST handling has been completely restructured to be modular and usable across different compiler stages. This design supports clean separation of concerns and enables easier maintenance and extension.

## Key Components

### 1. AST Type System (`ast_types.h`)

**Organized Node Categories:**
- **Special nodes**: Program, EOF, Error handling
- **Declaration nodes**: Functions, variables, types
- **Type nodes**: Basic types, pointers, arrays
- **Statement nodes**: Control flow, blocks, expressions
- **Expression nodes**: Operations, literals, calls

**Memory-Optimized Structure:**
- Fixed 24-byte `ASTNode` structure
- Union-based flexible child organization
- Compile-time size verification
- Phase flags for tracking compilation progress

**Key Features:**
```c
typedef struct ASTNode {
    ASTNodeType type;       // 2 bytes - node type
    ASTNodeFlags flags;     // 2 bytes - compiler phase flags  
    TokenIdx_t token_idx;   // 2 bytes - source reference
    TypeIdx_t type_idx;     // 2 bytes - type information
    
    // Flexible 16-byte union for different node structures
    union {
        struct { ASTNodeIdx_t child1, child2, child3, child4; } children;
        struct { ASTNodeIdx_t left, right; /* value */ } binary;
        struct { ASTNodeIdx_t operand; /* data */ } unary;
        struct { /* function-specific fields */ } call;
        // ... specialized layouts
    };
} ASTNode;
```

### 2. AST Builder (`ast_builder.h/.c`)

**Phase-Aware Construction:**
- Builder context tracks current compilation phase
- Automatic flag management for different phases
- Error counting and reporting integration
- Memory-efficient node creation

**Specialized Builders:**
```c
// High-level builders for different constructs
ASTNodeIdx_t ast_build_function_def(builder, decl, body);
ASTNodeIdx_t ast_build_if_stmt(builder, condition, then_stmt, else_stmt);
ASTNodeIdx_t ast_build_binary_expr(builder, op_token, left, right);
ASTNodeIdx_t ast_build_integer_literal(builder, token, value);
```

**Memory Management:**
- Integration with existing `hmapbuf` system
- Automatic node validation
- Flag-based optimization tracking

### 3. AST Visitor Pattern (`ast_visitor.h`)

**Flexible Traversal:**
- Pre-order, post-order, and level-order traversal
- Category-specific callbacks (declarations, statements, expressions)
- Conditional traversal based on node flags
- Error handling and recovery

**Visitor Structure:**
```c
typedef struct ASTVisitor {
    // Generic callbacks
    ast_visit_func_t pre_visit, post_visit;
    
    // Category-specific callbacks
    ast_visit_func_t visit_declaration;
    ast_visit_func_t visit_statement;
    ast_visit_func_t visit_expression;
    
    // Traversal control
    int max_depth;
    ASTNodeFlags skip_flags, only_flags;
    void* context;
} ASTVisitor;
```

### 4. Enhanced Parser (`enhanced_parser.c`)

**Modular Integration:**
- Uses AST builder for all node creation
- Integrated error handling and recovery
- Phase-aware flag setting
- Clean separation from AST structure details

**Example Usage:**
```c
// Old approach
ASTNodeIdx_t node = create_ast_node(AST_OPERATOR, token);
HBNode *hb_node = HBGet(node, HBMODE_AST);
hb_node->ast.o1 = left;
hb_node->ast.o2 = right;

// New modular approach  
ASTNodeIdx_t node = ast_build_binary_expr(&g_parser_builder, 
                                         op_token, left, right);
```

### 5. Compilation Driver (`modular_compiler.c`)

**Multi-Phase Pipeline:**
1. **Parsing**: AST construction with syntax validation
2. **Semantic Analysis**: Type checking, symbol resolution
3. **Optimization**: Constant folding, dead code elimination
4. **Code Generation**: Assembly output

**Phase Isolation:**
```c
// Each phase uses visitors with specific contexts
ASTVisitor semantic_visitor;
semantic_visitor.pre_visit = semantic_analyze_node;
SemanticContext ctx = {&builder, 0, 0};
ast_visit_subtree(&semantic_visitor, root_node);
```

## Benefits of Modular Design

### 1. **Clear Separation of Concerns**
- Each phase has its own builder and visitor
- AST structure is independent of processing logic
- Easy to add new phases or modify existing ones

### 2. **Memory Efficiency**
- Fixed-size nodes with flexible internal structure
- Efficient packing of common node patterns
- Phase flags prevent redundant processing

### 3. **Extensibility**
- New node types can be added easily
- Visitors can be customized for specific tasks
- Multiple passes can operate on the same AST

### 4. **Error Handling**
- Integrated error reporting with source location
- Recovery mechanisms built into visitor pattern
- Phase-specific error contexts

### 5. **Debugging and Analysis**
- Built-in AST printing and validation
- Tree statistics and memory profiling
- Phase-by-phase progress tracking

## Usage Examples

### Building AST Nodes
```c
ASTBuilder builder;
ast_builder_init(&builder, "Parser");

// Create a function with return statement
ASTNodeIdx_t return_val = ast_build_integer_literal(&builder, token, 42);
ASTNodeIdx_t return_stmt = ast_build_return_stmt(&builder, token, return_val);
ASTNodeIdx_t func_body = ast_build_compound_stmt(&builder, token, return_stmt);
ASTNodeIdx_t func_def = ast_build_function_def(&builder, func_decl, func_body);
```

### Traversing and Analyzing
```c
// Type checking visitor
ASTVisitor type_checker;
ast_visitor_init(&type_checker);
type_checker.visit_expression = check_expression_types;
type_checker.visit_declaration = check_declaration_types;

TypeContext ctx = {/* type checking state */};
type_checker.context = &ctx;

ast_visit_subtree(&type_checker, program_root);
```

### Code Generation
```c
// Code generation visitor
ASTVisitor codegen;
ast_visitor_init(&codegen);
codegen.pre_visit = generate_code_for_node;

CodeGenContext ctx = {output_file, 0, 0};
codegen.context = &ctx;

ast_visit_subtree(&codegen, program_root);
```

## Integration with Existing System

The modular AST system seamlessly integrates with:
- **hmapbuf**: LRU buffer management for AST nodes
- **sstore**: String storage for identifiers and literals
- **tstore**: Token storage for source references
- **Error handling**: Integrated error reporting and recovery

## Build and Test

```bash
# Build the modular compiler
make -f Makefile.modular modular

# Test the modular system
make -f Makefile.modular test_modular

# Demonstrate AST phases
make -f Makefile.modular demo_ast

# Profile memory usage
make -f Makefile.modular profile_memory
```

This modular design provides a solid foundation for extending the compiler while maintaining the low-memory footprint that is central to the project's goals.
