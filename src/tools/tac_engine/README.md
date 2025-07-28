# TAC Engine Documentation

## Overview

The TAC Engine is a virtual machine for executing Three-Address Code (TAC) instructions. It provides a complete testing environment for TAC intermediate representation with debugging support, inspired by the Unicorn Engine architecture.

## Features

- **Complete TAC Execution**: Execute all TAC instruction types (arithmetic, assignment, control flow)
- **Virtual Memory Management**: Simulated memory allocation and access
- **Debugging Support**: Breakpoints, single-stepping, execution hooks
- **Execution Tracing**: Track instruction execution with before/after state
- **Call Stack Management**: Function call/return support
- **Variable Management**: Both temporary and named variables
- **Unity Test Integration**: Macros for easy test framework integration

## Architecture

The engine consists of several key components:

### Core Engine (`tac_engine.c`)
- Engine lifecycle management (create, load, execute, destroy)
- Instruction execution loop
- Program counter and state management
- Error handling and reporting

### Memory Manager (`tac_engine_memory.c`)
- Virtual memory allocation and deallocation
- Memory read/write operations
- Operand evaluation and storage
- Call stack frame management

### Debugging System (`tac_engine_debug.c`)
- Breakpoint management
- Execution hooks
- Variable inspection
- Trace buffer management
- Disassembly support

## API Reference

### Engine Lifecycle

```c
// Create engine with configuration
tac_engine_t* tac_engine_open(const tac_engine_config_t* config);

// Load TAC instructions
tac_engine_error_t tac_engine_load_code(tac_engine_t* engine,
                                        const TACInstruction* instructions,
                                        uint32_t count);

// Start execution at address
tac_engine_error_t tac_engine_start(tac_engine_t* engine, uint32_t start_address);

// Execute single instruction
tac_engine_error_t tac_engine_step(tac_engine_t* engine);

// Run until breakpoint or completion
tac_engine_error_t tac_engine_run(tac_engine_t* engine, uint32_t max_steps);

// Cleanup engine
void tac_engine_close(tac_engine_t* engine);
```

### State Management

```c
// Query engine state
tac_engine_state_t tac_engine_get_state(tac_engine_t* engine);
tac_engine_error_t tac_engine_get_last_error(tac_engine_t* engine);
const char* tac_engine_get_error_message(tac_engine_t* engine);

// Control execution
tac_engine_error_t tac_engine_pause(tac_engine_t* engine);
tac_engine_error_t tac_engine_resume(tac_engine_t* engine);
tac_engine_error_t tac_engine_stop(tac_engine_t* engine);
tac_engine_error_t tac_engine_reset(tac_engine_t* engine);
```

### Variable Access

```c
// Access temporary variables
tac_engine_error_t tac_engine_get_temporary(tac_engine_t* engine,
                                            uint32_t temp_id,
                                            tac_value_t* value);
tac_engine_error_t tac_engine_set_temporary(tac_engine_t* engine,
                                            uint32_t temp_id,
                                            const tac_value_t* value);

// Access named variables
tac_engine_error_t tac_engine_get_variable(tac_engine_t* engine,
                                           uint32_t var_id,
                                           tac_value_t* value);
tac_engine_error_t tac_engine_set_variable(tac_engine_t* engine,
                                           uint32_t var_id,
                                           const tac_value_t* value);
```

### Memory Management

```c
// Virtual memory operations
uint32_t tac_engine_alloc_memory(tac_engine_t* engine, uint32_t size);
tac_engine_error_t tac_engine_free_memory(tac_engine_t* engine, uint32_t address);
tac_engine_error_t tac_engine_read_memory(tac_engine_t* engine,
                                          uint32_t address,
                                          void* buffer,
                                          uint32_t size);
tac_engine_error_t tac_engine_write_memory(tac_engine_t* engine,
                                           uint32_t address,
                                           const void* buffer,
                                           uint32_t size);
```

### Debugging Features

```c
// Breakpoint management
tac_engine_error_t tac_engine_add_breakpoint(tac_engine_t* engine, uint32_t address);
tac_engine_error_t tac_engine_remove_breakpoint(tac_engine_t* engine, uint32_t address);
tac_engine_error_t tac_engine_enable_breakpoint(tac_engine_t* engine,
                                                uint32_t address,
                                                bool enabled);

// Execution hooks
uint32_t tac_engine_add_hook(tac_engine_t* engine,
                             tac_hook_type_t hook_type,
                             tac_hook_callback_t callback,
                             void* user_data);
tac_engine_error_t tac_engine_remove_hook(tac_engine_t* engine, uint32_t hook_id);

// Execution tracing
tac_engine_error_t tac_engine_enable_tracing(tac_engine_t* engine, bool enabled);
uint32_t tac_engine_get_trace_count(tac_engine_t* engine);
tac_engine_error_t tac_engine_get_trace_entry(tac_engine_t* engine,
                                              uint32_t index,
                                              tac_trace_entry_t* entry);
```

## Usage Examples

### Basic Arithmetic Test

