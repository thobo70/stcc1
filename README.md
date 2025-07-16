# STCC1 - Small C Compiler with Extreme Low Memory Consumption

STCC1 is a C compiler implementation designed with extreme low memory consumption in mind. The project focuses on being able to compile C code in environments with severely constrained memory resources (target: <100KB RAM).

## Key Features

- **Ultra-Low Memory Footprint**: Custom memory management designed for <100KB RAM usage
- **File-Based Storage**: AST, symbols, and tokens stored on disk to minimize RAM usage  
- **LRU Buffer Management**: Intelligent caching system with only 64-100 nodes in memory
- **String Interning**: Efficient string storage with deduplication
- **Multi-Phase Architecture**: Separate lexer, parser, and code generator phases

## Architecture Overview

The compiler consists of several phases:

1. **cc0** - Lexical Analyzer: Tokenizes C source code
2. **cc0t** - Token Processor: Test utility for token stream
3. **cc1** - Parser: Builds Abstract Syntax Tree and Symbol Table
4. **Future: Code Generator** - Will generate target assembly

### Memory Management Strategy

- **hmapbuf**: Hash map buffer with LRU eviction for AST/symbol nodes
- **sstore**: String storage with file backing and memory cache  
- **tstore**: Token storage with disk persistence
- **astore**: Abstract Syntax Tree storage system
- **Enhanced error handling**: Multi-stage error reporting with recovery
- **Modular AST system**: Visitor pattern for tree traversal and manipulation

## Building

```bash
# Clean and build all components
make clean && make all

# Run tests
make test

# Check code quality
make lint  # Optional: if you want to add this target

# Generate documentation
make doc
```

## Current Status

✅ **Completed Components:**
- Lexical analysis (fully functional)
- Token storage and management
- String storage with deduplication  
- Custom low-memory buffer management
- AST construction and management
- Enhanced parser with symbol table support
- Comprehensive error handling system
- Code quality improvements (591 cpplint warnings reduced from 649)

🚧 **In Progress:**
- Full C grammar support completion
- Advanced error recovery mechanisms
- Memory optimization refinements

❌ **Planned:**
- Code generation phase
- Optimization passes
- Advanced memory optimizations

## Usage

```bash
# Tokenize C source file
./bin/cc0 input.c strings.out tokens.out

# Process tokens (test utility)  
./bin/cc0t strings.out tokens.out

# Parse and build AST
./bin/cc1 strings.out tokens.out ast.out symbols.out
```

## Memory Budget

| Component | Memory Target | Current Status |
|-----------|---------------|----------------|
| Parser State | <512B | ✅ Achieved |
| AST Buffer | ~6.4KB (64 nodes) | ✅ Configurable |
| Symbol Cache | <2KB | ✅ File-backed |
| String Cache | <1KB | ✅ LRU managed |
| **Total** | **<100KB** | 🎯 **On Track** |

## Testing

The test suite processes the lexer source code itself:

```bash
make test
```

This will:
1. Preprocess `src/cc0.c` to generate test input
2. Tokenize the preprocessed source
3. Parse and build AST/symbol tables
4. Report memory usage statistics

## Contributing

This project follows a specific memory-conscious development approach:

1. **Memory First**: All changes must consider memory impact
2. **Measure Everything**: Use built-in memory tracking tools
3. **Test Early**: Run `make test` frequently to check memory usage
4. **Code Quality**: Maintain cpplint compliance (currently 591 warnings, down from 649)
5. **Document Decisions**: Explain memory trade-offs in code comments

### Code Quality Standards
- Use proper header guard naming (`SRC_PATH_FILENAME_H_`)
- Fixed-width integer types (`uint16_t`, `int64_t`) instead of `short`/`long`
- No trailing whitespace
- 80-character line length limit
- Comprehensive copyright headers

See [IMPROVEMENTS.md](IMPROVEMENTS.md) for detailed development roadmap.

## Performance Goals

- Compile 1000-line C files in <100MB total memory
- Single-pass parsing where possible
- O(1) symbol lookup with bounded memory usage
- Streaming processing to minimize memory footprint

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

Thomas Boos (tboos70@gmail.com)