/**
 * @file tac_engine_internal.h
 * @brief Internal data structures for TAC Engine
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 * @copyright Copyright (c) 2025
 * 
 * This file contains the internal implementation details of the TAC Engine.
 * These structures are not exposed in the public API to maintain clean
 * encapsulation and allow for future optimization without breaking compatibility.
 */

#ifndef SRC_TOOLS_TAC_ENGINE_TAC_ENGINE_INTERNAL_H_
#define SRC_TOOLS_TAC_ENGINE_TAC_ENGINE_INTERNAL_H_

#include "tac_engine.h"
#include <stdio.h>

/**
 * @brief Virtual memory block
 */
typedef struct tac_memory_block {
    uint32_t address;               // Virtual address
    uint32_t size;                  // Block size
    uint8_t* data;                  // Actual data
    bool allocated;                 // Allocation status
    struct tac_memory_block* next;  // Next block in list
} tac_memory_block_t;

/**
 * @brief Virtual memory manager
 */
typedef struct tac_memory_manager {
    tac_memory_block_t* blocks;     // Linked list of memory blocks
    uint32_t next_address;          // Next available address
    uint32_t total_allocated;       // Total allocated bytes
    uint32_t max_size;              // Maximum memory size
} tac_memory_manager_t;

/**
 * @brief Call stack frame
 */
typedef struct tac_stack_frame {
    uint32_t return_address;        // Return instruction address
    uint32_t local_var_base;        // Base for local variables
    uint32_t param_count;           // Number of parameters
    tac_value_t* locals;            // Local variable storage
    struct tac_stack_frame* prev;   // Previous frame
} tac_stack_frame_t;

/**
 * @brief Execution hook entry
 */
typedef struct tac_hook_entry {
    uint32_t id;                    // Unique hook ID
    tac_hook_type_t type;           // Hook type
    tac_hook_callback_t callback;   // Callback function
    void* user_data;                // User data
    bool enabled;                   // Hook enabled flag
    struct tac_hook_entry* next;    // Next hook in list
} tac_hook_entry_t;

/**
 * @brief Breakpoint entry
 */
typedef struct tac_breakpoint {
    uint32_t address;               // Instruction address
    bool enabled;                   // Breakpoint enabled
    struct tac_breakpoint* next;    // Next breakpoint
} tac_breakpoint_t;

/**
 * @brief Execution trace entry
 */
typedef struct tac_trace_entry {
    uint32_t step;                  // Execution step number
    uint32_t address;               // Instruction address
    TACInstruction instruction;     // Executed instruction
    tac_value_t result_before;      // Result operand before execution
    tac_value_t result_after;       // Result operand after execution
} tac_trace_entry_t;

/**
 * @brief Execution trace buffer
 */
typedef struct tac_trace_buffer {
    tac_trace_entry_t* entries;     // Trace entries
    uint32_t capacity;              // Buffer capacity
    uint32_t count;                 // Number of entries
    uint32_t head;                  // Circular buffer head
    bool enabled;                   // Tracing enabled
} tac_trace_buffer_t;

/**
 * @brief Label table entry for jump resolution
 */
typedef struct tac_label_entry {
    uint16_t label_id;              // Label ID
    uint32_t address;               // Instruction address where label is defined
    struct tac_label_entry* next;   // Next entry in hash table
} tac_label_entry_t;

/**
 * @brief Label resolution table
 */
typedef struct tac_label_table {
    tac_label_entry_t* entries[256]; // Hash table (label_id % 256)
    uint32_t count;                 // Number of labels
} tac_label_table_t;

/**
 * @brief Main engine structure
 */
struct tac_engine {
    // Configuration
    tac_engine_config_t config;

    // Execution state
    tac_engine_state_t state;
    tac_engine_error_t last_error;
    uint32_t pc;                    // Program counter
    uint32_t step_count;            // Executed steps
    bool running;                   // Execution flag

    // Code storage
    TACInstruction* instructions;   // Loaded instructions
    uint32_t instruction_count;     // Number of instructions
    tac_label_table_t label_table;  // Label resolution table

    // Variable storage
    tac_value_t* temporaries;       // Temporary variables
    tac_value_t* variables;         // Named variables
    uint32_t temp_count;            // Number of temporaries
    uint32_t var_count;             // Number of variables

    // Memory management
    tac_memory_manager_t memory;

    // Call stack
    tac_stack_frame_t* call_stack;
    uint32_t call_depth;
    uint32_t last_call_instruction;  // Address of last CALL instruction for return value handling
    uint32_t param_counter;          // Counter for parameter passing
    tac_value_t param_stack[10];     // Temporary parameter storage for function calls

    // Debugging support
    tac_hook_entry_t* hooks;
    uint32_t next_hook_id;
    tac_breakpoint_t* breakpoints;
    tac_trace_buffer_t trace;

    // Error context
    char error_message[256];        // Detailed error message
    uint32_t error_address;         // Address where error occurred
};

// =============================================================================
// INTERNAL FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Initialize memory manager
 * @param memory Memory manager to initialize
 * @param max_size Maximum memory size
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_memory_init(tac_memory_manager_t* memory, uint32_t max_size);

/**
 * @brief Initialize label table
 * @param table Label table to initialize
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_label_table_init(tac_label_table_t* table);

/**
 * @brief Build label table from loaded instructions
 * @param engine TAC engine
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_build_label_table(tac_engine_t* engine);

/**
 * @brief Resolve label ID to instruction address
 * @param table Label table
 * @param label_id Label ID to resolve
 * @param address Output address
 * @return TAC_ENGINE_OK if found, TAC_ENGINE_ERR_INVALID_OPERAND if not found
 */
