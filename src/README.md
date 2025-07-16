/**
 * @file README.md
 * @brief Source Code Organization for STCC1 Small C Compiler
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.3
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

# STCC1 Source Code Organization

This document describes the reorganized source code structure for the STCC1 small C compiler, optimized for extreme low memory consumption.

## Directory Structure

### `/src/lexer/` - Lexical Analysis
**Purpose**: Tokenization and lexical processing
- `cc0.c` - Main lexical analyzer (tokenizer)
- `cc0t.c` - Token dump utility 
- `ctoken.h` - Token definitions and structures

**Memory Strategy**: File-based token storage with efficient streaming

### `/src/parser/` - Syntax Analysis  
**Purpose**: Parsing and syntax tree generation
- `cc1.c` - Enhanced main parser with error handling
- `parse.y` - Yacc/Bison grammar specification
- `parser.c` - Basic parser implementation
- `enhanced_parser.c` - Modular parser with advanced features
- `modular_compiler.c` - Integrated compilation pipeline

**Memory Strategy**: Memory-efficient AST generation with file-backed storage

### `/src/ast/` - Abstract Syntax Tree
**Purpose**: AST node definitions and manipulation
- `astnode.h` - Legacy AST node structure (24-byte optimized)
- `ast_types.h` - Modular AST type system
- `ast_builder.h/.c` - AST construction utilities
- `ast_visitor.h` - Visitor pattern for AST traversal

**Memory Strategy**: Fixed-size AST nodes (24 bytes) with file-backed storage via astore

### `/src/storage/` - Memory Management
**Purpose**: File-backed storage systems for memory efficiency
- `sstore.h/.c` - String storage system
- `tstore.h/.c` - Token storage system  
- `astore.h/.c` - AST node storage system
- `symtab.h/.c` - Symbol table management

**Memory Strategy**: All major data structures stored on disk during runtime

### `/src/error/` - Error Handling
**Purpose**: Comprehensive error reporting and recovery
- `error_core.h/.c` - Stage-independent error infrastructure
- `error_stages.h/.c` - Stage-specific error handlers (lexical, syntax, semantic, codegen)
- `error_handler.h` - Error handling interfaces
- `error_recovery.h` - Error recovery strategies

**Features**: Structured error messages with codes, suggestions, and source location tracking

### `/src/utils/` - Utility Systems
**Purpose**: Core utility functions and memory management
- `hash.h/.c` - Hash table implementation
- `hmapbuf.h/.c` - Memory-mapped buffer system with LRU eviction
- `memory_config.h` - Memory configuration and limits

**Memory Strategy**: LRU-based memory management keeping only 64-100 nodes in RAM

### `/src/demo/` - Demonstration Code
**Purpose**: Example usage and testing utilities
- `error_demo.c` - Comprehensive error handling demonstration
- `simple_error_demo.c` - Simple error system demo

**Note**: Demo files are not part of the core compiler build

### `/src/tools/` - Development Tools
**Purpose**: Additional development and debugging tools
- (Reserved for future development tools)

### `/src/core/` - Core Definitions
**Purpose**: Core compiler definitions and interfaces
- (Reserved for future core system files)

## Build System Integration

The reorganized structure requires updates to the Makefile to reflect the new paths:

```makefile
# Source directories
SRCDIR = src
LEXER_SRC = $(SRCDIR)/lexer
PARSER_SRC = $(SRCDIR)/parser  
AST_SRC = $(SRCDIR)/ast
STORAGE_SRC = $(SRCDIR)/storage
ERROR_SRC = $(SRCDIR)/error
UTILS_SRC = $(SRCDIR)/utils
```

## Memory Efficiency Goals

- **Total Memory Usage**: <100KB runtime memory
- **File-Based Storage**: All major data structures (AST, symbols, tokens, strings)
- **LRU Management**: Keep only actively used nodes in memory
- **Fixed-Size Structures**: 24-byte AST nodes, optimized layouts

## Compilation Pipeline

1. **Lexical Analysis** (`lexer/cc0.c`): Source → Tokens → `tstore`
2. **Syntax Analysis** (`parser/cc1.c`): Tokens → AST → `astore` + `symtab`
3. **Error Handling** (`error/`): Comprehensive error reporting throughout all stages
4. **Storage Management** (`storage/`): File-backed persistence of all major data structures

## Development Guidelines

- **Memory Conscious**: Always consider file-based storage for large data structures
- **Error Handling**: Use modular error system for all error reporting
- **Testing**: Use demo utilities to validate functionality
- **Documentation**: Keep this README updated as the structure evolves

## Dependencies

```
lexer/ → storage/ (tstore, sstore)
parser/ → lexer/, ast/, storage/, error/
ast/ → storage/ (astore)
error/ → storage/ (token locations)
utils/ → (independent utility functions)
```

This organization separates concerns while maintaining the project's core goal of extreme memory efficiency through file-based storage systems.
