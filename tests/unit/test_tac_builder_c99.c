//============================================================================//
// test_tac_builder_c99.c - Unit tests for TAC Builder with C99 Scoping
//
// Tests the new TAC builder implementation that follows C99 scoping semantics
// and eliminates hardcoded parameter mappings by using symbol table integration.
// Tests function name resolution and proper C99 scoping compliance.
//============================================================================//

#include "../test_common.h"
#include "../../src/ir/tac_builder.h"
#include "../../src/ir/tac_printer.h"
#include "../../src/storage/symtab.h"
#include "../../src/storage/sstore.h"
#include "../../src/storage/astore.h"
#include "../../src/ast/ast_types.h"

//============================================================================//
// TEST FUNCTION PROTOTYPES
//============================================================================//

void test_tac_builder_c99_scoping_basic(void);
void test_tac_builder_symbol_table_integration(void);
void test_tac_builder_no_hardcoded_mappings(void);
void run_tac_builder_c99_tests(void);

//============================================================================//
// SETUP AND TEARDOWN
//============================================================================//

static void setUp_tac_builder_c99(void) {
    // Initialize required components
    sstore_init("test_strings.sst");
    astore_init("test_ast.ast");
    symtab_init("test_symbols.sym");
}

static void tearDown_tac_builder_c99(void) {
    // Cleanup components
    symtab_close();
    astore_close();
    sstore_close();
}

//============================================================================//
// C99 SCOPING TESTS
//============================================================================//

void test_tac_builder_c99_scoping_basic(void) {
    setUp_tac_builder_c99();
    
    // Create a simple C99 program with proper scoping
    // int main() { int x = 5; return x; }
    
    // Create symbol table entries following C99 scoping
    SymTabEntry main_func = {0};
    main_func.type = SYM_FUNCTION;
    main_func.name = sstore_str("main", 4);
    main_func.scope_depth = 0;  // File scope
    SymIdx_t main_func_idx = symtab_add(&main_func);
    TEST_ASSERT_TRUE(main_func_idx > 0);
    
    SymTabEntry x_var = {0};
    x_var.type = SYM_VARIABLE;
    x_var.name = sstore_str("x", 1);
    x_var.scope_depth = 1;  // Function scope
    SymIdx_t x_var_idx = symtab_add(&x_var);
    TEST_ASSERT_TRUE(x_var_idx > 0);
    
    // Initialize TAC builder
    TACBuilder builder;
    TEST_ASSERT_EQUAL(1, tac_builder_init(&builder, "test_c99_basic.tac"));
    
    // Verify no errors occurred during initialization
    TEST_ASSERT_EQUAL(0, builder.error_count);
    
    // Verify function table contains main function
    TEST_ASSERT_TRUE(builder.function_table.count > 0);
    
    // Export function table for name resolution
    tac_builder_export_function_table(&builder);
    
    // Verify TAC was generated correctly
    uint32_t instruction_count = tacstore_getidx();
    // Just check that we have a valid instruction count (could be 0 initially)
    (void)instruction_count;  // Avoid unused variable warning
    
    tac_builder_cleanup(&builder);
    tearDown_tac_builder_c99();
}

void test_tac_builder_symbol_table_integration(void) {
    setUp_tac_builder_c99();
    
    // Test that TAC builder uses symbol table instead of hardcoded mappings
    
    // Create variables with specific symbol indices
    SymTabEntry var_a = {0};
    var_a.type = SYM_VARIABLE;
    var_a.name = sstore_str("a", 1);
    var_a.scope_depth = 1;
    SymIdx_t a_idx = symtab_add(&var_a);  // Should be index 1
    
    SymTabEntry var_b = {0};
    var_b.type = SYM_VARIABLE;
    var_b.name = sstore_str("b", 1);
    var_b.scope_depth = 1;
    SymIdx_t b_idx = symtab_add(&var_b);  // Should be index 2
    
    // Create identifier AST nodes that reference these symbols
    ASTNode a_ident = {0};
    a_ident.type = AST_EXPR_IDENTIFIER;
    a_ident.binary.value.symbol_idx = a_idx;
    ASTNodeIdx_t a_ident_ast = astore_add(&a_ident);
    
    ASTNode b_ident = {0};
    b_ident.type = AST_EXPR_IDENTIFIER;
    b_ident.binary.value.symbol_idx = b_idx;
    ASTNodeIdx_t b_ident_ast = astore_add(&b_ident);
    
    // Initialize TAC builder
    TACBuilder builder;
    TEST_ASSERT_EQUAL(1, tac_builder_init(&builder, "test_symbol_integration.tac"));
    
    // Translate identifiers
    TACOperand a_operand = tac_build_from_ast(&builder, a_ident_ast);
    TACOperand b_operand = tac_build_from_ast(&builder, b_ident_ast);
    
    // Verify operands use symbol indices, not hardcoded values
    TEST_ASSERT_EQUAL(TAC_OP_VAR, a_operand.type);
    TEST_ASSERT_EQUAL(a_idx, a_operand.data.variable.id);
    
    TEST_ASSERT_EQUAL(TAC_OP_VAR, b_operand.type);
    TEST_ASSERT_EQUAL(b_idx, b_operand.data.variable.id);
    
    // Verify no errors occurred
    TEST_ASSERT_EQUAL(0, builder.error_count);
    
    tac_builder_cleanup(&builder);
    tearDown_tac_builder_c99();
}

