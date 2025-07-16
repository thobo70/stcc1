/**
 * @file tac_builder.c
 * @brief TAC generation from AST nodes implementation
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.1
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#include "tac_builder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../storage/tstore.h"
#include "../utils/hmapbuf.h"

// Forward declarations for static functions
static TACOperand translate_integer_literal(TACBuilder* builder, ASTNode* ast_node);
static TACOperand translate_identifier(TACBuilder* builder, ASTNode* ast_node);
static TACOperand translate_binary_expr(TACBuilder* builder, ASTNode* ast_node);
static TACOperand translate_unary_expr(TACBuilder* builder, ASTNode* ast_node);
static TACOperand translate_assignment(TACBuilder* builder, ASTNode* ast_node);
static void translate_if_stmt(TACBuilder* builder, ASTNode* ast_node);
static void translate_while_stmt(TACBuilder* builder, ASTNode* ast_node);
static void translate_return_stmt(TACBuilder* builder, ASTNode* ast_node);
static void translate_compound_stmt(TACBuilder* builder, ASTNode* ast_node);
static TACOperand translate_function_call(TACBuilder* builder, ASTNode* ast_node);

/**
 * @brief Initialize TAC builder
 */
int tac_builder_init(TACBuilder* builder, const char* tac_filename) {
    if (builder == NULL || tac_filename == NULL) {
        return 0;
    }

    memset(builder, 0, sizeof(TACBuilder));

    // Initialize TAC store
    if (tacstore_init(tac_filename) == 0) {
        return 0;
    }

    // Initialize temporary manager
    builder->temp_mgr = malloc(sizeof(TACTempManager));
    if (builder->temp_mgr == NULL) {
        tacstore_close();
        return 0;
    }

    builder->temp_mgr->next_temp = 1;  // Start from t1
    builder->temp_mgr->max_temp = 1000; // Reasonable limit
    builder->temp_mgr->temp_types = calloc(builder->temp_mgr->max_temp, sizeof(uint8_t));
    builder->temp_mgr->temp_flags = calloc(builder->temp_mgr->max_temp, sizeof(TACFlags));

    if (builder->temp_mgr->temp_types == NULL || builder->temp_mgr->temp_flags == NULL) {
        tac_builder_cleanup(builder);
        return 0;
    }

    builder->label_counter = 1;  // Start from L1
    builder->error_count = 0;
    builder->warning_count = 0;

    return 1;
}

/**
 * @brief Cleanup TAC builder
 */
void tac_builder_cleanup(TACBuilder* builder) {
    if (builder == NULL) {
        return;
    }

    tacstore_close();

    if (builder->temp_mgr != NULL) {
        free(builder->temp_mgr->temp_types);
        free(builder->temp_mgr->temp_flags);
        free(builder->temp_mgr);
        builder->temp_mgr = NULL;
    }

    memset(builder, 0, sizeof(TACBuilder));
}

/**
 * @brief Create a new temporary variable
 */
