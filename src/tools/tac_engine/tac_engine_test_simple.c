/**
 * @file tac_engine_test.c
 * @brief Simple test for TAC Engine - MINIMAL VERSION
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 */

#include "tac_engine.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("TAC Engine Test - Basic Compilation Check\n");
    printf("=========================================\n");
    
    // Test basic configuration creation
    tac_engine_config_t config = {
        .max_temporaries = 100,
        .max_variables = 100,
        .max_memory_size = 64 * 1024,
        .max_call_depth = 32,
        .max_steps = 10000,
        .enable_tracing = false,
        .enable_bounds_check = true,
        .enable_type_check = true
    };
    
    printf("Configuration initialized\n");
    
    // Test engine creation
    tac_engine_t* engine = tac_engine_create(&config);
    if (!engine) {
        printf("ERROR: Failed to create engine\n");
        return 1;
    }
    
    printf("Engine created successfully\n");
    
    // Test basic state queries
    tac_engine_state_t state = tac_engine_get_state(engine);
    printf("Engine state: %d\n", state);
    
    tac_engine_error_t last_error = tac_engine_get_last_error(engine);
    printf("Last error: %d\n", last_error);
    
    uint32_t pc = tac_engine_get_pc(engine);
    printf("Program counter: %u\n", pc);
    
    const char* error_string = tac_engine_error_string(last_error);
    printf("Error string: %s\n", error_string);
    
    // Test basic operations
    tac_engine_error_t result;
    
    result = tac_engine_reset(engine);
    printf("Reset result: %d\n", result);
    
    result = tac_engine_stop(engine);
    printf("Stop result: %d\n", result);
    
    // Test cleanup
    tac_engine_destroy(engine);
    printf("Engine destroyed successfully\n");
    
    printf("\nAll basic tests passed!\n");
    return 0;
}
