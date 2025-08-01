//============================================================================//
// test_integration_c99_scoping.c - Integration tests for C99 Scoping System
//
// Integration tests that validate the complete C99 scoping system with 
// symbol table integration, function name resolution, and TAC generation.
// Tests end-to-end functionality from parsing to TAC execution.
//============================================================================//

#include "../test_common.h"
#include "../../src/ir/tac_builder.h"
#include "../../src/ir/tac_printer.h"
#include "../../src/storage/symtab.h"
#include "../../src/storage/sstore.h"

//============================================================================//
// TEST FUNCTION PROTOTYPES
//============================================================================//

void test_integration_c99_basic_functionality(void);
void test_integration_c99_function_name_resolution(void);
void run_integration_c99_scoping_tests(void);

//============================================================================//
// INTEGRATION TESTS
//============================================================================//

void test_integration_c99_basic_functionality(void) {
    // Test basic C99 functionality: proper symbol table integration
    
    // Initialize all components
    sstore_init("test_integration_strings.sst");
    symtab_init("test_integration_symbols.sym");
    
    // Create basic symbol table entries
    SymTabEntry main_func = {0};
    main_func.type = SYM_FUNCTION;
    main_func.name = sstore_str("main", 4);
    main_func.scope_depth = 0;  // File scope
    SymIdx_t main_idx = symtab_add(&main_func);
    TEST_ASSERT_TRUE(main_idx > 0);
    
    SymTabEntry x_var = {0};
    x_var.type = SYM_VARIABLE;
    x_var.name = sstore_str("x", 1);
    x_var.scope_depth = 1;  // Function scope
    SymIdx_t x_idx = symtab_add(&x_var);
    TEST_ASSERT_TRUE(x_idx > 0);
    
    // Initialize TAC builder
    TACBuilder builder;
    TEST_ASSERT_EQUAL(1, tac_builder_init(&builder, "test_integration_basic.tac"));
    
    // Verify no errors during initialization
    TEST_ASSERT_EQUAL(0, builder.error_count);
    
    // Verify function table was populated during initialization
    TEST_ASSERT_TRUE(builder.function_table.count > 0);
    
    // Export function table for name resolution
    tac_builder_export_function_table(&builder);
    
    // Cleanup
    tac_builder_cleanup(&builder);
    symtab_close();
    sstore_close();
}

void test_integration_c99_function_name_resolution(void) {
    // Test function name resolution system works end-to-end
    
    // Initialize components
    sstore_init("test_integration_func_strings.sst");
    symtab_init("test_integration_func_symbols.sym");
    
    // Create multiple function symbols
    SymTabEntry add_func = {0};
    add_func.type = SYM_FUNCTION;
    add_func.name = sstore_str("add", 3);
    add_func.scope_depth = 0;
    symtab_add(&add_func);
    
    SymTabEntry main_func = {0};
    main_func.type = SYM_FUNCTION;
    main_func.name = sstore_str("main", 4);
    main_func.scope_depth = 0;
    symtab_add(&main_func);
    
    // Initialize TAC builder
    TACBuilder builder;
    TEST_ASSERT_EQUAL(1, tac_builder_init(&builder, "test_integration_func.tac"));
    
    // Verify function table contains both functions
    TEST_ASSERT_EQUAL(2, builder.function_table.count);
    
    // Verify function names are loaded correctly
    bool found_add = false, found_main = false;
    for (uint32_t i = 0; i < builder.function_table.count; i++) {
        if (builder.function_table.function_names[i]) {
            if (strcmp(builder.function_table.function_names[i], "add") == 0) found_add = true;
            if (strcmp(builder.function_table.function_names[i], "main") == 0) found_main = true;
        }
    }
    TEST_ASSERT_TRUE(found_add);
    TEST_ASSERT_TRUE(found_main);
    
    // Export function table
    tac_builder_export_function_table(&builder);
    
    // Cleanup  
    tac_builder_cleanup(&builder);
    symtab_close();
    sstore_close();
}

//============================================================================//
// TEST RUNNER
//============================================================================//

void run_integration_c99_scoping_tests(void) {
    RUN_TEST(test_integration_c99_basic_functionality);
    RUN_TEST(test_integration_c99_function_name_resolution);
}
