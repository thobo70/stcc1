# STCC1 - Small C Compiler with Extreme Low Memory Consumption

STCC1 is a C compiler implementation designed with extreme low memory consumption in mind. The project focuses on being able to compile C code in environments with severely constrained memory resources (target: <100KB RAM).

## Key Features

- **Ultra-Low Memory Footprint**: Custom memory management designed for <100KB RAM usage
- **File-Based Storage**: AST, symbols, and tokens stored on disk to minimize RAM usage  
- **LRU Buffer Management**: Intelligent caching system with only 64-100 nodes in memory
- **String Interning**: Efficient string storage with deduplication
- **Multi-Phase Architecture**: Separate lexer, parser, and code generator phases

## Architecture Overview

STCC1 implements a traditional multi-pass compiler architecture with disk-based intermediate storage to minimize memory usage. The compiler pipeline consists of several distinct stages:

### Stage 1: Lexical Analysis
- **cc0** - Lexical Analyzer: Tokenizes C source code into a stream of tokens
- **cc0t** - Token Viewer: Debug utility to inspect the generated token stream

### Stage 2: Parsing & Analysis  
- **cc1** - Parser: Builds Abstract Syntax Tree (AST) and Symbol Table from tokens
- **cc1t** - AST Viewer: Debug utility to inspect generated AST and symbol table

### Stage 3: Intermediate Code Generation
- **cc2** - TAC Generator: Generates Three Address Code (TAC) from AST
- **cc2t** - TAC Viewer: Debug utility to inspect generated intermediate code

### Stage 4: Code Generation (Planned)
- **Future: Assembly Generator** - Will generate target assembly from TAC

### Compiler Pipeline & Data Flow

```
Source Code (.c)
      ↓
   [cc0] ← Lexical Analysis
      ↓
sstore.out + tokens.out ← String storage + Token stream
      ↓
   [cc1] ← Parsing & Symbol Analysis  
      ↓
ast.out + sym.out ← AST storage + Symbol table
      ↓
   [cc2] ← TAC Generation
      ↓
tac_output.tac ← Three Address Code
      ↓
[Future: cc3] ← Code Generation
      ↓
Assembly/Object Code
```

### File-Based Intermediate Storage

Each stage communicates through structured binary files to minimize memory usage:

- **sstore.out**: String storage file with deduplicated string literals and identifiers
- **tokens.out**: Token stream with token IDs and string references  
- **ast.out**: Abstract Syntax Tree nodes stored as binary structures
- **sym.out**: Symbol table with function/variable declarations and scope information
- **tac_output.tac**: Three Address Code intermediate representation

### Grammar Support Status

The current parser implements a **subset of C grammar** (~25% coverage):

✅ **Currently Supported:**
- Basic types: `int`, `char`, `float`, `double`, `void`
- Type modifiers: `signed`, `unsigned`, `long`, `short`
- Variable declarations: `int x;`, `int x = 5;`
- Function definitions: `int func(int a, int b) { ... }`
- Basic expressions: `a + b`, `x = y`, `a == b`
- Control flow: `if`, `while`, `return`, compound statements `{}`

❌ **Not Yet Supported:**
- Pointers: `int *ptr`, `&variable`, `*ptr`
- Arrays: `int arr[10]`, `arr[index]`
- Function calls: `func(a, b, c)` 
- Structs/unions: `struct { int x; }`
- Advanced operators: `++`, `--`, `&&`, `||`, `?:`
- Control flow: `for`, `do-while`, `switch`

> **Note**: See [TODO.md](TODO.md) for complete feature roadmap and implementation status.

### Memory Management Strategy

STCC1 implements sophisticated memory management to achieve the <100KB target:

- **hmapbuf**: Hash map buffer with LRU eviction for AST/symbol nodes (configurable 64-100 nodes)
- **sstore**: String storage with file backing and memory cache for deduplication
- **tstore**: Token storage with disk persistence and minimal RAM footprint
- **astore**: Abstract Syntax Tree storage system with on-demand loading
- **Enhanced error handling**: Multi-stage error reporting with recovery mechanisms
- **Modular AST system**: Visitor pattern for tree traversal and manipulation

### Storage Components Deep Dive

| Component | Purpose | Memory Impact | File Format |
|-----------|---------|---------------|-------------|
| **sstore** | String deduplication & storage | <1KB cache | Binary with string table |
| **tstore** | Token stream persistence | <512B buffer | Token ID + position pairs |
| **astore** | AST node storage & retrieval | ~6.4KB buffer | Binary AST structures |
| **symtab** | Symbol table management | <2KB cache | Symbol records with scope |
| **hmapbuf** | Generic buffer management | Configurable | LRU hash map structure |

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
- **Lexical Analysis**: Full C tokenization with proper token IDs and position tracking
- **Token Management**: Efficient disk-based token storage with minimal memory footprint
- **String Storage**: Deduplication system with file backing and LRU cache management
- **Parser Core**: Recursive descent parser supporting basic C constructs
- **AST Generation**: Abstract Syntax Tree construction and file-based persistence  
- **Symbol Table**: Function and variable symbol tracking with scope management
- **TAC Generation**: Three Address Code intermediate representation (basic implementation)
- **Memory Management**: Custom buffer system achieving <100KB target
- **Error Handling**: Comprehensive error reporting with source location tracking
- **Debug Tools**: Complete set of viewers for tokens, AST, symbols, and TAC

🚧 **In Progress:**
- **Grammar Completion**: Expanding from ~25% to fuller C language support
- **Expression Precedence**: Fixing operator associativity and precedence issues
- **Advanced Features**: Pointers, arrays, function calls, and complex expressions

