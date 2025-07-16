/**
 * @file tac_printer.h
 * @brief TAC pretty printing and debugging support
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 0.1
 * @date 2025-07-16
 * @copyright Copyright (c) 2024-2025 Thomas Boos
 */

#ifndef SRC_IR_TAC_PRINTER_H_
#define SRC_IR_TAC_PRINTER_H_

#include "tac_types.h"
#include "tac_store.h"
#include <stdio.h>

// TAC printing functions
void tac_print_instruction(TACInstruction instr,
                     TACIdx_t idx);
void tac_print_operand(TACOperand operand);
void tac_print_all_instructions(void);
void tac_print_range(TACIdx_t start,
                     TACIdx_t end);

// TAC output to file
void tac_write_to_file(const char* filename);
void tac_write_range_to_file(const char* filename,
                     TACIdx_t start,
                     TACIdx_t end);

// TAC analysis and statistics
void tac_print_statistics(void);
void tac_analyze_operand_usage(void);

#endif  // SRC_IR_TAC_PRINTER_H_
