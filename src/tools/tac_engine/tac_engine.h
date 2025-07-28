/**
 * @file tac_engine.h
 * @brief TAC Engine - A Unicorn-like emulator for executing Three-Address Code
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 * @copyright Copyright (c) 2025
 * 
 * TAC ENGINE DESIGN PHILOSOPHY:
 * 
 * The TAC Engine is inspired by the Unicorn Engine architecture but specifically
 * designed for emulating Three-Address Code execution. It provides:
 * 
 * 1. EASY TESTING API:
 *    - Simple setup and teardown
 *    - Step-by-step execution control
 *    - Register and memory inspection
 *    - Breakpoint and hook support
 * 
 * 2. VIRTUAL MACHINE MODEL:
 *    - Virtual registers for temporaries and variables
 *    - Virtual memory for arrays and structures
 *    - Call stack for function execution
 *    - Symbol table integration
 * 
 * 3. DEBUGGING FEATURES:
 *    - Instruction tracing
 *    - Register/memory dumps
 *    - Execution hooks and callbacks
 *    - Error injection for robustness testing
 * 
 * 4. TEST FRAMEWORK INTEGRATION:
 *    - Unity test framework compatible
 *    - Assertion helpers for common checks
 *    - Test fixture support
 *    - Isolated execution contexts
 * 
 * USAGE EXAMPLE:
 * ```c
 * // Create engine instance
 * tac_engine_t* engine = tac_engine_create();
 * 
 * // Load TAC instructions
 * TACInstruction code[] = {
 *     {TAC_ASSIGN, 0, TAC_MAKE_TEMP(1), TAC_MAKE_IMMEDIATE(42), TAC_OPERAND_NONE},
 *     {TAC_ADD, 0, TAC_MAKE_TEMP(2), TAC_MAKE_TEMP(1), TAC_MAKE_IMMEDIATE(8)}
 * };
 * tac_engine_load_code(engine, code, 2);
 * 
 * // Execute and verify
 * tac_engine_run(engine);
 * assert(tac_engine_get_temp(engine, 2) == 50);
 * 
 * // Cleanup
 * tac_engine_destroy(engine);
 * ```
 */

#ifndef SRC_TOOLS_TAC_ENGINE_TAC_ENGINE_H_
#define SRC_TOOLS_TAC_ENGINE_TAC_ENGINE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../ir/tac_types.h"

// Forward declarations
typedef struct tac_engine tac_engine_t;
typedef struct tac_execution_context tac_execution_context_t;

/**
 * @brief Engine execution states
 */
typedef enum tac_engine_state {
    TAC_ENGINE_STOPPED = 0,    // Engine not running
    TAC_ENGINE_RUNNING,        // Currently executing
    TAC_ENGINE_PAUSED,         // Paused at breakpoint
    TAC_ENGINE_FINISHED,       // Execution completed normally
    TAC_ENGINE_ERROR           // Execution stopped due to error
} tac_engine_state_t;

/**
 * @brief Engine error codes
 */
typedef enum tac_engine_error {
    TAC_ENGINE_OK = 0,               // No error
    TAC_ENGINE_ERR_NULL_POINTER,     // NULL pointer passed
    TAC_ENGINE_ERR_INVALID_OPCODE,   // Unknown TAC opcode
    TAC_ENGINE_ERR_INVALID_OPERAND,  // Invalid operand type/value
    TAC_ENGINE_ERR_OUT_OF_MEMORY,    // Memory allocation failed
    TAC_ENGINE_ERR_STACK_OVERFLOW,   // Call stack overflow
    TAC_ENGINE_ERR_STACK_UNDERFLOW,  // Stack underflow
    TAC_ENGINE_ERR_DIVISION_BY_ZERO, // Division by zero
    TAC_ENGINE_ERR_INVALID_MEMORY,   // Invalid memory access
    TAC_ENGINE_ERR_BREAKPOINT,       // Hit breakpoint
    TAC_ENGINE_ERR_MAX_STEPS         // Maximum steps exceeded
} tac_engine_error_t;

