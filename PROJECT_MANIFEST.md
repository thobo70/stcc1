# STCC1 Compiler Project Manifest
## Development Principles & Standards Declaration

**Project:** STCC1 - Simple Three-Address Code C Compiler  
**Version:** 1.0  
**Date:** July 27, 2025  
**Owner:** thobo70  

---

## 🎯 **CORE MISSION STATEMENT**

STCC1 is a modular, educational C compiler that transforms C source code into three-address code (TAC) through distinct lexical, parsing, and code generation stages. This project prioritizes **code quality, correctness, and maintainability** over rapid feature delivery.

---

## 🛡️ **FUNDAMENTAL PRINCIPLES**

### 0. **SAFE MEMORY - MEMORY SHALL BE CONSIDERED TO BE VERY SMALL**

### 1. **TEST INTEGRITY - NEVER COMPROMISE**
- ❌ **NEVER weaken a test to make it pass**
- ❌ **NEVER skip failing tests "temporarily"**
- ❌ **NEVER reduce test coverage to avoid failures**
- ✅ **FIX the code, not the test**
- ✅ **Add more tests when bugs are found**
- ✅ **Maintain 100% test success rate**

### 2. **CODE CORRECTNESS FIRST**
- ❌ **NEVER commit code that breaks existing functionality**
- ❌ **NEVER ignore compiler warnings**
- ❌ **NEVER use workarounds instead of proper fixes**
- ✅ **Fix root causes, not symptoms**
- ✅ **Validate all changes with comprehensive testing**
- ✅ **Maintain clean, readable, documented code**

### 3. **COMPILER PIPELINE INTEGRITY**
- ❌ **NEVER break the cc0 → cc1 → cc2 pipeline**
- ❌ **NEVER introduce stage dependencies that shouldn't exist**
- ❌ **NEVER compromise TAC output correctness**
- ✅ **Maintain clear separation between lexer, parser, and TAC generator**
- ✅ **Ensure each stage can be tested independently**
- ✅ **Validate TAC semantics match C source semantics**

### 4. **NEVER MIX SOURCES WITH BUILD ARTIFACTS**

---

## 📋 **DEVELOPMENT STANDARDS**

### **Quality Gates**
Every change MUST pass these gates before integration:

1. **Build Gate**: `make clean && make` - Zero warnings, clean build
2. **Test Gate**: `make test` - All Unity tests pass (0 failures)
3. **Compiler Gate**: `make test-compiler` - All pipeline tests pass
4. **Documentation Gate**: Update relevant documentation
5. **Code Review Gate**: Peer review for architectural changes

### **Testing Requirements**
- **Unit Tests**: Cover all new functions and modules
- **Integration Tests**: Validate multi-stage interactions
- **Regression Tests**: Preserve all existing functionality
- **Edge Case Tests**: Handle boundary conditions and errors
- **Performance Tests**: Maintain reasonable compilation speed

### **Code Quality Standards**
- **CPPLINT Compliance**: Follow established style guidelines
- **Memory Safety**: No leaks, proper cleanup, bounds checking
- **Error Handling**: Graceful failure modes, informative messages
- **Documentation**: Doxygen comments for all public interfaces
- **Modularity**: Clear interfaces, minimal coupling

---

## 🔧 **TECHNICAL GOVERNANCE**

### **Architecture Principles**
- **Modular Design**: Each component has a single, well-defined responsibility
- **Clean Interfaces**: Minimal, documented APIs between components
- **Error Propagation**: Consistent error handling across all stages
- **Storage Management**: Centralized memory and symbol management
- **Extensibility**: Design for future language feature additions

### **File Organization Standards**
```
src/
├── lexer/          # cc0 - Token generation only
├── parser/         # cc1 - AST construction only  
├── ir/             # cc2 - TAC generation only
├── ast/            # AST types and utilities
├── storage/        # Symbol tables, string storage
├── error/          # Error handling and recovery
└── utils/          # Shared utilities (hash, buffers)
```

### **Naming Conventions**
- **Functions**: `snake_case` (e.g., `ast_build_expression`)
- **Types**: `PascalCase` with prefix (e.g., `ASTNode`, `TACInstruction`)  
- **Constants**: `UPPER_SNAKE_CASE` (e.g., `MAX_IDENTIFIER_LENGTH`)
- **Files**: `snake_case.c/.h` matching module purpose

---

## 🚫 **STRICT PROHIBITIONS**

### **Code Quality Violations**
- **No Magic Numbers**: Use named constants
- **No Global Variables**: Pass context explicitly
- **No Hardcoded Paths**: Use configurable defaults
- **No Copy-Paste Code**: Extract common functionality
- **No TODO Comments**: File issues for future work

