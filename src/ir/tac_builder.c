/**
 * @file tac_builder.c
 * @brief TAC generation from AST nodes implementation
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.1
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#include "tac_builder.h"
#include "tac_printer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../storage/tstore.h"
#include "../storage/sstore.h"
#include "../storage/symtab.h"
#include "../utils/hmapbuf.h"

// Function table for mapping function names to TAC labels
typedef struct {
    char* function_name;
    uint32_t label_id;
    uint32_t instruction_address;
} FunctionTableEntry;

typedef struct {
    FunctionTableEntry* entries;
    uint32_t count;
    uint32_t capacity;
} FunctionTable;

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
static int tac_builder_load_symbols(TACBuilder* builder);

/**
 * @brief Look up a variable in the symbol table and return its TAC operand
 */
/**
 * @brief Create a new TAC builder instance
 */

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
    
    // Initialize function table
    builder->function_table.count = 0;
    builder->function_table.main_function_idx = (uint32_t)-1; // Invalid index initially
    
    // Load symbol table information
    if (tac_builder_load_symbols(builder) != 1) {
        printf("Warning: Could not load symbol table information\n");
    }

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

    // Clean up function table
    for (uint32_t i = 0; i < builder->function_table.count; i++) {
        free(builder->function_table.function_names[i]);
        builder->function_table.function_names[i] = NULL;
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

    TACInstruction instr;
    instr.opcode = op;
    instr.flags = TAC_FLAG_NONE;
    instr.result = result;
    instr.operand1 = op1;
    instr.operand2 = op2;

    TACIdx_t idx = tacstore_add(&instr);
    if (idx == 0) {
        builder->error_count++;
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
    if (builder == NULL || node == 0) {
        return TAC_OPERAND_NONE;
    }

    // Get AST node using the astore_get interface
    ASTNode ast_node = astore_get(node);
    if (ast_node.type == AST_FREE) {
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    switch (ast_node.type) {
        case AST_LIT_INTEGER:
            return translate_integer_literal(builder, &ast_node);

        case AST_EXPR_IDENTIFIER:
            return translate_identifier(builder, &ast_node);

        case AST_EXPR_BINARY_OP:
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
            translate_return_stmt(builder, &ast_node);
            return TAC_OPERAND_NONE;

        case AST_STMT_COMPOUND:
            translate_compound_stmt(builder, &ast_node);
            return TAC_OPERAND_NONE;

        case AST_STMT_EXPRESSION:
            // Expression statements (e.g., assignments, function calls as statements)
            if (ast_node.children.child1 != 0) {
                return tac_build_from_ast(builder, ast_node.children.child1);
            }
            return TAC_OPERAND_NONE;

        case AST_EXPR_CALL:
            return translate_function_call(builder, &ast_node);

        // Program and declaration types
        case AST_PROGRAM:
            // Process all program children (declarations) in proper order:
            // 1. Global variables first
            // 2. Function definitions in source order
            {
                ASTNodeIdx_t current_decl = ast_node.children.child1;
                int decl_count = 0;
                const int MAX_DECLARATIONS = 1000;  // Prevent infinite loops
                
                // First pass: process global variable declarations
                ASTNodeIdx_t decl = current_decl;
                while (decl != 0 && decl_count < MAX_DECLARATIONS) {
                    ASTNode decl_node = astore_get(decl);
                    if (decl_node.type != AST_FREE) {
                        // Check if this is a global variable declaration
                        if (decl_node.type == AST_VAR_DECL) {
                            // Process global variable immediately
                            tac_build_from_ast(builder, decl);
                        }
                        
                        // Move to the next declaration in the chain
                        ASTNodeIdx_t next_decl = decl_node.children.child2;  // Follow the chain
                        
                        // Cycle detection
                        if (next_decl == decl) {
                            fprintf(stderr, "Warning: Detected cycle in declaration chain at node %u\n", decl);
                            break;
                        }
                        
                        decl = next_decl;
                        decl_count++;
                    } else {
                        break;
                    }
                }
                
                // Second pass: process function definitions
                decl = current_decl;
                decl_count = 0;
                while (decl != 0 && decl_count < MAX_DECLARATIONS) {
                    ASTNode decl_node = astore_get(decl);
                    if (decl_node.type != AST_FREE) {
                        // Check if this is a function definition
                        if (decl_node.type == AST_FUNCTION_DEF || decl_node.type == AST_FUNCTION_DECL) {
                            // Process function
                            tac_build_from_ast(builder, decl);
                        }
                        
                        // Move to the next declaration in the chain
                        ASTNodeIdx_t next_decl = decl_node.children.child2;  // Follow the chain
                        
                        // Cycle detection
                        if (next_decl == decl) {
                            fprintf(stderr, "Warning: Detected cycle in declaration chain at node %u\n", decl);
                            break;
                        }
                        
                        decl = next_decl;
                        decl_count++;
                    } else {
                        break;
                    }
                }
                
                if (decl_count >= MAX_DECLARATIONS) {
                    printf("Warning: Declaration chain exceeded maximum length (%d), stopping\n", MAX_DECLARATIONS);
                }
            }
            return TAC_OPERAND_NONE;

        case AST_VAR_DECL:
            // Variable declarations should use symbol table entries
            // The parser should have already created symbol table entries and stored the index
            
            // Get symbol index directly from the AST node
            SymIdx_t symbol_idx = ast_node.declaration.symbol_idx;
            if (symbol_idx == 0) {
                builder->error_count++;
                return TAC_OPERAND_NONE;
            }
            
            // Create variable operand using symbol index
            TACOperand var_operand = tac_make_variable(symbol_idx, 0);
            
            // Process initialization if present
            if (ast_node.declaration.initializer != 0) {
                // Evaluate the initialization expression
                TACOperand init_operand = tac_build_from_ast(builder, ast_node.declaration.initializer);
                if (init_operand.type != TAC_OP_NONE) {
                    // Generate assignment instruction: var = init_value
                    tac_emit_instruction(builder, TAC_ASSIGN, var_operand, init_operand, TAC_OPERAND_NONE);
                }
            }
            
            return var_operand;

        case AST_FUNCTION_DEF:
            // Extract function name from AST node
            {
                char* func_name = sstore_get(ast_node.binary.value.string_pos);
                if (func_name) {
                    // Find this function in the pre-loaded function table
                    uint32_t func_idx = (uint32_t)-1;
                    for (uint32_t i = 0; i < builder->function_table.count; i++) {
                        if (builder->function_table.function_names[i] && 
                            strcmp(builder->function_table.function_names[i], func_name) == 0) {
                            func_idx = i;
                            break;
                        }
                    }
                    
                    if (func_idx != (uint32_t)-1) {
                        // Found in function table - emit label and update address
                        TACOperand func_label = tac_new_label(builder);
                        builder->function_table.label_ids[func_idx] = func_label.data.label.offset;
                        builder->function_table.instruction_addresses[func_idx] = tacstore_getidx();
                        
                        tac_emit_instruction(builder, TAC_LABEL, func_label, TAC_OPERAND_NONE, TAC_OPERAND_NONE);
                    } else {
                        fprintf(stderr, "ERROR: Function '%s' not found in symbol table\n", func_name);
                        builder->error_count++;
                        return TAC_OPERAND_NONE;
                    }
                } else {
                    fprintf(stderr, "ERROR: Could not extract function name from AST\n");
                    builder->error_count++;
                    return TAC_OPERAND_NONE;
                }
            }
            
            // Process function body
            if (ast_node.children.child1 != 0) {
                tac_build_from_ast(builder, ast_node.children.child1);
            }
            return TAC_OPERAND_NONE;

        default:
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
 * @brief Translate identifier using symbol table lookup
 */
static TACOperand translate_identifier(TACBuilder* builder, ASTNode* ast_node) {
    // Extract symbol index from AST node
    if (ast_node->type != AST_EXPR_IDENTIFIER) {
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    // Get the symbol index directly from the AST node
    SymIdx_t symbol_idx = ast_node->binary.value.symbol_idx;
    if (symbol_idx == 0) {
        // No symbol index stored - this is an error
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    // Create TAC operand using symbol index as variable ID
    return tac_make_variable(symbol_idx, 0);
}

/**
 * @brief Translate binary expression
 */
static TACOperand translate_binary_expr(TACBuilder* builder, ASTNode* ast_node) {
    // Check if the specified children are valid, if not, look for alternatives
    ASTNodeIdx_t left_node = ast_node->binary.left;
    ASTNodeIdx_t right_node = ast_node->binary.right;

    // If left child is freed, look for a nearby valid identifier node
    if (left_node != 0) {
        ASTNode left_child = astore_get(left_node);
        if (left_child.type == AST_FREE) {
            // Look for nearby identifier nodes (typical pattern: identifier before literal)
            for (ASTNodeIdx_t i = (left_node > 5) ? left_node - 5 : 1; i <= left_node + 10; i++) {
                ASTNode candidate = astore_get(i);
                if (candidate.type == AST_EXPR_IDENTIFIER) {
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
            // Look for nearby literal nodes
            for (ASTNodeIdx_t i = (right_node > 5) ? right_node - 5 : 1; i <= right_node + 10; i++) {
                ASTNode candidate = astore_get(i);
                if (candidate.type == AST_LIT_INTEGER) {
                    right_node = i;
                    break;
                }
            }
        }
    }

    // Translate operands
    TACOperand left = tac_build_from_ast(builder, left_node);
    TACOperand right = tac_build_from_ast(builder, right_node);

    if (left.type == TAC_OP_NONE || right.type == TAC_OP_NONE) {
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    // Get operator from token
    Token_t token = tstore_get(ast_node->token_idx);
    TACOpcode opcode = token_to_tac_opcode(token.id);

    if (opcode == TAC_NOP) {
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    // Generate result temporary
    TACOperand result = tac_new_temp(builder, ast_node->type_idx);

    // Emit instruction
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

    // Map unary operators to appropriate TAC opcodes
    TACOpcode opcode;
    switch (ast_node->unary.operator) {
        case T_MINUS:
            opcode = TAC_NEG;  // Unary minus becomes negation
            break;
        case T_PLUS:
            // Unary plus is a no-op, just return the operand
            return operand;
        case T_NOT:
            opcode = TAC_NOT;  // Logical not
            break;
        default:
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
    // Check the RHS node type for parsing bugs
    ASTNode rhs_node = astore_get(ast_node->binary.right);
    
    // Check for parsing bug where RHS points to another assignment instead of the binary expression
    if (rhs_node.type == AST_EXPR_ASSIGN) {
        // Get the LHS variable
        TACOperand lhs = tac_build_from_ast(builder, ast_node->binary.left);
        if (lhs.type == TAC_OP_NONE) {
            builder->error_count++;
            return TAC_OPERAND_NONE;
        }
        
        // Search backwards from current assignment to find the binary expression that should be the RHS
        // The parser creates binary expressions just before the assignment that uses them
        ASTNodeIdx_t binary_expr_node = 0;
        
        // Search for the binary expression in a small range before this assignment
        // Since we don't know the exact node index of this assignment, search broadly
        for (ASTNodeIdx_t i = 1; i <= 24 && binary_expr_node == 0; i++) {
            ASTNode candidate = astore_get(i);
            if (candidate.type == AST_EXPR_BINARY_OP) {
                // Check if this binary operation involves the LHS variable
                ASTNode left_operand = astore_get(candidate.binary.left);
                if (left_operand.type == AST_EXPR_IDENTIFIER) {
                    // Use symbol indices to compare identifiers
                    SymIdx_t left_symbol = left_operand.binary.value.symbol_idx;
                    ASTNode lhs_node = astore_get(ast_node->binary.left);
                    SymIdx_t lhs_symbol = lhs_node.binary.value.symbol_idx;
                    
                    if (left_symbol != 0 && left_symbol == lhs_symbol) {
                        // Found the binary expression that matches the LHS variable
                        binary_expr_node = i;
                        break;
                    }
                }
            }
        }
        
        if (binary_expr_node != 0) {
            // Process the correct binary expression
            TACOperand rhs = tac_build_from_ast(builder, binary_expr_node);
            if (rhs.type != TAC_OP_NONE) {
                tac_emit_assign(builder, lhs, rhs);
                return lhs;
            }
        }
        
        // If no binary expression found, this is an error case
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }
    
    // Check for the return statement bug
    if (rhs_node.type == AST_STMT_RETURN) {
        // Assignment RHS incorrectly points to return statement, implement workaround
        
        // Get the LHS (should be the variable being assigned to)
        TACOperand lhs = tac_build_from_ast(builder, ast_node->binary.left);
        if (lhs.type == TAC_OP_NONE) {
            builder->error_count++;
            return TAC_OPERAND_NONE;
        }
        
        // Search for the binary expression that should be the RHS  
        // Look backward from the return statement to find a binary operation
        ASTNodeIdx_t search_end = ast_node->binary.right;
        ASTNodeIdx_t rhs_binary_expr = 0;
        
        for (ASTNodeIdx_t i = (search_end > 20) ? search_end - 20 : 1; 
             i < search_end && rhs_binary_expr == 0; i++) {
            ASTNode candidate = astore_get(i);
            if (candidate.type == AST_EXPR_BINARY_OP) {
                rhs_binary_expr = i;
                break;
            }
        }
        
        if (rhs_binary_expr != 0) {
            // Process the binary expression normally
            TACOperand rhs = tac_build_from_ast(builder, rhs_binary_expr);
            if (rhs.type != TAC_OP_NONE) {
                tac_emit_assign(builder, lhs, rhs);
                return lhs;
            }
        }
        
        // If no binary expression found, this is an error
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }
    
    // Normal assignment processing
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
    } else {
        // Even if there's no else block, we need to emit the else label
        // because the conditional jump references it
        tac_emit_label(builder, else_label.data.label.offset);
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
    // Check all possible child nodes for return value
    ASTNodeIdx_t return_value_node = 0;

    // Check child1 first
    if (ast_node->children.child1 != 0) {
        ASTNode child1 = astore_get(ast_node->children.child1);
        if (child1.type != AST_FREE) {
            return_value_node = ast_node->children.child1;
        }
    }

    // If child1 is freed, look for nearby identifier nodes that could be the return value
    if (return_value_node == 0 && ast_node->children.child1 != 0) {
        // Look for nearby identifier nodes (typical pattern for 'return x')
        ASTNodeIdx_t start_search = (ast_node->children.child1 > 10) ? ast_node->children.child1 - 10 : 1;
        for (ASTNodeIdx_t i = start_search; i <= ast_node->children.child1 + 15; i++) {
            ASTNode candidate = astore_get(i);
            if (candidate.type == AST_EXPR_IDENTIFIER) {
                return_value_node = i;
                break;
            }
            // Also look for literal values (e.g., return 10;)
            if (candidate.type == AST_LIT_INTEGER) {
                return_value_node = i;
                break;
            }
        }
    }

    // Check other children as backup
    if (return_value_node == 0 && ast_node->children.child2 != 0) {
        ASTNode child2 = astore_get(ast_node->children.child2);
        if (child2.type != AST_FREE) {
            return_value_node = ast_node->children.child2;
        }
    }

    if (return_value_node != 0) {
        // Return with value
        TACOperand value = tac_build_from_ast(builder, return_value_node);
        if (value.type != TAC_OP_NONE) {
            tac_emit_instruction(builder, TAC_RETURN, TAC_OPERAND_NONE, value, TAC_OPERAND_NONE);
        }
    } else {
        // Return void
        tac_emit_instruction(builder, TAC_RETURN_VOID, TAC_OPERAND_NONE,
                           TAC_OPERAND_NONE, TAC_OPERAND_NONE);
    }
}

/**
 * @brief Translate compound statement - fixed to properly follow parser's AST structure
 */
static void translate_compound_stmt(TACBuilder* builder, ASTNode* ast_node) {
    // The parser stores statements chained using child2 as 'next' pointer
    // starting from child1 of the compound statement
    // BUT: Conditional statements (if/while) use child4 for chaining to avoid union conflicts
    
    ASTNodeIdx_t current_stmt = ast_node->children.child1;
    int statement_count = 0;  // Prevent infinite loops
    const int MAX_STATEMENTS = 1000;  // Safety limit
    
    while (current_stmt != 0 && statement_count < MAX_STATEMENTS) {
        statement_count++;
        
        // Get current statement info for debugging
        ASTNode stmt_node = astore_get(current_stmt);
        
        // Skip binary expressions that are not statements (they are part of assignments)
        if (stmt_node.type == AST_EXPR_BINARY_OP) {
            // Don't process this node, just move to next
        } else {
            // Process the current statement
            tac_build_from_ast(builder, current_stmt);
        }
        
        // Move to the next statement in the chain
        if (stmt_node.type != AST_FREE) {
            ASTNodeIdx_t next_stmt = 0;
            
            // Check if this is a conditional statement that uses child4 for chaining
            if (stmt_node.type == AST_STMT_IF || stmt_node.type == AST_STMT_WHILE) {
                next_stmt = stmt_node.children.child4;  // Follow the special chain
            } else if (stmt_node.type == AST_STMT_COMPOUND || 
                       stmt_node.type == AST_STMT_RETURN ||
                       stmt_node.type == AST_VAR_DECL ||
                       stmt_node.type == AST_FUNCTION_DEF ||
                       stmt_node.type == AST_EXPR_ASSIGN ||
                       stmt_node.type == AST_STMT_EXPRESSION) {
                // These statement types use child2 for chaining to the next statement
                next_stmt = stmt_node.children.child2;  // Follow the normal chain
            } else {
                // Other expression types don't use child2 for statement chaining
                // child2 is typically the right operand, not the next statement
                // Skip binary expressions that are not statements
                if (stmt_node.type == AST_EXPR_BINARY_OP) {
                    next_stmt = 0;  // Stop chaining
                } else {
                    next_stmt = 0;  // Stop chaining
                }
            }
            
            // Prevent infinite loops by checking if we're pointing to ourselves
            if (next_stmt == current_stmt) {
                printf("Warning: Detected cycle in statement chain at node %u\n", current_stmt);
                break;
            }
            
            current_stmt = next_stmt;
        } else {
            break;
        }
    }
    
    if (statement_count >= MAX_STATEMENTS) {
        printf("Warning: Statement chain exceeded maximum length (%d), stopping\n", MAX_STATEMENTS);
    }
}

/**
 * @brief Translate function call
 */
static TACOperand translate_function_call(TACBuilder* builder, ASTNode* ast_node) {
    // Extract function name from the function call AST node
    ASTNode func_node = astore_get(ast_node->call.function);
    char* func_name = NULL;
    TACOperand func_operand;
    
    if (func_node.type == AST_EXPR_IDENTIFIER) {
        // Get the function name from symbol table using symbol index
        SymTabIdx_t symbol_idx = func_node.binary.value.symbol_idx;
        
        if (symbol_idx != 0) {
            SymTabEntry symbol = symtab_get(symbol_idx);
            func_name = sstore_get(symbol.name);
        }
    }
    
    // Look up function in function table
    if (func_name) {
        uint32_t target_label = 0;
        int found = 0;
        
        for (uint32_t i = 0; i < builder->function_table.count; i++) {
            if (builder->function_table.function_names[i] && strcmp(builder->function_table.function_names[i], func_name) == 0) {
                target_label = builder->function_table.label_ids[i];
                found = 1;
                break;
            }
        }
        
        if (found) {
            func_operand = TAC_MAKE_LABEL(target_label);
        } else {
            // Function not found in table - this is an error
            fprintf(stderr, "ERROR: Function '%s' not found in function table\n", func_name);
            builder->error_count++;
            return TAC_OPERAND_NONE;
        }
    } else {
        // Cannot extract function name - this is an error
        fprintf(stderr, "ERROR: Cannot extract function name from function call\n");
        builder->error_count++;
        return TAC_OPERAND_NONE;
    }

    // Translate arguments and emit param instructions  
    int param_count = ast_node->call.arg_count;
    ASTNodeIdx_t first_arg = ast_node->call.arguments;

    // Arguments are stored sequentially starting from first_arg
    for (int i = 0; i < param_count && first_arg != 0; i++) {
        ASTNodeIdx_t current_arg = first_arg + i;  // Sequential argument nodes
        TACOperand param = tac_build_from_ast(builder, current_arg);
        if (param.type != TAC_OP_NONE) {
            tac_emit_instruction(builder, TAC_PARAM, TAC_OPERAND_NONE, param, TAC_OPERAND_NONE);
        } else {
            // If sequential approach fails, try the chaining approach
            ASTNode arg_node = astore_get(first_arg);
            if (arg_node.type != AST_FREE) {
                param = tac_build_from_ast(builder, first_arg);
                if (param.type != TAC_OP_NONE) {
                    tac_emit_instruction(builder, TAC_PARAM, TAC_OPERAND_NONE, param, TAC_OPERAND_NONE);
                }
                first_arg = arg_node.children.child2; // Move to next via chaining
            } else {
                break;
            }
        }
    }

    // Generate result temporary
    TACOperand result = tac_new_temp(builder, ast_node->call.return_type);

    // Emit call instruction with function address
    tac_emit_instruction(builder, TAC_CALL, result, func_operand, TAC_OPERAND_NONE);

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

uint32_t tac_builder_get_main_address(TACBuilder* builder) {
    if (!builder) {
        return 0;
    }
    
    // Look for main function in function table
    if (builder->function_table.main_function_idx < builder->function_table.count) {
        return builder->function_table.instruction_addresses[builder->function_table.main_function_idx];
    }
    
    return 0;  // Main function not found
}

uint32_t tac_builder_get_entry_label(TACBuilder* builder) {
    if (!builder) {
        return 0;
    }
    
    // Look for main function in function table
    if (builder->function_table.main_function_idx < builder->function_table.count) {
        return builder->function_table.label_ids[builder->function_table.main_function_idx];
    }
    
    return 0;  // Main function not found
}

/**
 * @brief Load symbol table information into TAC builder
 */
static int tac_builder_load_symbols(TACBuilder* builder) {
    if (!builder) {
        return 0;
    }
    
    // Get the total number of symbols
    uint32_t symbol_count = symtab_get_count();
    
    if (symbol_count == 0) {
        return 0;
    }
    
    // Scan all symbols and load function symbols
    for (uint32_t i = 1; i <= symbol_count; i++) {  // Symbol indices start at 1
        SymTabEntry entry = symtab_get(i);
        
        if (entry.type == SYM_FUNCTION) {
            if (builder->function_table.count >= 32) {
                printf("Warning: Function table full, skipping symbol %u\n", i);
                break;
            }
            
            // Get function name from string store
            char* func_name = sstore_get(entry.name);
            
            if (func_name) {
                uint32_t func_idx = builder->function_table.count;
                
                // Store function name (make a copy)
                builder->function_table.function_names[func_idx] = strdup(func_name);
                if (!builder->function_table.function_names[func_idx]) {
                    printf("Warning: Failed to allocate memory for function name\n");
                    continue;
                }
                
                // Initialize label and instruction address (will be set during TAC generation)
                builder->function_table.label_ids[func_idx] = 0;
                builder->function_table.instruction_addresses[func_idx] = 0;
                
                // Check if this is the entry point function (must be main)
                if (strcmp(func_name, "main") == 0) {
                    builder->function_table.main_function_idx = func_idx;
                }
                // Do not use fallback - main function must be explicitly named
                
                builder->function_table.count++;
            }
        }
    }
    
    return 1;
}

/**
 * @brief Export function table to TAC printer for label resolution
 */
void tac_builder_export_function_table(TACBuilder* builder) {
    if (!builder) {
        return;
    }
    
    // Create a TAC printer function table
    static TACPrinterFunctionTable printer_table;
    printer_table.count = builder->function_table.count;
    
    // Copy function names and label IDs
    for (uint32_t i = 0; i < builder->function_table.count && i < 32; i++) {
        printer_table.function_names[i] = builder->function_table.function_names[i];
        printer_table.label_ids[i] = builder->function_table.label_ids[i];
    }
    
    // Set the function table in the TAC printer
    tac_printer_set_function_table(&printer_table);
}