/**
 * @brief Engine configuration options
 */
typedef struct tac_engine_config {
    uint32_t max_temporaries;     // Maximum temporary variables (default: 1024)
    uint32_t max_variables;       // Maximum named variables (default: 1024)
    uint32_t max_memory_size;     // Virtual memory size in bytes (default: 64KB)
    uint32_t max_call_depth;      // Maximum function call depth (default: 256)
    uint32_t max_steps;           // Maximum execution steps (default: 1M)
    bool enable_tracing;          // Enable instruction tracing
    bool enable_bounds_check;     // Enable array bounds checking
    bool enable_type_check;       // Enable type checking
} tac_engine_config_t;

/**
 * @brief Value types for engine storage
 */
typedef enum tac_value_type {
    TAC_VALUE_INT32 = 0,         // 32-bit signed integer
    TAC_VALUE_UINT32,            // 32-bit unsigned integer
    TAC_VALUE_INT64,             // 64-bit signed integer
    TAC_VALUE_UINT64,            // 64-bit unsigned integer
    TAC_VALUE_FLOAT,             // 32-bit float
    TAC_VALUE_DOUBLE,            // 64-bit double
    TAC_VALUE_POINTER,           // Memory pointer
    TAC_VALUE_BOOL               // Boolean value
} tac_value_type_t;

/**
 * @brief Generic value container
 */
typedef struct tac_value {
    tac_value_type_t type;
    union {
        int32_t i32;
        uint32_t u32;
        int64_t i64;
        uint64_t u64;
        float f32;
        double f64;
        void* ptr;
        bool boolean;
    } data;
} tac_value_t;

/**
 * @brief Execution hook types
 */
typedef enum tac_hook_type {
    TAC_HOOK_INSTRUCTION,        // Before each instruction
    TAC_HOOK_MEMORY_READ,        // Before memory read
    TAC_HOOK_MEMORY_WRITE,       // Before memory write
    TAC_HOOK_FUNCTION_CALL,      // Function call
    TAC_HOOK_FUNCTION_RETURN,    // Function return
    TAC_HOOK_ERROR               // Error occurred
} tac_hook_type_t;

/**
 * @brief Hook callback function type
 * @param engine Pointer to engine instance
 * @param hook_type Type of hook triggered
 * @param address Current instruction address (for instruction hooks)
 * @param user_data User-provided data pointer
 * @return true to continue execution, false to stop
 */
typedef bool (*tac_hook_callback_t)(tac_engine_t* engine, 
                                    tac_hook_type_t hook_type,
                                    uint32_t address,
                                    void* user_data);

// =============================================================================
// CORE ENGINE API
// =============================================================================

/**
 * @brief Create a new TAC engine instance
 * @param config Engine configuration (NULL for defaults)
 * @return New engine instance or NULL on failure
 */
tac_engine_t* tac_engine_create(const tac_engine_config_t* config);

/**
 * @brief Destroy a TAC engine instance
 * @param engine Engine to destroy
 */
void tac_engine_destroy(tac_engine_t* engine);

/**
 * @brief Reset engine to initial state
 * @param engine Engine instance
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_reset(tac_engine_t* engine);

/**
 * @brief Get default engine configuration
 * @return Default configuration structure
 */
tac_engine_config_t tac_engine_default_config(void);

// =============================================================================
// CODE LOADING AND EXECUTION
// =============================================================================

/**
 * @brief Load TAC instructions into engine
 * @param engine Engine instance
 * @param instructions Array of TAC instructions
 * @param count Number of instructions
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_load_code(tac_engine_t* engine,
                                        const TACInstruction* instructions,
                                        uint32_t count);

/**
 * @brief Execute all loaded instructions
 * @param engine Engine instance
 * @return TAC_ENGINE_OK on successful completion
 */