### **Testing Violations**
- **No Flaky Tests**: Tests must be deterministic
- **No Environment Dependencies**: Tests run in isolation
- **No Manual Test Steps**: All testing automated
- **No Commented-Out Tests**: Remove or fix, don't disable
- **No Test Pollution**: Each test cleans up after itself

### **Architecture Violations**
- **No Circular Dependencies**: Maintain acyclic dependency graph
- **No Stage Bypassing**: Each compilation stage must execute
- **No Direct File Access**: Use abstracted storage interfaces
- **No Embedded SQL/External DBs**: Keep it simple and portable
- **No Platform-Specific Code**: Maintain Linux/Unix compatibility

---

## ✅ **MANDATORY PRACTICES**

### **Before Every Commit**
1. Run full test suite: `make test && make test-compiler`
2. Check for memory leaks with valgrind (when available)
3. Verify documentation is up to date
4. Ensure commit message describes the "why" not just "what"
5. Validate that change serves the project mission

### **When Adding Features**
1. Write tests FIRST (TDD approach)
2. Implement minimal viable solution
3. Refactor for clarity and maintainability
4. Document the feature and its limitations
5. Update integration tests to cover new paths

### **When Fixing Bugs**
1. Write a failing test that reproduces the bug
2. Fix the bug with minimal code change
3. Ensure the test now passes
4. Add related edge case tests
5. Document the fix and root cause

---

## 📊 **SUCCESS METRICS**

### **Primary Metrics**
- **Test Success Rate**: 100% (Zero tolerance for failing tests)
- **Build Success Rate**: 100% (Zero warnings, clean compilation)
- **Memory Safety**: Zero leaks, zero overflows
- **Code Coverage**: >90% for critical paths
- **Documentation Coverage**: 100% for public APIs

### **Quality Indicators**
- **Compilation Speed**: Sub-second for typical programs
- **TAC Correctness**: Semantic equivalence with C source
- **Error Messages**: Clear, actionable, location-specific
- **Code Maintainability**: Easy to understand and modify
- **Platform Portability**: Works on all target Unix systems

---

## 🎓 **EDUCATIONAL MISSION**

As an educational compiler project, STCC1 must:

### **Demonstrate Best Practices**
- Clean, readable, well-documented code
- Proper error handling and edge case management
- Modular architecture with clear separation of concerns
- Comprehensive testing at all levels
- Professional development workflow

### **Teaching Objectives**
- **Lexical Analysis**: How tokens are extracted from source
- **Parsing**: How ASTs are built from token streams
- **Code Generation**: How high-level constructs become low-level code
- **Symbol Management**: How identifiers and scopes are tracked
- **Error Recovery**: How compilers handle and report problems

---

## 🔄 **CONTINUOUS IMPROVEMENT**

### **Regular Reviews**
- **Weekly**: Test suite health and coverage analysis
- **Monthly**: Code quality metrics and architecture review
- **Quarterly**: Performance benchmarking and optimization
- **Annually**: Major refactoring and modernization

### **Evolution Principles**
- **Backward Compatibility**: Existing test cases must continue to pass
- **Incremental Change**: Small, well-tested improvements
- **Documentation First**: Design decisions documented before implementation
- **Community Input**: Welcome feedback, but maintain project vision
- **Technical Debt**: Regular refactoring to prevent accumulation

---

## 📞 **ACCOUNTABILITY**

### **Project Stewardship**
- **Owner**: thobo70 (final authority on architectural decisions)
- **Contributors**: Must follow this manifest for all contributions
- **Reviewers**: Enforce manifest compliance in code reviews
- **Users**: Report violations of principles or standards

### **Violation Response**
1. **Immediate**: Revert changes that violate core principles
2. **Short-term**: Fix root cause and strengthen prevention
3. **Long-term**: Update manifest to address new scenarios
4. **Always**: Learn from violations to improve processes

---

## 🎖️ **COMMITMENT DECLARATION**

By working on this project, all contributors commit to:

1. **Upholding** these principles even when under pressure
2. **Advocating** for quality over quick fixes
3. **Mentoring** others in these practices
4. **Evolving** the manifest as the project grows
5. **Maintaining** the educational value and code quality

---

## 📜 **MANIFEST SIGNATURE**

```
Project: STCC1 Compiler
Manifest Version: 1.0
Date: July 27, 2025
Author: GitHub Copilot (per user request)
Status: ACTIVE AND BINDING

"In code we trust, but we test everything."
- STCC1 Project Team
```

---

**Remember: This manifest is a living document. It evolves with the project but never compromises on core principles of quality, correctness, and educational value.**
