//============================================================================//
// test_tac.c - Unit tests for TAC (Three Address Code) functionality
//
// Tests the STAS TAC generation and storage components to ensure proper
// handling of three-address code instructions, operands, and operations.
//============================================================================//

#include "../test_common.h"
#include "../../src/ir/tac_store.h"
#include "../../src/ir/tac_types.h"

//============================================================================//
// TEST FUNCTION PROTOTYPES
//============================================================================//

void test_tac_init_close(void);
void test_tac_instruction_storage(void);
void test_tac_basic_operations(void);
void run_tac_tests(void);

//============================================================================//
// SETUP AND TEARDOWN
//============================================================================//

void test_tac_init_close(void) {
    // Test TAC store initialization
    TEST_ASSERT_EQUAL(1, tacstore_init("tests/temp/test_init.tac"));  // Returns 1 on success
    tacstore_close();
}

//============================================================================//
// TAC STORAGE TESTS
//============================================================================//

void test_tac_instruction_storage(void) {
    TEST_ASSERT_EQUAL(1, tacstore_init("tests/temp/test_storage.tac"));  // Returns 1 on success
    
    // Create a test TAC instruction: t1 = 5 + 3
    TACInstruction instr;
    instr.opcode = TAC_ADD;
    instr.flags = TAC_FLAG_NONE;
    instr.result = TAC_MAKE_TEMP(1);
    instr.operand1 = TAC_MAKE_IMMEDIATE(5);
    instr.operand2 = TAC_MAKE_IMMEDIATE(3);
    
    // Store the instruction
    TACIdx_t idx = tacstore_add(&instr);
    TEST_ASSERT_TRUE(idx > 0);  // Should return valid index
    
    // Retrieve and verify the stored instruction  
    TACInstruction retrieved = tacstore_get(idx);
    TEST_ASSERT_EQUAL(TAC_ADD, retrieved.opcode);
    TEST_ASSERT_EQUAL(TAC_OP_TEMP, retrieved.result.type);
    TEST_ASSERT_EQUAL(1, retrieved.result.data.variable.id);
    TEST_ASSERT_EQUAL(TAC_OP_IMMEDIATE, retrieved.operand1.type);
    TEST_ASSERT_EQUAL(5, retrieved.operand1.data.immediate.value);
    TEST_ASSERT_EQUAL(TAC_OP_IMMEDIATE, retrieved.operand2.type);
    TEST_ASSERT_EQUAL(3, retrieved.operand2.data.immediate.value);
    
    tacstore_close();
}

//============================================================================//
// TAC BASIC OPERATIONS TESTS
//============================================================================//

void test_tac_basic_operations(void) {
    TEST_ASSERT_EQUAL(1, tacstore_init("tests/temp/test_basic_ops.tac"));  // Returns 1 on success
    
    // Test assignment: t1 = 42
    TACInstruction assign_instr;
    assign_instr.opcode = TAC_ASSIGN;
    assign_instr.flags = TAC_FLAG_NONE;
    assign_instr.result = TAC_MAKE_TEMP(1);
    assign_instr.operand1 = TAC_MAKE_IMMEDIATE(42);
    assign_instr.operand2 = TAC_OPERAND_NONE;
    
    TACIdx_t idx = tacstore_add(&assign_instr);
    TEST_ASSERT_TRUE(idx > 0);  // Should return valid index
    
    TACInstruction retrieved = tacstore_get(idx);
    TEST_ASSERT_EQUAL(TAC_ASSIGN, retrieved.opcode);
    TEST_ASSERT_EQUAL(42, retrieved.operand1.data.immediate.value);
    TEST_ASSERT_EQUAL(TAC_OP_NONE, retrieved.operand2.type);
    
    tacstore_close();
}

//============================================================================//
// TEST RUNNER
//============================================================================//

void run_tac_tests(void) {
    RUN_TEST(test_tac_init_close);
    RUN_TEST(test_tac_instruction_storage);
    RUN_TEST(test_tac_basic_operations);
}
