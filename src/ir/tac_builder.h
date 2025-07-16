/**
 * @file tac_builder.h
 * @brief TAC generation from AST nodes
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.1
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#ifndef SRC_IR_TAC_BUILDER_H_
#define SRC_IR_TAC_BUILDER_H_

#include "tac_types.h"
#include "tac_store.h"
#include "../ast/ast_types.h"
#include "../ast/ast_builder.h"

/**
 * @brief TAC builder context for AST translation
 */
typedef struct TACBuilder {
    TACStore* store;         // TAC instruction store
    TACTempManager* temp_mgr; // Temporary management
    uint16_t label_counter;  // Label generation counter
    ASTBuilder* ast_builder; // AST builder reference
    int error_count;         // Error count during translation
    int warning_count;       // Warning count during translation
} TACBuilder;

// TAC builder initialization and cleanup
int tac_builder_init(TACBuilder* builder,
                     const char* tac_filename);
void tac_builder_cleanup(TACBuilder* builder);

// Core translation functions
TACOperand tac_build_from_ast(TACBuilder* builder,
                     ASTNodeIdx_t node);
TACIdx_t tac_emit_instruction(TACBuilder* builder,
                     TACOpcode op,
                             TACOperand result,
                     TACOperand op1,
                     TACOperand op2);

// Operand creation helpers
TACOperand tac_new_temp(TACBuilder* builder,
                     TypeIdx_t type);
TACOperand tac_new_label(TACBuilder* builder);
TACOperand tac_make_variable(uint16_t var_id,
                     uint8_t scope);
TACOperand tac_make_immediate_int(int32_t value);
TACOperand tac_make_label_ref(uint16_t label_id);

// Instruction emission helpers
TACIdx_t tac_emit_label(TACBuilder* builder,
                     uint16_t label_id);
TACIdx_t tac_emit_assign(TACBuilder* builder,
                     TACOperand dest,
                     TACOperand src);
TACIdx_t tac_emit_binary_op(TACBuilder* builder,
                     TACOpcode op,
                           TACOperand result,
                     TACOperand left,
                     TACOperand right);
TACIdx_t tac_emit_unary_op(TACBuilder* builder,
                     TACOpcode op,
                          TACOperand result,
                     TACOperand operand);
TACIdx_t tac_emit_conditional_jump(TACBuilder* builder,
                     TACOperand condition,
                                  uint16_t label_id,
                     int jump_if_false);
TACIdx_t tac_emit_unconditional_jump(TACBuilder* builder,
                     uint16_t label_id);

// Function handling
TACIdx_t tac_emit_function_start(TACBuilder* builder,
                     SymTabIdx_t func_symbol);
TACIdx_t tac_emit_function_end(TACBuilder* builder);
TACIdx_t tac_emit_call(TACBuilder* builder,
                     TACOperand result,
                      TACOperand function,
                     int param_count);
TACIdx_t tac_emit_param(TACBuilder* builder,
                     TACOperand param);
TACIdx_t tac_emit_return(TACBuilder* builder,
                     TACOperand value);

// Utility functions
TACOpcode token_to_tac_opcode(TokenID_t token_id);
const char* tac_opcode_to_string(TACOpcode opcode);
const char* tac_operand_type_to_string(TACOperandType type);

// Debug and validation
void tac_builder_print_stats(TACBuilder* builder);
int tac_validate_operand(TACOperand operand);

#endif  // SRC_IR_TAC_BUILDER_H_
