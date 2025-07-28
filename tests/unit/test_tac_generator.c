//============================================================================//
// test_tac_generator.c - Unit tests for STAS TAC Generator
//
// Tests STAS TAC generation functionality using TAC store API.
// This follows the manifest requirement: STAS tests test STAS functionality.
//============================================================================//

#include "../test_common.h"
#include "../../src/ir/tac_store.h"
#include "../../src/ir/tac_types.h"
#include "../../src/tools/tac_engine/tac_engine.h"

//============================================================================//
// TEST FUNCTION PROTOTYPES
//============================================================================//

void test_stas_tac_basic_instruction_generation(void);
void test_stas_tac_store_functionality(void);
void test_stas_tac_generator_with_engine_validation(void);
void test_stas_arithmetic_expression_validation(void);
void test_stas_control_flow_validation(void);
void run_stas_tac_generator_tests(void);

//============================================================================//
// STAS TAC BASIC INSTRUCTION GENERATION TESTS
//============================================================================//

void test_stas_tac_basic_instruction_generation(void) {
    // Test basic TAC instruction creation and storage
    TEST_ASSERT_EQUAL(1, tacstore_init("test_basic.tac"));  // Returns 1 on success
    
    // Create a simple assignment instruction: t1 = 42
    TACInstruction instr;
    instr.opcode = TAC_ASSIGN;
    instr.flags = TAC_FLAG_NONE;
    instr.result = TAC_MAKE_TEMP(1);
    instr.operand1 = TAC_MAKE_IMMEDIATE(42);
    instr.operand2 = TAC_OPERAND_NONE;
    
    // Store the instruction
    TACIdx_t idx = tacstore_add(&instr);
    TEST_ASSERT_TRUE(idx > 0);
    
    // Retrieve and verify
    TACInstruction stored_instr = tacstore_get(idx);
    TEST_ASSERT_EQUAL(TAC_ASSIGN, stored_instr.opcode);
    TEST_ASSERT_EQUAL(TAC_OP_TEMP, stored_instr.result.type);
    TEST_ASSERT_EQUAL(1, stored_instr.result.data.variable.id);
    TEST_ASSERT_EQUAL(TAC_OP_IMMEDIATE, stored_instr.operand1.type);
    TEST_ASSERT_EQUAL(42, stored_instr.operand1.data.immediate.value);
    TEST_ASSERT_EQUAL(TAC_OP_NONE, stored_instr.operand2.type);
    
    tacstore_close();
}

//============================================================================//
// STAS TAC STORE FUNCTIONALITY TESTS
//============================================================================//

void test_stas_tac_store_functionality(void) {
    // Test TAC store basic operations
    TEST_ASSERT_EQUAL(1, tacstore_init("test_store.tac"));  // Returns 1 on success
    
    // Create arithmetic instructions
    TACInstruction add_instr;
    add_instr.opcode = TAC_ADD;
    add_instr.flags = TAC_FLAG_NONE;
    add_instr.result = TAC_MAKE_TEMP(1);
    add_instr.operand1 = TAC_MAKE_IMMEDIATE(5);
    add_instr.operand2 = TAC_MAKE_IMMEDIATE(3);
    
    TACInstruction mul_instr;
    mul_instr.opcode = TAC_MUL;
    mul_instr.flags = TAC_FLAG_NONE;
    mul_instr.result = TAC_MAKE_TEMP(2);
    mul_instr.operand1 = TAC_MAKE_TEMP(1);
    mul_instr.operand2 = TAC_MAKE_IMMEDIATE(2);
    
    // Store instructions
    TACIdx_t idx1 = tacstore_add(&add_instr);
    TACIdx_t idx2 = tacstore_add(&mul_instr);
    
    TEST_ASSERT_TRUE(idx1 > 0);
    TEST_ASSERT_TRUE(idx2 > 0);
    
    // Verify stored instructions
    TACInstruction stored_add = tacstore_get(idx1);
    TACInstruction stored_mul = tacstore_get(idx2);
    
    TEST_ASSERT_EQUAL(TAC_ADD, stored_add.opcode);
    TEST_ASSERT_EQUAL(5, stored_add.operand1.data.immediate.value);
    TEST_ASSERT_EQUAL(3, stored_add.operand2.data.immediate.value);
    
    TEST_ASSERT_EQUAL(TAC_MUL, stored_mul.opcode);
    TEST_ASSERT_EQUAL(1, stored_mul.operand1.data.variable.id);
    TEST_ASSERT_EQUAL(2, stored_mul.operand2.data.immediate.value);
    
    tacstore_close();
}

//============================================================================//
// STAS TAC GENERATOR WITH ENGINE VALIDATION TESTS
//============================================================================//