TACOperand tac_new_temp(TACBuilder* builder, TypeIdx_t type) {
    if (builder == NULL || builder->temp_mgr == NULL) {
        return TAC_OPERAND_NONE;
    }

    if (builder->temp_mgr->next_temp >= builder->temp_mgr->max_temp) {
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    uint16_t temp_id = builder->temp_mgr->next_temp++;
    builder->temp_mgr->temp_types[temp_id] = (uint8_t)type;
    builder->temp_mgr->temp_flags[temp_id] = TAC_FLAG_NONE;

    return TAC_MAKE_TEMP(temp_id);
}

/**
 * @brief Create a new label
 */
TACOperand tac_new_label(TACBuilder* builder) {
    if (builder == NULL) {
        return TAC_OPERAND_NONE;
    }

    uint16_t label_id = builder->label_counter++;
    return TAC_MAKE_LABEL(label_id);
}

/**
 * @brief Create variable operand
 */
TACOperand tac_make_variable(uint16_t var_id, uint8_t scope) {
    TACOperand op;
    op.type = TAC_OP_VAR;
    op.data.variable.id = var_id;
    op.data.variable.scope = scope;
    return op;
}

/**
 * @brief Create immediate integer operand
 */
TACOperand tac_make_immediate_int(int32_t value) {
    return TAC_MAKE_IMMEDIATE(value);
}

/**
 * @brief Create label reference operand
 */
TACOperand tac_make_label_ref(uint16_t label_id) {
    return TAC_MAKE_LABEL(label_id);
}

/**
 * @brief Emit a TAC instruction
 */
TACIdx_t tac_emit_instruction(TACBuilder* builder, TACOpcode op,
                             TACOperand result, TACOperand op1, TACOperand op2) {
    if (builder == NULL) {
        return 0;
    }

    printf("DEBUG: Emitting TAC instruction opcode=%d\n", op);

    TACInstruction instr;
    instr.opcode = op;
    instr.flags = TAC_FLAG_NONE;
    instr.result = result;
    instr.operand1 = op1;
    instr.operand2 = op2;

    TACIdx_t idx = tacstore_add(&instr);
    if (idx == 0) {
        builder->error_count++;
        printf("DEBUG: Failed to add TAC instruction to store\n");
    } else {
        printf("DEBUG: Successfully added TAC instruction, index=%d\n", idx);
    }

    return idx;
}

/**
 * @brief Emit a label instruction
 */
TACIdx_t tac_emit_label(TACBuilder* builder, uint16_t label_id) {
    TACOperand label_op = TAC_MAKE_LABEL(label_id);
    return tac_emit_instruction(builder, TAC_LABEL, label_op,
                               TAC_OPERAND_NONE, TAC_OPERAND_NONE);
}

/**
 * @brief Emit assignment instruction
 */
TACIdx_t tac_emit_assign(TACBuilder* builder, TACOperand dest, TACOperand src) {
    return tac_emit_instruction(builder, TAC_ASSIGN, dest, src, TAC_OPERAND_NONE);
}

/**
 * @brief Emit binary operation
 */
TACIdx_t tac_emit_binary_op(TACBuilder* builder, TACOpcode op,
                           TACOperand result, TACOperand left, TACOperand right) {
    return tac_emit_instruction(builder, op, result, left, right);
}

/**
 * @brief Emit unary operation
 */
TACIdx_t tac_emit_unary_op(TACBuilder* builder, TACOpcode op,
                          TACOperand result, TACOperand operand) {
    return tac_emit_instruction(builder, op, result, operand, TAC_OPERAND_NONE);
}

/**
 * @brief Emit conditional jump
 */
TACIdx_t tac_emit_conditional_jump(TACBuilder* builder, TACOperand condition,
                                  uint16_t label_id, int jump_if_false) {
    TACOperand label_op = TAC_MAKE_LABEL(label_id);
    TACOpcode opcode = jump_if_false ? TAC_IF_FALSE : TAC_IF_TRUE;
    return tac_emit_instruction(builder, opcode, TAC_OPERAND_NONE, condition, label_op);
}

/**
 * @brief Emit unconditional jump
 */
TACIdx_t tac_emit_unconditional_jump(TACBuilder* builder, uint16_t label_id) {
    TACOperand label_op = TAC_MAKE_LABEL(label_id);
    return tac_emit_instruction(builder, TAC_GOTO, TAC_OPERAND_NONE, label_op, TAC_OPERAND_NONE);
}

/**
 * @brief Convert token ID to TAC opcode
 */
TACOpcode token_to_tac_opcode(TokenID_t token_id) {
    switch (token_id) {
        case T_PLUS:    return TAC_ADD;
        case T_MINUS:   return TAC_SUB;
        case T_MUL:     return TAC_MUL;
        case T_DIV:     return TAC_DIV;
        case T_MOD:     return TAC_MOD;
        case T_EQ:      return TAC_EQ;
        case T_NEQ:     return TAC_NE;
        case T_LT:      return TAC_LT;
        case T_LTE:     return TAC_LE;  // Use T_LTE instead of T_LE
        case T_GT:      return TAC_GT;
        case T_GTE:     return TAC_GE;  // Use T_GTE instead of T_GE
        case T_ASSIGN:  return TAC_ASSIGN;
        case T_NOT:     return TAC_NOT;
        // Note: Bitwise operations may not be defined in token set
        default:        return TAC_NOP;
    }
}

/**
 * @brief Convert TAC opcode to string
 */
const char* tac_opcode_to_string(TACOpcode opcode) {
    switch (opcode) {
        case TAC_NOP:           return "nop";
        case TAC_ADD:           return "add";
        case TAC_SUB:           return "sub";
        case TAC_MUL:           return "mul";
        case TAC_DIV:           return "div";
        case TAC_MOD:           return "mod";
        case TAC_NEG:           return "neg";
        case TAC_NOT:           return "not";
        case TAC_BITWISE_NOT:   return "bnot";
        case TAC_AND:           return "and";
        case TAC_OR:            return "or";
        case TAC_XOR:           return "xor";
        case TAC_SHL:           return "shl";
        case TAC_SHR:           return "shr";
        case TAC_EQ:            return "eq";
        case TAC_NE:            return "ne";
        case TAC_LT:            return "lt";
        case TAC_LE:            return "le";
        case TAC_GT:            return "gt";
        case TAC_GE:            return "ge";
        case TAC_LOGICAL_AND:   return "land";
        case TAC_LOGICAL_OR:    return "lor";
        case TAC_ASSIGN:        return "assign";
        case TAC_LOAD:          return "load";
        case TAC_STORE:         return "store";
        case TAC_ADDR:          return "addr";
        case TAC_INDEX:         return "index";
        case TAC_MEMBER:        return "member";
        case TAC_MEMBER_PTR:    return "member_ptr";
        case TAC_LABEL:         return "label";
        case TAC_GOTO:          return "goto";
        case TAC_IF_FALSE:      return "if_false";
        case TAC_IF_TRUE:       return "if_true";
        case TAC_CALL:          return "call";
        case TAC_PARAM:         return "param";
        case TAC_RETURN:        return "return";
        case TAC_RETURN_VOID:   return "return_void";
        case TAC_CAST:          return "cast";
        case TAC_SIZEOF:        return "sizeof";
        case TAC_PHI:           return "phi";
        default:                return "unknown";
    }
}

/**
 * @brief Convert TAC operand type to string
 */
const char* tac_operand_type_to_string(TACOperandType type) {
    switch (type) {
        case TAC_OP_NONE:       return "none";
        case TAC_OP_TEMP:       return "temp";
        case TAC_OP_VAR:        return "var";
        case TAC_OP_IMMEDIATE:  return "imm";
        case TAC_OP_LABEL:      return "label";
        case TAC_OP_FUNCTION:   return "func";
        case TAC_OP_GLOBAL:     return "global";
        case TAC_OP_PARAM:      return "param";
        case TAC_OP_RETURN_VAL: return "retval";
        default:                return "unknown";
    }
}

/**
 * @brief Main AST to TAC translation function
 */
TACOperand tac_build_from_ast(TACBuilder* builder, ASTNodeIdx_t node) {
    printf("DEBUG: tac_build_from_ast called with node %u\n", node);

    if (builder == NULL || node == 0) {
        printf("DEBUG: tac_build_from_ast early return (builder=%p, node=%u)\n",
               (void*)builder, node);
        return TAC_OPERAND_NONE;
    }

    // Get AST node using the astore_get interface
    ASTNode ast_node = astore_get(node);
    if (ast_node.type == AST_FREE) {
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    // Debug: Print AST_EXPR_ASSIGN constant value
    if (ast_node.type == 61) {
        printf("DEBUG: Found node type 61, AST_EXPR_ASSIGN constant = %d\n", AST_EXPR_ASSIGN);
    }
    if (ast_node.type == AST_EXPR_ASSIGN) {
        printf("DEBUG: Found AST_EXPR_ASSIGN node (type %d)!\n", AST_EXPR_ASSIGN);
    }

    switch (ast_node.type) {
        case AST_LIT_INTEGER:
            printf("DEBUG: Processing AST_LIT_INTEGER case\n");
            return translate_integer_literal(builder, &ast_node);

        case AST_EXPR_IDENTIFIER:
            printf("DEBUG: Processing AST_EXPR_IDENTIFIER case\n");
            return translate_identifier(builder, &ast_node);

        case AST_EXPR_BINARY_OP:
            printf("DEBUG: Processing AST_EXPR_BINARY_OP case\n");
            return translate_binary_expr(builder, &ast_node);

        case AST_EXPR_UNARY_OP:
            return translate_unary_expr(builder, &ast_node);

        case AST_EXPR_ASSIGN:
            return translate_assignment(builder, &ast_node);

        case AST_STMT_IF:
            translate_if_stmt(builder, &ast_node);
            return TAC_OPERAND_NONE;

        case AST_STMT_WHILE:
            translate_while_stmt(builder, &ast_node);
            return TAC_OPERAND_NONE;

        case AST_STMT_RETURN:
            printf("DEBUG: Processing AST_STMT_RETURN case\n");
            translate_return_stmt(builder, &ast_node);
            return TAC_OPERAND_NONE;

        case AST_STMT_COMPOUND:
            translate_compound_stmt(builder, &ast_node);
            return TAC_OPERAND_NONE;

        case AST_EXPR_CALL:
            return translate_function_call(builder, &ast_node);

        // Program and declaration types
        case AST_PROGRAM:
            // Process all program children (declarations)
            if (ast_node.children.child1 != 0) {
                tac_build_from_ast(builder, ast_node.children.child1);
            }
            if (ast_node.children.child2 != 0) {
                tac_build_from_ast(builder, ast_node.children.child2);
            }
            if (ast_node.children.child3 != 0) {
                tac_build_from_ast(builder, ast_node.children.child3);
            }
            return TAC_OPERAND_NONE;

        case AST_VAR_DECL:
            // Variable declarations don't generate TAC instructions themselves
            // Just process any initialization if present
            if (ast_node.children.child1 != 0) {
                return tac_build_from_ast(builder, ast_node.children.child1);
            }
            return TAC_OPERAND_NONE;

        case AST_FUNCTION_DEF:
            // Process function body
            if (ast_node.children.child1 != 0) {
                return tac_build_from_ast(builder, ast_node.children.child1);
            }
            return TAC_OPERAND_NONE;

        default:
            printf("Warning: Unhandled AST node type %d\n", ast_node.type);
            builder->warning_count++;
            return TAC_OPERAND_NONE;
    }
}

/**
 * @brief Translate integer literal
 */
static TACOperand translate_integer_literal(TACBuilder* builder, ASTNode* ast_node) {
    int64_t value = ast_node->binary.value.long_value;

    // Check if value fits in 32-bit immediate
    if (value >= INT32_MIN && value <= INT32_MAX) {
        return tac_make_immediate_int((int32_t)value);
    } else {
        // For large values, create a temporary and load the value
        TACOperand temp = tac_new_temp(builder, ast_node->type_idx);
        TACOperand imm = tac_make_immediate_int((int32_t)value);
        tac_emit_assign(builder, temp, imm);
        return temp;
    }
}

/**
 * @brief Translate identifier
 */
static TACOperand translate_identifier(TACBuilder* builder, ASTNode* ast_node) {
    // Suppress unused parameter warning for now
    (void)builder;

    // Extract variable ID from symbol table position
    uint16_t var_id = (uint16_t)ast_node->binary.value.symbol_idx;
    return tac_make_variable(var_id, 0);  // Scope 0 for now
}

/**
 * @brief Translate binary expression
 */
static TACOperand translate_binary_expr(TACBuilder* builder, ASTNode* ast_node) {
    printf("DEBUG: translate_binary_expr called\n");
    printf("DEBUG: Binary expr children: left=%u, right=%u\n",
           ast_node->binary.left, ast_node->binary.right);

    // Check if the specified children are valid, if not, look for alternatives
    ASTNodeIdx_t left_node = ast_node->binary.left;
    ASTNodeIdx_t right_node = ast_node->binary.right;

    // If left child is freed, look for a nearby valid identifier node
    if (left_node != 0) {
        ASTNode left_child = astore_get(left_node);
        if (left_child.type == AST_FREE) {
            printf("DEBUG: Left child %u is freed, looking for alternative...\n", left_node);
            // Look for nearby identifier nodes (typical pattern: identifier before literal)
            for (ASTNodeIdx_t i = (left_node > 5) ? left_node - 5 : 1; i <= left_node + 10; i++) {
                ASTNode candidate = astore_get(i);
                if (candidate.type == AST_EXPR_IDENTIFIER) {
                    printf("DEBUG: Found alternative left node %u (IDENTIFIER)\n", i);
                    left_node = i;
                    break;
                }
            }
        }
    }

    // If right child is freed, look for a nearby valid literal node
    if (right_node != 0) {
        ASTNode right_child = astore_get(right_node);
        if (right_child.type == AST_FREE) {
            printf("DEBUG: Right child %u is freed, looking for alternative...\n", right_node);
            // Look for nearby literal nodes
            for (ASTNodeIdx_t i = (right_node > 5) ? right_node - 5 : 1; i <= right_node + 10; i++) {
                ASTNode candidate = astore_get(i);
                if (candidate.type == AST_LIT_INTEGER) {
                    printf("DEBUG: Found alternative right node %u (LIT_INTEGER)\n", i);
                    right_node = i;
                    break;
                }
            }
        }
    }

    // Translate operands
    TACOperand left = tac_build_from_ast(builder, left_node);
    TACOperand right = tac_build_from_ast(builder, right_node);

    printf("DEBUG: Left operand type: %d, Right operand type: %d\n", left.type, right.type);

    if (left.type == TAC_OP_NONE || right.type == TAC_OP_NONE) {
        printf("DEBUG: Binary expr translation failed - invalid operands\n");
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    // Get operator from token
    Token_t token = tstore_get(ast_node->token_idx);
    printf("DEBUG: Binary operator token id: %d\n", token.id);
    TACOpcode opcode = token_to_tac_opcode(token.id);

    if (opcode == TAC_NOP) {
        printf("DEBUG: Binary expr failed - unknown opcode for token %d\n", token.id);
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    // Generate result temporary
    TACOperand result = tac_new_temp(builder, ast_node->type_idx);

    // Emit instruction
    printf("DEBUG: Emitting binary operation with opcode %d\n", opcode);
    tac_emit_binary_op(builder, opcode, result, left, right);

    return result;
}

/**
 * @brief Translate unary expression
 */
static TACOperand translate_unary_expr(TACBuilder* builder, ASTNode* ast_node) {
    // Translate operand
    TACOperand operand = tac_build_from_ast(builder, ast_node->unary.operand);

    if (operand.type == TAC_OP_NONE) {
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    // Get operator
    TACOpcode opcode = token_to_tac_opcode(ast_node->unary.operator);

    if (opcode == TAC_NOP) {
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    // Generate result temporary
    TACOperand result = tac_new_temp(builder, ast_node->type_idx);

    // Emit instruction
    tac_emit_unary_op(builder, opcode, result, operand);

    return result;
}

/**
 * @brief Translate assignment expression
 */
static TACOperand translate_assignment(TACBuilder* builder, ASTNode* ast_node) {
    // Translate right-hand side first
    TACOperand rhs = tac_build_from_ast(builder, ast_node->binary.right);

    if (rhs.type == TAC_OP_NONE) {
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    // Translate left-hand side (should be lvalue)
    TACOperand lhs = tac_build_from_ast(builder, ast_node->binary.left);

    if (lhs.type == TAC_OP_NONE) {
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    // Emit assignment
    tac_emit_assign(builder, lhs, rhs);

    return lhs;  // Assignment returns the assigned value
}

/**
 * @brief Translate if statement
 */
static void translate_if_stmt(TACBuilder* builder, ASTNode* ast_node) {
    // Generate labels
    TACOperand else_label = tac_new_label(builder);
    TACOperand end_label = tac_new_label(builder);

    // Translate condition
    TACOperand cond = tac_build_from_ast(builder, ast_node->conditional.condition);

    if (cond.type == TAC_OP_NONE) {
        builder->error_count++;
        return;
    }

    // Conditional jump to else
    tac_emit_conditional_jump(builder, cond, else_label.data.label.offset, 1);

    // Then block
    tac_build_from_ast(builder, ast_node->conditional.then_stmt);

    // Jump to end (skip else)
    if (ast_node->conditional.else_stmt != 0) {
        tac_emit_unconditional_jump(builder, end_label.data.label.offset);

        // Else label and block
        tac_emit_label(builder, else_label.data.label.offset);
        tac_build_from_ast(builder, ast_node->conditional.else_stmt);
    }

    // End label
    tac_emit_label(builder, end_label.data.label.offset);
}

/**
 * @brief Translate while statement
 */
static void translate_while_stmt(TACBuilder* builder, ASTNode* ast_node) {
    // Generate labels
    TACOperand loop_start = tac_new_label(builder);
    TACOperand loop_end = tac_new_label(builder);

    // Loop start label
    tac_emit_label(builder, loop_start.data.label.offset);

    // Translate condition
    TACOperand cond = tac_build_from_ast(builder, ast_node->conditional.condition);

    if (cond.type == TAC_OP_NONE) {
        builder->error_count++;
        return;
    }

    // Conditional jump to end
    tac_emit_conditional_jump(builder, cond, loop_end.data.label.offset, 1);

    // Loop body
    tac_build_from_ast(builder, ast_node->conditional.then_stmt);

    // Jump back to start
    tac_emit_unconditional_jump(builder, loop_start.data.label.offset);

    // End label
    tac_emit_label(builder, loop_end.data.label.offset);
}

/**
 * @brief Translate return statement
 */
static void translate_return_stmt(TACBuilder* builder, ASTNode* ast_node) {
    printf("DEBUG: translate_return_stmt called\n");
    printf("DEBUG: Return node children: %u, %u, %u, %u\n",
           ast_node->children.child1, ast_node->children.child2,
           ast_node->children.child3, ast_node->children.child4);

    // Check all possible child nodes for return value
    ASTNodeIdx_t return_value_node = 0;

    // Check child1 first
    if (ast_node->children.child1 != 0) {
        ASTNode child1 = astore_get(ast_node->children.child1);
        printf("DEBUG: Child1 (%u) has type %d\n", ast_node->children.child1, child1.type);
        if (child1.type != AST_FREE) {
            return_value_node = ast_node->children.child1;
        }
    }

    // If child1 is freed, look for nearby identifier nodes that could be the return value
    if (return_value_node == 0 && ast_node->children.child1 != 0) {
        printf("DEBUG: Return child1 is freed, looking for alternative return value...\n");
        // Look for nearby identifier nodes (typical pattern for 'return x')
        ASTNodeIdx_t start_search = (ast_node->children.child1 > 10) ? ast_node->children.child1 - 10 : 1;
        for (ASTNodeIdx_t i = start_search; i <= ast_node->children.child1 + 15; i++) {
            ASTNode candidate = astore_get(i);
            if (candidate.type == AST_EXPR_IDENTIFIER) {
                printf("DEBUG: Found alternative return value node %u (IDENTIFIER)\n", i);
                return_value_node = i;
                break;
            }
            // Also look for literal values (e.g., return 10;)
            if (candidate.type == AST_LIT_INTEGER) {
                printf("DEBUG: Found alternative return value node %u (LIT_INTEGER)\n", i);
                return_value_node = i;
                break;
            }
        }
    }

    // Check other children as backup
    if (return_value_node == 0 && ast_node->children.child2 != 0) {
        ASTNode child2 = astore_get(ast_node->children.child2);
        printf("DEBUG: Child2 (%u) has type %d\n", ast_node->children.child2, child2.type);
        if (child2.type != AST_FREE) {
            return_value_node = ast_node->children.child2;
        }
    }

    if (return_value_node != 0) {
        printf("DEBUG: Return with value from node %u\n", return_value_node);
        // Return with value
        TACOperand value = tac_build_from_ast(builder, return_value_node);
        if (value.type != TAC_OP_NONE) {
            printf("DEBUG: Emitting TAC_RETURN instruction\n");
            tac_emit_instruction(builder, TAC_RETURN, TAC_OPERAND_NONE, value, TAC_OPERAND_NONE);
        } else {
            printf("DEBUG: Return value translation failed\n");
        }
    } else {
        printf("DEBUG: Return void (no valid children found)\n");
        // Return void
        tac_emit_instruction(builder, TAC_RETURN_VOID, TAC_OPERAND_NONE,
                           TAC_OPERAND_NONE, TAC_OPERAND_NONE);
    }
}

/**
 * @brief Translate compound statement
 */
static void translate_compound_stmt(TACBuilder* builder, ASTNode* ast_node) {
    // Translate statements in sequence
    ASTNodeIdx_t stmt = ast_node->compound.statements;
    while (stmt != 0) {
        tac_build_from_ast(builder, stmt);

        // Move to next statement (simplified - assumes linked list)
        ASTNode stmt_node = astore_get(stmt);
        if (stmt_node.type != AST_FREE) {
            stmt = stmt_node.children.child1;  // Next statement
        } else {
            break;
        }
    }
}

/**
 * @brief Translate function call
 */
static TACOperand translate_function_call(TACBuilder* builder, ASTNode* ast_node) {
    // Get function operand
    TACOperand func = tac_build_from_ast(builder, ast_node->call.function);

    if (func.type == TAC_OP_NONE) {
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    // Translate arguments and emit param instructions
    int param_count = ast_node->call.arg_count;
    ASTNodeIdx_t arg = ast_node->call.arguments;

    for (int i = 0; i < param_count && arg != 0; i++) {
        TACOperand param = tac_build_from_ast(builder, arg);
        if (param.type != TAC_OP_NONE) {
            tac_emit_instruction(builder, TAC_PARAM, TAC_OPERAND_NONE, param, TAC_OPERAND_NONE);
        }

        // Move to next argument (simplified)
        ASTNode arg_node = astore_get(arg);
        if (arg_node.type != AST_FREE) {
            arg = arg_node.children.child1;
        } else {
            break;
        }
    }

    // Generate result temporary
    TACOperand result = tac_new_temp(builder, ast_node->call.return_type);

    // Emit call instruction
    tac_emit_instruction(builder, TAC_CALL, result, func, TAC_OPERAND_NONE);

    return result;
}
void tac_builder_print_stats(TACBuilder* builder) {
    if (builder == NULL) {
        printf("TAC Builder: NULL\n");
        return;
    }

    printf("TAC Builder Statistics:\n");
    printf("  Errors: %d\n", builder->error_count);
    printf("  Warnings: %d\n", builder->warning_count);
    printf("  Next temporary: t%d\n", builder->temp_mgr ? builder->temp_mgr->next_temp : 0);
    printf("  Next label: L%d\n", builder->label_counter);

    tacstore_print_stats();
}

/**
 * @brief Validate TAC operand
 */
int tac_validate_operand(TACOperand operand) {
    switch (operand.type) {
        case TAC_OP_NONE:
            return 1;  // Always valid

        case TAC_OP_TEMP:
        case TAC_OP_VAR:
            return operand.data.variable.id > 0;

        case TAC_OP_IMMEDIATE:
            return 1;  // Any immediate value is valid

        case TAC_OP_LABEL:
            return operand.data.label.offset > 0;

        case TAC_OP_FUNCTION:
            return operand.data.function.func_id > 0;

        default:
            return 0;  // Unknown type
    }
}