tac_engine_error_t tac_engine_run(tac_engine_t* engine);

/**
 * @brief Execute a single instruction
 * @param engine Engine instance
 * @return TAC_ENGINE_OK on success, TAC_ENGINE_FINISHED when done
 */
tac_engine_error_t tac_engine_step(tac_engine_t* engine);

/**
 * @brief Execute until reaching specified address
 * @param engine Engine instance
 * @param address Target instruction address
 * @return TAC_ENGINE_OK when target reached
 */
tac_engine_error_t tac_engine_run_until(tac_engine_t* engine, uint32_t address);

/**
 * @brief Stop execution
 * @param engine Engine instance
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_stop(tac_engine_t* engine);

// =============================================================================
// STATE INSPECTION
// =============================================================================

/**
 * @brief Get current engine state
 * @param engine Engine instance
 * @return Current execution state
 */
tac_engine_state_t tac_engine_get_state(tac_engine_t* engine);

/**
 * @brief Get current program counter
 * @param engine Engine instance
 * @return Current instruction address
 */
uint32_t tac_engine_get_pc(tac_engine_t* engine);

/**
 * @brief Get last error code
 * @param engine Engine instance
 * @return Last error that occurred
 */
tac_engine_error_t tac_engine_get_last_error(tac_engine_t* engine);

/**
 * @brief Get number of executed steps
 * @param engine Engine instance
 * @return Number of steps executed
 */
uint32_t tac_engine_get_step_count(tac_engine_t* engine);

/**
 * @brief Get human-readable error string
 * @param error Error code
 * @return Error description string
 */
const char* tac_engine_error_string(tac_engine_error_t error);

// =============================================================================
// VARIABLE AND TEMPORARY ACCESS
// =============================================================================

/**
 * @brief Get temporary variable value
 * @param engine Engine instance
 * @param temp_id Temporary variable ID
 * @param value Output value
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_get_temp(tac_engine_t* engine, 
                                       uint16_t temp_id, 
                                       tac_value_t* value);

/**
 * @brief Set temporary variable value
 * @param engine Engine instance
 * @param temp_id Temporary variable ID
 * @param value Value to set
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_set_temp(tac_engine_t* engine,
                                       uint16_t temp_id,
                                       const tac_value_t* value);

/**
 * @brief Get named variable value
 * @param engine Engine instance
 * @param var_id Variable ID
 * @param value Output value
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_get_var(tac_engine_t* engine,
                                      uint16_t var_id,
                                      tac_value_t* value);

/**
 * @brief Set named variable value
 * @param engine Engine instance
 * @param var_id Variable ID
 * @param value Value to set
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_set_var(tac_engine_t* engine,
                                      uint16_t var_id,
                                      const tac_value_t* value);

// =============================================================================
// MEMORY MANAGEMENT
// =============================================================================

/**
 * @brief Allocate virtual memory
 * @param engine Engine instance
 * @param size Size in bytes
 * @return Virtual address or 0 on failure
 */
uint32_t tac_engine_malloc(tac_engine_t* engine, uint32_t size);

/**
 * @brief Free virtual memory
 * @param engine Engine instance
 * @param address Virtual address to free
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_free(tac_engine_t* engine, uint32_t address);

/**
 * @brief Read from virtual memory
 * @param engine Engine instance
 * @param address Virtual address
 * @param buffer Output buffer
 * @param size Number of bytes to read
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_mem_read(tac_engine_t* engine,
                                       uint32_t address,
                                       void* buffer,
                                       uint32_t size);

/**
 * @brief Write to virtual memory
 * @param engine Engine instance
 * @param address Virtual address
 * @param buffer Input buffer
 * @param size Number of bytes to write
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_mem_write(tac_engine_t* engine,
                                        uint32_t address,
                                        const void* buffer,
                                        uint32_t size);

// =============================================================================
// DEBUGGING AND HOOKS
// =============================================================================

/**
 * @brief Set breakpoint at instruction address
 * @param engine Engine instance
 * @param address Instruction address
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_add_breakpoint(tac_engine_t* engine, 
                                             uint32_t address);

/**
 * @brief Remove breakpoint
 * @param engine Engine instance
 * @param address Instruction address
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_remove_breakpoint(tac_engine_t* engine,
                                                uint32_t address);

/**
 * @brief Add execution hook
 * @param engine Engine instance
 * @param hook_type Type of hook
 * @param callback Callback function
 * @param user_data User data for callback
 * @return Hook ID or 0 on failure
 */