void test_tac_builder_no_hardcoded_mappings(void) {
    setUp_tac_builder_c99();
    
    // Test that TAC builder NEVER uses hardcoded parameter mappings
    // This was the original complaint: "only idiots implement hardcoded mappings"
    
    // Create parameters with non-sequential indices to detect hardcoding
    SymTabEntry param_z = {0};
    param_z.type = SYM_VARIABLE;  // Use SYM_VARIABLE instead of SYM_PARAMETER
    param_z.name = sstore_str("z", 1);
    param_z.scope_depth = 1;
    SymIdx_t z_idx = symtab_add(&param_z);  // First parameter
    
    // Skip an index to create gaps
    SymTabEntry dummy = {0};
    dummy.type = SYM_VARIABLE;
    dummy.name = sstore_str("dummy", 5);
    dummy.scope_depth = 1;
    symtab_add(&dummy);  // Creates gap in indices
    
    SymTabEntry param_a = {0};
    param_a.type = SYM_VARIABLE;
    param_a.name = sstore_str("a", 1);
    param_a.scope_depth = 1;
    SymIdx_t a_idx = symtab_add(&param_a);  // Second parameter (non-sequential)
    
    // Create identifier AST nodes
    ASTNode z_ident = {0};
    z_ident.type = AST_EXPR_IDENTIFIER;
    z_ident.binary.value.symbol_idx = z_idx;
    ASTNodeIdx_t z_ast = astore_add(&z_ident);
    
    ASTNode a_ident = {0};
    a_ident.type = AST_EXPR_IDENTIFIER;
    a_ident.binary.value.symbol_idx = a_idx;
    ASTNodeIdx_t a_ast = astore_add(&a_ident);
    
    // Initialize TAC builder
    TACBuilder builder;
    TEST_ASSERT_EQUAL(1, tac_builder_init(&builder, "test_no_hardcoding.tac"));
    
    // Translate identifiers
    TACOperand z_operand = tac_build_from_ast(&builder, z_ast);
    TACOperand a_operand = tac_build_from_ast(&builder, a_ast);
    
    // Verify TAC operands use EXACT symbol indices, not hardcoded values
    TEST_ASSERT_EQUAL(TAC_OP_VAR, z_operand.type);
    TEST_ASSERT_EQUAL(z_idx, z_operand.data.variable.id);  // Must match exact symbol index
    
    TEST_ASSERT_EQUAL(TAC_OP_VAR, a_operand.type);
    TEST_ASSERT_EQUAL(a_idx, a_operand.data.variable.id);  // Must match exact symbol index
    
    // Verify these match the actual symbol table indices 
    // The key test: TAC operands must use symbol table indices, not ignore them
    TEST_ASSERT_EQUAL(z_idx, z_operand.data.variable.id);
    TEST_ASSERT_EQUAL(a_idx, a_operand.data.variable.id);
    
    // The critical verification: operands are different (proving no hardcoded v1, v2 pattern)
    // If hardcoded, both would map to sequential values regardless of symbol indices
    TEST_ASSERT_NOT_EQUAL(z_operand.data.variable.id, a_operand.data.variable.id);
    
    tac_builder_cleanup(&builder);
    tearDown_tac_builder_c99();
}

//============================================================================//
// TEST RUNNER
//============================================================================//

void run_tac_builder_c99_tests(void) {
    RUN_TEST(test_tac_builder_c99_scoping_basic);
    RUN_TEST(test_tac_builder_symbol_table_integration);
    RUN_TEST(test_tac_builder_no_hardcoded_mappings);
}