❌ **Planned:**
- **Backend Code Generation**: Assembly/object code generation from TAC
- **Optimization Passes**: Dead code elimination, constant folding, register allocation
- **Extended C Features**: Preprocessor, advanced type system, full standard library support

## Usage

### Basic Compilation Pipeline

```bash
# Step 1: Tokenize C source file
./bin/cc0 input.c sstore.out tokens.out

# Step 2: Parse and build AST + Symbol Table
./bin/cc1 sstore.out tokens.out ast.out sym.out

# Step 3: Generate Three Address Code (TAC)
./bin/cc2 sstore.out tokens.out ast.out sym.out tac_output.tac
```

### Debug and Inspection Tools

```bash
# View generated tokens (debug utility)
./bin/cc0t sstore.out tokens.out

# View AST and symbol table (debug utility)  
./bin/cc1t sstore.out ast.out sym.out

# View generated TAC (debug utility)
./bin/cc2t tac_output.tac
```

### Example Workflow

```bash
# Create a simple C program
echo 'int add(int a, int b) { return a + b; }' > example.c

# Run complete pipeline
./bin/cc0 example.c sstore.out tokens.out
./bin/cc1 sstore.out tokens.out ast.out sym.out  
./bin/cc2 sstore.out tokens.out ast.out sym.out tac_output.tac

# Inspect results
echo "=== Tokens ==="
./bin/cc0t sstore.out tokens.out

echo "=== AST & Symbols ==="
./bin/cc1t sstore.out ast.out sym.out

echo "=== TAC ==="
./bin/cc2t tac_output.tac
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

### Automated Test Suite

The test suite processes the lexer source code itself as validation:

```bash
make test
```

This will:
1. Preprocess `src/test/simpletest.c` to generate test input
2. Tokenize the preprocessed source
3. Parse and build AST/symbol tables
4. Generate TAC intermediate code
5. Report memory usage statistics for each stage

### Manual Testing Examples

```bash
# Test basic declarations
echo 'int x = 5;' | ./bin/cc0 /dev/stdin sstore.out tokens.out
./bin/cc1 sstore.out tokens.out ast.out sym.out

# Test function definitions (currently supported)
echo 'int add(int a, int b) { return a + b; }' > test.c
./bin/cc0 test.c sstore.out tokens.out
./bin/cc1 sstore.out tokens.out ast.out sym.out
./bin/cc1t sstore.out ast.out sym.out

# Test unsupported features (will show parser limitations)
echo 'int arr[10]; func(a, b);' > advanced.c
./bin/cc0 advanced.c sstore.out tokens.out
./bin/cc1 sstore.out tokens.out ast.out sym.out  # Will show errors
```

### Understanding Compiler Output

Each tool provides specific information:

- **cc0**: Shows token count and string storage statistics
- **cc1**: Reports parsing errors, AST node count, and symbol statistics  
- **cc2**: Displays TAC generation status and instruction count
- **Viewers (cc0t, cc1t, cc2t)**: Detailed inspection of intermediate representations

## Known Limitations

### Parser Limitations
- **Expression precedence bug**: `a + b + c` incorrectly parsed as `a + (b + c)`
- **No function calls**: `func(args)` syntax not supported
- **No pointers/arrays**: `int *ptr` and `arr[index]` not implemented
- **Limited operators**: Missing `++`, `--`, `&&`, `||`, bitwise operators

### Memory Constraints  
- **Fixed buffer sizes**: AST buffer limited to 64-100 nodes (configurable)
- **No dynamic allocation**: All memory pre-allocated at startup
- **File I/O dependency**: Heavy reliance on disk storage may impact performance

### Incomplete Features
- **No preprocessor**: `#include`, `#define` not supported
- **Basic type system**: No advanced type checking or conversions
- **Simple error recovery**: Parser may fail on complex syntax errors

> **Important**: STCC1 is designed for educational and embedded use cases where memory is extremely constrained. It intentionally trades feature completeness for memory efficiency.

## Contributing

This project follows a specific memory-conscious development approach:

1. **Memory First**: All changes must consider memory impact
2. **Measure Everything**: Use built-in memory tracking tools
3. **Test Early**: Run `make test` frequently to check memory usage
4. **Grammar Awareness**: Understand current parser limitations (see [TODO.md](TODO.md))
5. **Code Quality**: Maintain cpplint compliance and coding standards
6. **Document Decisions**: Explain memory trade-offs in code comments

### Development Priorities

See [TODO.md](TODO.md) for the complete development roadmap. Current high-priority items:

1. **Fix expression parsing bug** (affects all mathematical operations)
2. **Add function call support** (blocking most real C programs)
3. **Implement pointer/array syntax** (essential C features)
4. **Expand operator support** (mathematical and logical operations)

### Code Quality Standards
- Use proper header guard naming (`SRC_PATH_FILENAME_H_`)
- Fixed-width integer types (`uint16_t`, `int64_t`) instead of `short`/`long`
- No trailing whitespace
- 80-character line length limit
- Comprehensive copyright headers

### Testing New Features
When adding grammar features, ensure:
- Parser handles both valid and invalid syntax gracefully
- Memory usage remains within target bounds
- AST generation creates correct node structures
- Symbol table updates appropriately
- TAC generation handles new constructs

See [IMPROVEMENTS.md](IMPROVEMENTS.md) for detailed development guidelines.

## Performance Goals

- Compile 1000-line C files in <100MB total memory
- Single-pass parsing where possible
- O(1) symbol lookup with bounded memory usage
- Streaming processing to minimize memory footprint

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

Thomas Boos (tboos70@gmail.com)