tac_engine_error_t tac_resolve_label(const tac_label_table_t* table, uint16_t label_id, uint32_t* address);

/**
 * @brief Cleanup label table
 * @param table Label table to cleanup
 */
void tac_label_table_cleanup(tac_label_table_t* table);

/**
 * @brief Cleanup memory manager
 * @param memory Memory manager to cleanup
 */
void tac_memory_cleanup(tac_memory_manager_t* memory);

/**
 * @brief Allocate memory block
 * @param memory Memory manager
 * @param size Size to allocate
 * @return Virtual address or 0 on failure
 */
uint32_t tac_memory_alloc(tac_memory_manager_t* memory, uint32_t size);

/**
 * @brief Free memory block
 * @param memory Memory manager
 * @param address Virtual address
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_memory_free(tac_memory_manager_t* memory, uint32_t address);

/**
 * @brief Read from memory
 * @param memory Memory manager
 * @param address Virtual address
 * @param buffer Output buffer
 * @param size Number of bytes
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_memory_read(tac_memory_manager_t* memory,
                                   uint32_t address,
                                   void* buffer,
                                   uint32_t size);

/**
 * @brief Write to memory
 * @param memory Memory manager
 * @param address Virtual address
 * @param buffer Input buffer
 * @param size Number of bytes
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_memory_write(tac_memory_manager_t* memory,
                                    uint32_t address,
                                    const void* buffer,
                                    uint32_t size);

/**
 * @brief Execute single TAC instruction
 * @param engine Engine instance
 * @param instruction Instruction to execute
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_execute_instruction(tac_engine_t* engine,
                                           const TACInstruction* instruction);

/**
 * @brief Evaluate TAC operand to value
 * @param engine Engine instance
 * @param operand Operand to evaluate
 * @param value Output value
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_eval_operand(tac_engine_t* engine,
                                    const TACOperand* operand,
                                    tac_value_t* value);

/**
 * @brief Store value to TAC operand
 * @param engine Engine instance
 * @param operand Target operand
 * @param value Value to store
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_store_operand(tac_engine_t* engine,
                                     const TACOperand* operand,
                                     const tac_value_t* value);

/**
 * @brief Push call stack frame
 * @param engine Engine instance
 * @param return_address Return address
 * @param param_count Number of parameters
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_push_frame(tac_engine_t* engine,
                                  uint32_t return_address,
                                  uint32_t param_count);

/**
 * @brief Pop call stack frame
 * @param engine Engine instance
 * @return Return address or 0 on error
 */
uint32_t tac_pop_frame(tac_engine_t* engine);

/**
 * @brief Check if breakpoint is set at address
 * @param engine Engine instance
 * @param address Instruction address
 * @return true if breakpoint exists
 */
bool tac_has_breakpoint(tac_engine_t* engine, uint32_t address);

/**
 * @brief Trigger execution hooks
 * @param engine Engine instance
 * @param hook_type Hook type
 * @param address Current address
 * @return true to continue execution
 */
bool tac_trigger_hooks(tac_engine_t* engine,
                       tac_hook_type_t hook_type,
                       uint32_t address);

/**
 * @brief Add trace entry
 * @param engine Engine instance
 * @param instruction Executed instruction
 * @param result_before Result before execution
 * @param result_after Result after execution
 */
void tac_add_trace(tac_engine_t* engine,
                   const TACInstruction* instruction,
                   const tac_value_t* result_before,
                   const tac_value_t* result_after);

/**
 * @brief Set engine error state
 * @param engine Engine instance
 * @param error Error code
 * @param message Error message format
 * @param ... Format arguments
 */
void tac_set_error(tac_engine_t* engine, 
                   tac_engine_error_t error,
                   const char* message, ...);

/**
 * @brief Validate operand
 * @param engine Engine instance
 * @param operand Operand to validate
 * @return TAC_ENGINE_OK if valid
 */
tac_engine_error_t tac_validate_operand(tac_engine_t* engine,
                                        const TACOperand* operand);

/**
 * @brief Convert value between types
 * @param from Source value
 * @param to_type Target type
 * @param to Output value
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_convert_value(const tac_value_t* from,
                                     tac_value_type_t to_type,
                                     tac_value_t* to);

/**
 * @brief Execute assignment instruction
 * @param engine Engine instance
 * @param instruction Instruction to execute
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_execute_assign(tac_engine_t* engine,
                                      const TACInstruction* instruction);

/**
 * @brief Execute binary operation instruction
 * @param engine Engine instance
 * @param instruction Instruction to execute
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_execute_binary_op(tac_engine_t* engine,
                                         const TACInstruction* instruction);

/**
 * @brief Execute jump instruction
 * @param engine Engine instance
 * @param instruction Instruction to execute
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_execute_jump(tac_engine_t* engine,
                                    const TACInstruction* instruction);

/**
 * @brief Execute conditional jump instruction
 * @param engine Engine instance
 * @param instruction Instruction to execute
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_execute_conditional_jump(tac_engine_t* engine,
                                                const TACInstruction* instruction);

/**
 * @brief Execute call instruction
 * @param engine Engine instance
 * @param instruction Instruction to execute
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_execute_call(tac_engine_t* engine,
                                    const TACInstruction* instruction);

/**
 * @brief Execute return instruction
 * @param engine Engine instance
 * @param instruction Instruction to execute
 * @return TAC_ENGINE_OK on success
 */
tac_engine_error_t tac_execute_return(tac_engine_t* engine,
                                      const TACInstruction* instruction);

#endif  // SRC_TOOLS_TAC_ENGINE_TAC_ENGINE_INTERNAL_H_
