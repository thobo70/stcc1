# STCC1 Compiler Improvements Plan

## Priority 1: Core Parser Completion

### 1.1 Expand Grammar Support
- [ ] Complete C expression parsing (arithmetic, logical, comparison)
- [ ] Implement full statement parsing (if/while/for/return)
- [ ] Add function parameter and body parsing
- [ ] Support variable declarations with initializers
- [ ] Handle pointer declarations and operations

### 1.2 AST Enhancement
- [ ] Define complete AST node types for all C constructs
- [ ] Implement proper parent-child relationships in AST
- [ ] Add AST traversal and manipulation utilities
- [ ] Implement AST pretty-printing for debugging

### 1.3 Symbol Table Improvements
- [ ] Implement scope management (global, function, block scopes)
- [ ] Add type checking and symbol resolution
- [ ] Support function signatures and parameter lists
- [ ] Handle struct/union member resolution

## Priority 2: Memory Optimization Enhancements

### 2.1 Buffer Management Optimization
- [ ] Tune HBNNODES size based on typical compilation units
- [ ] Implement more sophisticated LRU eviction policies
- [ ] Add memory usage monitoring and reporting
- [ ] Consider implementing memory pools for different node types

### 2.2 Storage Efficiency
- [ ] Implement compression for string storage
- [ ] Optimize token representation (bit-packing)
- [ ] Add incremental parsing support
- [ ] Implement lazy loading for large compilation units

## Priority 3: Code Generation Phase

### 3.1 Intermediate Representation
- [ ] Design simple IR for target-independent optimization
- [ ] Implement AST to IR translation
- [ ] Add basic optimizations (constant folding, dead code elimination)

### 3.2 Target Code Generation
- [ ] Implement x86-64 assembly generation
- [ ] Add register allocation
- [ ] Support function calls and stack management
- [ ] Handle different data types and operations

## Priority 4: Error Handling and Diagnostics

### 4.1 Enhanced Error Reporting
- [ ] Implement precise error location tracking
- [ ] Add meaningful error messages with suggestions
- [ ] Support error recovery in parser
- [ ] Create comprehensive test suite for error cases

### 4.2 Debugging Support
- [ ] Add debug symbol generation
- [ ] Implement source line mapping
- [ ] Create internal debugging utilities

## Priority 5: Testing and Documentation

### 5.1 Test Infrastructure
- [ ] Create comprehensive test suite for each component
- [ ] Add regression tests
- [ ] Implement fuzzing for robustness testing
- [ ] Add performance benchmarks

### 5.2 Documentation
- [ ] Update README with proper project description
- [ ] Document the memory management strategy
- [ ] Create developer documentation
- [ ] Add code generation examples

## Memory Consumption Targets

| Component | Current | Target | Strategy |
|-----------|---------|--------|----------|
| Parser State | ~1KB | <500B | Minimize parser stack depth |
| AST Nodes | 100 nodes | Configurable | LRU buffer management |
| Symbol Table | File-based | <2KB RAM | Hash table with disk backing |
| String Storage | File-based | <1KB cache | String interning with LRU |

## Performance Goals

- Compile 1000-line C files in <100MB RAM usage
- Single-pass compilation where possible  
- O(1) symbol lookup with bounded memory
- Streaming token processing to minimize memory footprint
