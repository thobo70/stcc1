/**
 * @file tac_test.c
 * @brief Simple test program for TAC generation
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.1
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#include <stdio.h>
#include "../ir/tac_builder.h"
#include "../ir/tac_printer.h"
#include "../storage/sstore.h"
#include "../storage/tstore.h"

/**
 * @brief Simple TAC generation test
 */
int main(void) {
    printf("=== TAC Generation Test ===\n");
    
    // Initialize storage systems
    if (sstore_init("test_strings.dat") != 0) {
        printf("Error: Cannot initialize string store\n");
        return 1;
    }
    
    if (tstore_init("test_tokens.dat") != 0) {
        printf("Error: Cannot initialize token store\n");
        sstore_close();
        return 1;
    }
    
    // Initialize TAC builder
    TACBuilder builder;
    if (!tac_builder_init(&builder, "test_tac.dat")) {
        printf("Error: Cannot initialize TAC builder\n");
        tstore_close();
        sstore_close();
        return 1;
    }
    
    printf("TAC Builder initialized successfully\n\n");
    
    // Test 1: Simple arithmetic
    printf("Test 1: Simple arithmetic (a = 5 + 3)\n");
    TACOperand a = tac_make_variable(1, 0);
    TACOperand five = tac_make_immediate_int(5);
    TACOperand three = tac_make_immediate_int(3);
    TACOperand temp1 = tac_new_temp(&builder, 0);
    
    tac_emit_binary_op(&builder, TAC_ADD, temp1, five, three);
    tac_emit_assign(&builder, a, temp1);
    
    // Test 2: Conditional jump
    printf("Test 2: Conditional (if a > 0)\n");
    TACOperand zero = tac_make_immediate_int(0);
    TACOperand temp2 = tac_new_temp(&builder, 0);
    TACOperand label1 = tac_new_label(&builder);
    
    tac_emit_binary_op(&builder, TAC_GT, temp2, a, zero);
    tac_emit_conditional_jump(&builder, temp2, label1.data.label.offset, 1);
    
    // Test 3: Function call simulation
    printf("Test 3: Function call simulation\n");
    TACOperand param1 = tac_make_immediate_int(42);
    TACOperand func = tac_make_variable(100, 0);  // Function ID
    TACOperand result = tac_new_temp(&builder, 0);
    
    tac_emit_instruction(&builder, TAC_PARAM, TAC_OPERAND_NONE, param1, TAC_OPERAND_NONE);
    tac_emit_instruction(&builder, TAC_CALL, result, func, TAC_OPERAND_NONE);
    
    // Test 4: Label and jump
    printf("Test 4: Label and jump\n");
    tac_emit_label(&builder, label1.data.label.offset);
    TACOperand label2 = tac_new_label(&builder);
    tac_emit_unconditional_jump(&builder, label2.data.label.offset);
    tac_emit_label(&builder, label2.data.label.offset);
    
    printf("\nGenerated TAC Instructions:\n");
    printf("==========================\n");
    tac_print_all_instructions();
    
    printf("TAC Statistics:\n");
    printf("===============\n");
    tac_print_statistics();
    
    printf("Operand Usage Analysis:\n");
    printf("=======================\n");
    tac_analyze_operand_usage();
    
    // Write TAC to file
    tac_write_to_file("test_output.tac");
    
    // Print builder statistics
    tac_builder_print_stats(&builder);
    
    // Cleanup
    tac_builder_cleanup(&builder);
    tstore_close();
    sstore_close();
    
    printf("\nTAC generation test completed successfully!\n");
    return 0;
}
