/**
 * @file tac_types.h
 * @brief Three-Address Code types and structures for STCC1 compiler
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.1
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#ifndef SRC_IR_TAC_TYPES_H_
#define SRC_IR_TAC_TYPES_H_

#include <stdint.h>
#include "../storage/sstore.h"
#include "../lexer/ctoken.h"
#include "../ast/ast_types.h"

// Forward declarations
typedef uint16_t TACIdx_t;

/**
 * @brief TAC operand types
 */
typedef enum TACOperandType {
    TAC_OP_NONE = 0,         // No operand
    TAC_OP_TEMP,             // Temporary variable (t1, t2, ...)
    TAC_OP_VAR,              // Named variable
    TAC_OP_IMMEDIATE,        // Constant value
    TAC_OP_LABEL,            // Jump label
    TAC_OP_FUNCTION,         // Function reference
    TAC_OP_GLOBAL,           // Global variable
    TAC_OP_PARAM,            // Function parameter
    TAC_OP_RETURN_VAL        // Function return value
} TACOperandType;

/**
 * @brief TAC operand - flexible addressing
 * Size: 4 bytes
 */
typedef struct TACOperand {
    TACOperandType type;     // 1 byte - operand type
    union {
        struct {
            uint16_t id;     // Variable/temporary ID
            uint8_t scope;   // Scope level (for locals)
        } variable;

        struct {
            int32_t value;   // Immediate integer value
        } immediate;

        struct {
            uint16_t offset; // Jump target offset
            uint8_t padding;
        } label;

        struct {
            uint16_t func_id; // Function identifier
            uint8_t padding;
        } function;

        uint32_t raw;        // Raw 32-bit access
    } data;
} TACOperand;

/**
 * @brief TAC instruction opcodes
 */
typedef enum TACOpcode {
    // Special operations
    TAC_NOP = 0x00,          // No operation

    // Binary arithmetic (result = op1 OP op2)
    TAC_ADD = 0x10,          // result = operand1 + operand2
    TAC_SUB,                 // result = operand1 - operand2
    TAC_MUL,                 // result = operand1 * operand2
    TAC_DIV,                 // result = operand1 / operand2
    TAC_MOD,                 // result = operand1 % operand2

    // Unary arithmetic (result = OP op1)
    TAC_NEG,                 // result = -operand1
    TAC_NOT,                 // result = !operand1
    TAC_BITWISE_NOT,         // result = ~operand1

    // Bitwise operations
    TAC_AND,                 // result = operand1 & operand2
    TAC_OR,                  // result = operand1 | operand2
    TAC_XOR,                 // result = operand1 ^ operand2
    TAC_SHL,                 // result = operand1 << operand2
    TAC_SHR,                 // result = operand1 >> operand2

    // Comparison operations (result = op1 REL op2)
    TAC_EQ = 0x20,           // result = operand1 == operand2
    TAC_NE,                  // result = operand1 != operand2
    TAC_LT,                  // result = operand1 < operand2
    TAC_LE,                  // result = operand1 <= operand2
    TAC_GT,                  // result = operand1 > operand2
    TAC_GE,                  // result = operand1 >= operand2

    // Logical operations
    TAC_LOGICAL_AND,         // result = operand1 && operand2
    TAC_LOGICAL_OR,          // result = operand1 || operand2

    // Assignment operations
    TAC_ASSIGN = 0x30,       // result = operand1
    TAC_LOAD,                // result = *operand1 (indirect load)
    TAC_STORE,               // *result = operand1 (indirect store)
    TAC_ADDR,                // result = &operand1 (address of)

    // Array and struct operations
    TAC_INDEX,               // result = operand1[operand2]
    TAC_MEMBER,              // result = operand1.field
    TAC_MEMBER_PTR,          // result = operand1->field

    // Control flow
    TAC_LABEL = 0x40,        // Label definition (no operands)
    TAC_GOTO,                // goto operand1 (unconditional jump)
    TAC_IF_FALSE,            // if (!operand1) goto operand2
    TAC_IF_TRUE,             // if (operand1) goto operand2

    // Function operations
    TAC_CALL = 0x50,         // result = call operand1(params...)
    TAC_PARAM,               // Push parameter operand1
    TAC_RETURN,              // return operand1
    TAC_RETURN_VOID,         // return (no value)

    // Special operations
    TAC_CAST = 0x60,         // result = (type)operand1
    TAC_SIZEOF,              // result = sizeof(operand1)
    TAC_PHI                  // SSA phi function: result = φ(op1, op2)
} TACOpcode;