uint32_t tac_engine_add_hook(tac_engine_t* engine,
                             tac_hook_type_t hook_type,
                             tac_hook_callback_t callback,
                             void* user_data);

/**
 * @brief Remove execution hook
 * @param engine Engine instance
 * @param hook_id Hook ID returned by tac_engine_add_hook
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_remove_hook(tac_engine_t* engine, uint32_t hook_id);

/**
 * @brief Enable/disable instruction tracing
 * @param engine Engine instance
 * @param enable true to enable tracing
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_set_tracing(tac_engine_t* engine, bool enable);

// =============================================================================
// TEST FRAMEWORK HELPERS
// =============================================================================

/**
 * @brief Create integer value
 * @param value Integer value
 * @return tac_value_t structure
 */
static inline tac_value_t tac_value_int32(int32_t value) {
    return (tac_value_t){TAC_VALUE_INT32, {.i32 = value}};
}

/**
 * @brief Create boolean value
 * @param value Boolean value
 * @return tac_value_t structure
 */
static inline tac_value_t tac_value_bool(bool value) {
    return (tac_value_t){TAC_VALUE_BOOL, {.boolean = value}};
}

/**
 * @brief Create pointer value
 * @param ptr Pointer value
 * @return tac_value_t structure
 */
static inline tac_value_t tac_value_ptr(void* ptr) {
    return (tac_value_t){TAC_VALUE_POINTER, {.ptr = ptr}};
}

/**
 * @brief Check if engine execution completed successfully
 * @param engine Engine instance
 * @return true if finished without errors
 */
bool tac_engine_is_finished(tac_engine_t* engine);

/**
 * @brief Get execution statistics
 * @param engine Engine instance
 * @param steps_executed Output: number of steps executed
 * @param memory_used Output: bytes of memory used
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_engine_get_stats(tac_engine_t* engine,
                                        uint32_t* steps_executed,
                                        uint32_t* memory_used);

// =============================================================================
// CONVENIENCE MACROS FOR TESTING
// =============================================================================

/**
 * @brief Assert that engine executed successfully
 */
#define TAC_ENGINE_ASSERT_SUCCESS(engine) \
    do { \
        tac_engine_state_t state = tac_engine_get_state(engine); \
        if (state != TAC_ENGINE_FINISHED) { \
            tac_engine_error_t err = tac_engine_get_last_error(engine); \
            fprintf(stderr, "TAC Engine failed: %s\n", tac_engine_error_string(err)); \
        } \
        assert(state == TAC_ENGINE_FINISHED); \
    } while(0)

/**
 * @brief Assert temporary variable has expected value
 */
#define TAC_ENGINE_ASSERT_TEMP_EQ(engine, temp_id, expected) \
    do { \
        tac_value_t val; \
        assert(tac_engine_get_temp(engine, temp_id, &val) == TAC_ENGINE_OK); \
        assert(val.data.i32 == (expected)); \
    } while(0)

/**
 * @brief Assert variable has expected value
 */
#define TAC_ENGINE_ASSERT_VAR_EQ(engine, var_id, expected) \
    do { \
        tac_value_t val; \
        assert(tac_engine_get_var(engine, var_id, &val) == TAC_ENGINE_OK); \
        assert(val.data.i32 == (expected)); \
    } while(0)

#endif  // SRC_TOOLS_TAC_ENGINE_TAC_ENGINE_H_