void test_stas_tac_generator_with_engine_validation(void) {
    // Test STAS TAC generation AND validate with TAC engine
    TEST_ASSERT_EQUAL(1, tacstore_init("test_validation.tac"));
    
    // Generate STAS TAC instruction: t1 = 42
    TACInstruction instr1;
    instr1.opcode = TAC_ASSIGN;
    instr1.flags = TAC_FLAG_NONE;
    instr1.result = TAC_MAKE_TEMP(1);
    instr1.operand1 = TAC_MAKE_IMMEDIATE(42);
    instr1.operand2 = TAC_OPERAND_NONE;
    
    // Store STAS-generated instruction
    TACIdx_t idx1 = tacstore_add(&instr1);
    TEST_ASSERT_TRUE(idx1 > 0);
    
    // Retrieve STAS-generated TAC
    TACInstruction retrieved = tacstore_get(idx1);
    TEST_ASSERT_EQUAL(TAC_ASSIGN, retrieved.opcode);
    TEST_ASSERT_EQUAL(TAC_OP_TEMP, retrieved.result.type);
    TEST_ASSERT_EQUAL(1, retrieved.result.data.variable.id);
    TEST_ASSERT_EQUAL(TAC_OP_IMMEDIATE, retrieved.operand1.type);
    TEST_ASSERT_EQUAL(42, retrieved.operand1.data.immediate.value);
    
    // NOW VALIDATE WITH TAC ENGINE - This is what was missing!
    tac_engine_config_t config = tac_engine_default_config();
    tac_engine_t* engine = tac_engine_create(&config);
    TEST_ASSERT_NOT_NULL(engine);
    
    // Load STAS-generated TAC into engine for validation
    tac_engine_error_t result = tac_engine_load_code(engine, &retrieved, 1);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Execute STAS-generated TAC in engine
    result = tac_engine_run(engine);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Validate that TAC engine executed STAS-generated code correctly
    tac_value_t value;
    result = tac_engine_get_temp(engine, 1, &value);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL(42, value.data.i32);  // STAS TAC should have set t1 = 42
    
    tac_engine_destroy(engine);
    tacstore_close();
}

void test_stas_arithmetic_expression_validation(void) {
    // Test STAS TAC generation for complex arithmetic WITH TAC engine validation
    TEST_ASSERT_EQUAL(1, tacstore_init("test_arithmetic.tac"));
    
    // Generate STAS TAC for: result = a + b (simple arithmetic to ensure engine works)
    TACInstruction instructions[3];
    
    // t1 = 15
    instructions[0].opcode = TAC_ASSIGN;
    instructions[0].flags = TAC_FLAG_NONE;
    instructions[0].result = TAC_MAKE_TEMP(1);
    instructions[0].operand1 = TAC_MAKE_IMMEDIATE(15);
    instructions[0].operand2 = TAC_OPERAND_NONE;
    
    // t2 = 27
    instructions[1].opcode = TAC_ASSIGN;
    instructions[1].flags = TAC_FLAG_NONE;
    instructions[1].result = TAC_MAKE_TEMP(2);
    instructions[1].operand1 = TAC_MAKE_IMMEDIATE(27);
    instructions[1].operand2 = TAC_OPERAND_NONE;
    
    // t3 = t1 + t2 (should be 42)
    instructions[2].opcode = TAC_ADD;
    instructions[2].flags = TAC_FLAG_NONE;
    instructions[2].result = TAC_MAKE_TEMP(3);
    instructions[2].operand1 = TAC_MAKE_TEMP(1);
    instructions[2].operand2 = TAC_MAKE_TEMP(2);
    
    // Store all STAS-generated instructions
    for (int i = 0; i < 3; i++) {
        TACIdx_t idx = tacstore_add(&instructions[i]);
        TEST_ASSERT_TRUE(idx > 0);
        
        // Verify each stored instruction structure
        TACInstruction retrieved = tacstore_get(idx);
        TEST_ASSERT_EQUAL(instructions[i].opcode, retrieved.opcode);
        TEST_ASSERT_EQUAL(instructions[i].result.type, retrieved.result.type);
    }
    
    // NOW VALIDATE WITH TAC ENGINE - The critical part!
    tac_engine_config_t config = tac_engine_default_config();
    tac_engine_t* engine = tac_engine_create(&config);
    TEST_ASSERT_NOT_NULL(engine);
    
    // Load STAS-generated TAC sequence into engine
    tac_engine_error_t result = tac_engine_load_code(engine, instructions, 3);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Execute STAS-generated TAC in engine
    result = tac_engine_run(engine);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Validate that TAC engine executed STAS arithmetic correctly
    tac_value_t value1, value2, value3;
    
    result = tac_engine_get_temp(engine, 1, &value1);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL(15, value1.data.i32);
    
    result = tac_engine_get_temp(engine, 2, &value2);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL(27, value2.data.i32);
    
    result = tac_engine_get_temp(engine, 3, &value3);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL(42, value3.data.i32);  // 15 + 27 = 42
    
    tac_engine_destroy(engine);
    tacstore_close();
}

