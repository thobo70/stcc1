# STCC1 Test Framework

This directory contains comprehensive unit and integration tests for the STCC1 compiler using the Unity test framework.

## Directory Structure

```
tests/
├── test_main.c              # Main test runner
├── test_common.h            # Common test utilities and macros
├── test_common.c            # Helper functions implementation
├── unit/                    # Unit tests for individual components
│   ├── test_lexer.c        # Lexer (cc0) tests
│   ├── test_parser.c       # Parser (cc1) tests
│   ├── test_ast.c          # AST storage tests
│   ├── test_storage.c      # Storage components tests
│   ├── test_tac.c          # TAC generation tests
│   └── test_utils.c        # Utility functions tests
├── integration/             # Integration tests
│   └── test_integration.c  # End-to-end compiler pipeline tests
├── fixtures/                # Test input files and expected outputs
└── temp/                    # Temporary files created during testing
```

## Running Tests

### Build and Run All Tests
```bash
make test
```

### Build Test Runner Only
```bash
make test-build
```

### Run Specific Test Suites
```bash
make test-unit           # Unit tests only
make test-integration    # Integration tests only
```

### Clean Test Artifacts
```bash
make test-clean
```

### Show Test Help
```bash
make test-help
```

## Test Framework Features

### Unity Test Framework
- Uses the Unity C testing framework (included in the project)
- Provides assertion macros for comprehensive testing
- Automatic test discovery and execution
- Detailed test result reporting

### Helper Functions
- `create_temp_file()` - Creates temporary test input files
- `run_compiler_stage()` - Executes compiler stages (cc0, cc1, cc2)
- `cleanup_temp_files()` - Cleans up test artifacts
- File existence and content verification macros

### Test Categories

#### Unit Tests
- **Lexer Tests**: Token generation, identifier recognition, literal parsing
- **Parser Tests**: AST construction, operator precedence, statement parsing
- **AST Tests**: Node creation, storage, and retrieval
- **Storage Tests**: String store, symbol table, AST store, TAC store
- **TAC Tests**: Three-address code generation and validation
- **Utils Tests**: Hash functions, hash map buffer operations

#### Integration Tests
- **Complete Pipeline**: End-to-end compilation from C source to TAC
- **Expression Handling**: Complex arithmetic and assignment expressions
- **Control Flow**: If-else statements, loops, function calls
- **Error Handling**: Graceful handling of syntax errors

## Test Input Files

Test files are created dynamically using `create_temp_file()` or stored in the `fixtures/` directory for reusable test cases.

## Temporary Files

Tests create temporary files in `tests/temp/` which are automatically cleaned up. These include:
- `*.out` - Binary storage files (sstore, ast, sym, tac)
- `*.c` - Temporary C source files
- `*.tac` - TAC output files

## Adding New Tests

### Unit Tests
1. Create a new test file in `tests/unit/`
2. Include `../test_common.h`
3. Implement test functions using Unity assertions
4. Add a `run_*_tests()` function
5. Add the test runner to `test_common.h` and `test_main.c`

### Integration Tests
1. Add test functions to `tests/integration/test_integration.c`
2. Use `run_compiler_stage()` helper for pipeline testing
3. Verify output files and content

### Example Test Function
```c
void test_example_functionality(void) {
    // Setup
    char* input_file = create_temp_file("int main() { return 0; }");
    
    // Execute
    char* outputs[] = {"output1.out", "output2.out"};
    int result = run_compiler_stage("cc0", input_file, outputs);
    
    // Verify
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_FILE_EXISTS("output1.out");
}
```

## Test Output

Tests provide detailed output including:
- Pass/fail status for each test
- Error messages for failed assertions
- Summary statistics
- File paths for generated test artifacts

## Dependencies

- Unity test framework (included)
- STCC1 compiler components
- Standard C library
- Make build system