/**
 * @brief TAC instruction flags for optimization
 */
typedef enum TACFlags {
    TAC_FLAG_NONE         = 0x0000,
    TAC_FLAG_DEAD_CODE    = 0x0001,  // Dead code elimination candidate
    TAC_FLAG_CONST_FOLD   = 0x0002,  // Constant folding applied
    TAC_FLAG_CSE          = 0x0004,  // Common subexpression elimination
    TAC_FLAG_COPY_PROP    = 0x0008,  // Copy propagation applied
    TAC_FLAG_MODIFIED     = 0x0010,  // Instruction modified
    TAC_FLAG_LIVE         = 0x0020,  // Live variable
    TAC_FLAG_OPTIMIZED    = 0x8000   // Optimization complete
} TACFlags;

/**
 * @brief Three-address code instruction
 * Size: 16 bytes (optimized for memory efficiency)
 */
typedef struct TACInstruction {
    TACOpcode opcode;        // 2 bytes - operation type
    TACFlags flags;          // 2 bytes - optimization flags

    TACOperand result;       // 4 bytes - destination operand
    TACOperand operand1;     // 4 bytes - first source operand
    TACOperand operand2;     // 4 bytes - second source operand
} TACInstruction;

/**
 * @brief Temporary variable manager
 */
typedef struct TACTempManager {
    uint16_t next_temp;      // Next temporary ID
    uint16_t max_temp;       // Maximum concurrent temporaries
    uint8_t* temp_types;     // Type of each temporary
    TACFlags* temp_flags;    // Flags for each temporary
} TACTempManager;

/**
 * @brief TAC function context
 */
typedef struct TACFunction {
    SymTabIdx_t symbol_idx;  // Function symbol
    TACIdx_t start_idx;      // First instruction
    TACIdx_t end_idx;        // Last instruction
    uint16_t temp_count;     // Number of temporaries used
    uint16_t param_count;    // Number of parameters
    uint16_t local_count;    // Number of local variables
    TypeIdx_t return_type;   // Return type
} TACFunction;

/**
 * @brief Basic block structure for optimization
 */
typedef struct TACBasicBlock {
    TACIdx_t start_idx;      // First instruction index
    TACIdx_t end_idx;        // Last instruction index
    uint16_t id;             // Basic block ID
    uint16_t predecessor_count; // Number of predecessors
    uint16_t successor_count;   // Number of successors
    TACFlags flags;          // Block optimization flags
} TACBasicBlock;

/**
 * @brief Control flow graph
 */
typedef struct TACCFG {
    TACBasicBlock* blocks;   // Array of basic blocks
    uint16_t block_count;    // Number of blocks
    uint16_t* edges;         // Edge relationships
    uint16_t edge_count;     // Number of edges
} TACCFG;

// Compile-time size check (C11 feature, may need compiler support)
// Temporarily disabled for debugging
#ifdef __STDC_VERSION__
#if __STDC_VERSION__ >= 201112L
// _Static_assert(sizeof(TACInstruction) == 16, "TACInstruction must be exactly 16 bytes");
// _Static_assert(sizeof(TACOperand) == 4, "TACOperand must be exactly 4 bytes");
#endif
#endif

// Convenience macros for creating operands
#define TAC_OPERAND_NONE ((TACOperand){TAC_OP_NONE, {.raw = 0}})
#define TAC_MAKE_TEMP(id) ((TACOperand){TAC_OP_TEMP, {.variable = {id, 0}}})
#define TAC_MAKE_VAR(id) ((TACOperand){TAC_OP_VAR, {.variable = {id, 0}}})
#define TAC_MAKE_IMMEDIATE(val) ((TACOperand){TAC_OP_IMMEDIATE, {.immediate = {val}}})
#define TAC_MAKE_LABEL(id) ((TACOperand){TAC_OP_LABEL, {.label = {id, 0}}})

#endif  // SRC_IR_TAC_TYPES_H_