void test_stas_control_flow_validation(void) {
    // Test STAS TAC generation for control flow AND validate with TAC engine
    TEST_ASSERT_EQUAL(1, tacstore_init("test_control_flow.tac"));
    
    // Generate STAS TAC for comparison: t3 = (t1 > t2)
    TACInstruction instructions[3];
    
    // t1 = 7
    instructions[0].opcode = TAC_ASSIGN;
    instructions[0].flags = TAC_FLAG_NONE;
    instructions[0].result = TAC_MAKE_TEMP(1);
    instructions[0].operand1 = TAC_MAKE_IMMEDIATE(7);
    instructions[0].operand2 = TAC_OPERAND_NONE;
    
    // t2 = 5
    instructions[1].opcode = TAC_ASSIGN;
    instructions[1].flags = TAC_FLAG_NONE;
    instructions[1].result = TAC_MAKE_TEMP(2);
    instructions[1].operand1 = TAC_MAKE_IMMEDIATE(5);
    instructions[1].operand2 = TAC_OPERAND_NONE;
    
    // t3 = t1 > t2 (comparison operation: 7 > 5 should be true/1)
    instructions[2].opcode = TAC_GT;
    instructions[2].flags = TAC_FLAG_NONE;
    instructions[2].result = TAC_MAKE_TEMP(3);
    instructions[2].operand1 = TAC_MAKE_TEMP(1);
    instructions[2].operand2 = TAC_MAKE_TEMP(2);
    
    // Store STAS-generated instructions
    for (int i = 0; i < 3; i++) {
        TACIdx_t idx = tacstore_add(&instructions[i]);
        TEST_ASSERT_TRUE(idx > 0);
        
        // Verify stored instruction structure
        TACInstruction retrieved = tacstore_get(idx);
        TEST_ASSERT_EQUAL(instructions[i].opcode, retrieved.opcode);
        TEST_ASSERT_EQUAL(instructions[i].result.type, retrieved.result.type);
    }
    
    // Verify the comparison instruction specifically
    TACInstruction comp_instr = tacstore_get(3);
    TEST_ASSERT_EQUAL(TAC_GT, comp_instr.opcode);
    TEST_ASSERT_EQUAL(TAC_OP_TEMP, comp_instr.result.type);
    TEST_ASSERT_EQUAL(3, comp_instr.result.data.variable.id);
    TEST_ASSERT_EQUAL(TAC_OP_TEMP, comp_instr.operand1.type);
    TEST_ASSERT_EQUAL(1, comp_instr.operand1.data.variable.id);
    TEST_ASSERT_EQUAL(TAC_OP_TEMP, comp_instr.operand2.type);
    TEST_ASSERT_EQUAL(2, comp_instr.operand2.data.variable.id);
    
    // NOW VALIDATE WITH TAC ENGINE - The critical validation step!
    tac_engine_config_t config = tac_engine_default_config();
    tac_engine_t* engine = tac_engine_create(&config);
    TEST_ASSERT_NOT_NULL(engine);
    
    // Load STAS-generated TAC into engine
    tac_engine_error_t result = tac_engine_load_code(engine, instructions, 3);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Execute STAS-generated TAC in engine
    result = tac_engine_run(engine);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    
    // Validate that TAC engine executed STAS comparison correctly
    tac_value_t value1, value2, value3;
    
    result = tac_engine_get_temp(engine, 1, &value1);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL(7, value1.data.i32);
    
    result = tac_engine_get_temp(engine, 2, &value2);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_EQUAL(5, value2.data.i32);
    
    result = tac_engine_get_temp(engine, 3, &value3);
    TEST_ASSERT_EQUAL(TAC_ENGINE_OK, result);
    TEST_ASSERT_NOT_EQUAL(0, value3.data.i32);  // 7 > 5 should be true (non-zero)
    
    tac_engine_destroy(engine);
    tacstore_close();
}

//============================================================================//
// TEST RUNNER
//============================================================================//

void run_stas_tac_generator_tests(void) {
    RUN_TEST(test_stas_tac_basic_instruction_generation);
    RUN_TEST(test_stas_tac_store_functionality);
    RUN_TEST(test_stas_tac_generator_with_engine_validation);
    RUN_TEST(test_stas_arithmetic_expression_validation);
    RUN_TEST(test_stas_control_flow_validation);
}