```c
#include "tac_engine.h"

void test_arithmetic() {
    // Configure engine
    tac_engine_config_t config = TAC_ENGINE_DEFAULT_CONFIG;
    config.max_temporaries = 10;
    
    // Create engine
    tac_engine_t* engine = tac_engine_open(&config);
    
    // Create TAC program: t2 = 5 + 3
    TACInstruction instructions[] = {
        // t0 = 5
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand1 = {
                .type = TAC_OPERAND_CONSTANT,
                .value = {.type = INT_TYPE, .value = {.int_val = 5}}
            }
        },
        // t1 = 3
        {
            .op = TAC_OP_ASSIGN,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 1},
            .operand1 = {
                .type = TAC_OPERAND_CONSTANT,
                .value = {.type = INT_TYPE, .value = {.int_val = 3}}
            }
        },
        // t2 = t0 + t1
        {
            .op = TAC_OP_ADD,
            .result = {.type = TAC_OPERAND_TEMPORARY, .id = 2},
            .operand1 = {.type = TAC_OPERAND_TEMPORARY, .id = 0},
            .operand2 = {.type = TAC_OPERAND_TEMPORARY, .id = 1}
        }
    };
    
    // Load and execute
    tac_engine_load_code(engine, instructions, 3);
    tac_engine_start(engine, 0);
    tac_engine_run(engine, 10);
    
    // Check result
    tac_value_t result;
    tac_engine_get_temporary(engine, 2, &result);
    assert(result.value.int_val == 8);
    
    tac_engine_close(engine);
}
```

### Unity Test Integration

```c
#include "unity.h"
#include "tac_engine.h"

void test_using_macros(void) {
    TAC_ENGINE_TEST_SETUP();
    
    TACInstruction instructions[] = {
        TAC_ASSIGN_CONST_INT(0, 10),
        TAC_ASSIGN_CONST_INT(1, 5),
        TAC_SUB_TEMP_TEMP(2, 0, 1)
    };
    
    TAC_ENGINE_LOAD_CODE(instructions, 3);
    TAC_ENGINE_RUN_TO_COMPLETION();
    
    tac_value_t result;
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, tac_engine_get_temporary(engine, 2, &result));
    TEST_ASSERT_EQUAL_INT32(5, result.value.int_val);
    
    TAC_ENGINE_TEST_TEARDOWN();
}
```

### Debugging with Hooks

```c
static bool debug_callback(tac_engine_t* engine,
                          tac_hook_type_t hook_type,
                          uint32_t address,
                          void* user_data) {
    if (hook_type == TAC_HOOK_INSTRUCTION) {
        printf("Executing instruction at address %u\n", address);
    }
    return true; // Continue execution
}

void test_with_debugging() {
    tac_engine_t* engine = tac_engine_open(&TAC_ENGINE_DEFAULT_CONFIG);
    
    // Add instruction hook
    tac_engine_add_hook(engine, TAC_HOOK_INSTRUCTION, debug_callback, NULL);
    
    // Add breakpoint
    tac_engine_add_breakpoint(engine, 2);
    
    // Load and run code
    // ... execution will pause at breakpoint and print each instruction
    
    tac_engine_close(engine);
}
```

## Building

Use the provided Makefile:

```bash
# Build library and tests
make all

# Run tests
make test

# Clean build artifacts
make clean

# Install library
make install
```

## Configuration Options

The engine can be configured through `tac_engine_config_t`:

```c
typedef struct {
    uint32_t max_temporaries;      // Maximum temporary variables (default: 1000)
    uint32_t max_variables;        // Maximum named variables (default: 1000)
    uint32_t max_memory_size;      // Virtual memory size (default: 1MB)
    uint32_t max_call_depth;       // Maximum call stack depth (default: 100)
    uint32_t max_steps;            // Step limit (0 = unlimited)
    bool enable_tracing;           // Enable execution tracing
    uint32_t max_trace_entries;    // Trace buffer size (default: 1000)
} tac_engine_config_t;
```

## Error Handling

All functions return error codes. Use `tac_engine_get_last_error()` and `tac_engine_get_error_message()` for detailed error information.

Common error codes:
- `TAC_ENGINE_OK`: Success
- `TAC_ENGINE_ERROR_INVALID_PARAM`: Invalid parameter
- `TAC_ENGINE_ERROR_OUT_OF_MEMORY`: Memory allocation failed
- `TAC_ENGINE_ERROR_INVALID_ADDRESS`: Invalid instruction address
- `TAC_ENGINE_ERROR_DIVISION_BY_ZERO`: Division by zero
- `TAC_ENGINE_ERROR_BREAKPOINT_HIT`: Breakpoint encountered

## Integration with Compiler

The TAC Engine integrates with the STCC1 compiler's TAC generation:

1. **TAC Types**: Uses existing `TACInstruction` and `TACOperand` structures
2. **Error Integration**: Compatible with compiler error handling
3. **Test Framework**: Designed for Unity test integration
4. **Memory Model**: Matches compiler's memory management patterns

## Limitations

Current implementation limitations:

1. **File Loading**: TAC file parsing not yet implemented
2. **Complex Instructions**: Some advanced TAC operations not yet supported
3. **Function Parameters**: Basic call/return mechanism only
4. **Memory Model**: Simplified virtual memory implementation
5. **Optimization**: No instruction optimization or JIT compilation

## Future Enhancements

Planned improvements:

1. **TAC File Parser**: Load TAC code from text files
2. **Advanced Debugging**: Watchpoints, conditional breakpoints
3. **Performance Profiling**: Instruction timing and hotspot analysis
4. **Memory Protection**: Virtual memory protection and segmentation
5. **JIT Compilation**: Just-in-time compilation for performance
6. **Remote Debugging**: Network debugging protocol
7. **GUI Integration**: Graphical debugger interface

## Contributing

When extending the TAC Engine:

1. **Maintain API Compatibility**: Don't break existing interfaces
2. **Add Tests**: Include test cases for new features
3. **Document Changes**: Update this documentation
4. **Follow Style**: Maintain consistent coding style
5. **Error Handling**: Provide detailed error messages

## License

Part of the STCC1 project. See main project LICENSE file for details.
