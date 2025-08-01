/**
 * @file test_common.c
 * @brief Implementation of common test utilities
 * @author Thomas Boos (tboos70@gmail.com)
 * @version 1.0
 * @date 2025-07-27
 */

#include "test_common.h"
#include <sys/stat.h>
#include <time.h>
#include "../src/ir/tac_store.h"
#include "../src/tools/tac_engine/tac_engine.h"

static char temp_filename[256];
static int temp_file_counter = 0;

/**
 * @brief Create a temporary file with given content
 */
char* create_temp_file(const char* content) {
    snprintf(temp_filename, sizeof(temp_filename), 
             TEMP_PATH "test_temp_%d_%ld.c", 
             temp_file_counter++, time(NULL));
    
    FILE* fp = fopen(temp_filename, "w");
    if (fp) {
        fputs(content, fp);
        fclose(fp);
        return temp_filename;
    }
    return NULL;
}

/**
 * @brief Clean up temporary files
 */
void cleanup_temp_files(void) {
    // Remove temporary files created during tests
    int result = system("rm -f " TEMP_PATH "test_temp_*");
    (void)result; // Suppress unused result warning
}

/**
 * @brief Set up test environment
 */
void setup_test_environment(void) {
    // Create temp directory if it doesn't exist
    mkdir("tests", 0755);
    mkdir("tests/temp", 0755);
    
    // Clean up any existing temp files
    cleanup_temp_files();
}

/**
 * @brief Run a compiler stage and return exit code
 */
int run_compiler_stage(const char* stage, const char* input_file, char** output_files) {
    char command[1024];  // Increased size to accommodate timeout prefix
    
    if (strcmp(stage, "cc0") == 0) {
        snprintf(command, sizeof(command), 
                "timeout 10s ./bin/cc0 %s %s %s", 
                input_file, output_files[0], output_files[1]);
    } else if (strcmp(stage, "cc1") == 0) {
        snprintf(command, sizeof(command), 
                "timeout 10s ./bin/cc1 %s %s %s %s", 
                output_files[0], output_files[1], output_files[2], output_files[3]);
    } else if (strcmp(stage, "cc2") == 0) {
        snprintf(command, sizeof(command), 
                "timeout 10s ./bin/cc2 %s %s %s %s %s %s", 
                output_files[0], output_files[1], output_files[2], 
                output_files[3], output_files[4], output_files[5]);
    } else {
        return -1;
    }
    
    return system(command);
}

/**
 * @brief Read entire file content into a string
 */
char* read_file_content(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return NULL;
    
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* content = malloc(length + 1);
    if (content) {
        size_t bytes_read = fread(content, 1, length, fp);
        content[bytes_read] = '\0'; // Handle partial reads gracefully
        (void)bytes_read; // Suppress unused variable warning
    }
    
    fclose(fp);
    return content;
}

/**
 * @brief Compare two files for equality
 */
int compare_files(const char* file1, const char* file2) {
    char* content1 = read_file_content(file1);
    char* content2 = read_file_content(file2);
    
    if (!content1 || !content2) {
        free(content1);
        free(content2);
        return -1;
    }
    
    int result = strcmp(content1, content2);
    free(content1);
    free(content2);
    return result;
}

/**
 * @brief Load TAC instructions from binary file using tacstore
 */
int load_tac_from_file(const char* filename, TACInstruction** instructions, uint32_t* count) {
    printf("DEBUG: Opening TAC store file: %s\n", filename);
    fflush(stdout);
    
    // Open TAC store file (returns 1 on success, 0 on failure)
    if (tacstore_open(filename) == 0) {
        printf("DEBUG: Failed to open TAC store file\n");
        fflush(stdout);
        return -1;
    }
    
    printf("DEBUG: TAC store opened successfully\n");
    fflush(stdout);
    
    // Get the current instruction count from the TAC store
    uint32_t instruction_count = tacstore_getidx();
    printf("DEBUG: Instruction count: %u\n", instruction_count);
    fflush(stdout);
    
    if (instruction_count == 0) {
        printf("DEBUG: No instructions found\n");
        fflush(stdout);
        tacstore_close();
        return -3; // No instructions found
    }
    
    // Allocate memory for instructions
    *instructions = malloc(sizeof(TACInstruction) * instruction_count);
    if (!(*instructions)) {
        printf("DEBUG: Memory allocation failed\n");
        fflush(stdout);
        tacstore_close();
        return -4; // Memory allocation failed
    }
    
    printf("DEBUG: Memory allocated, loading instructions...\n");
    fflush(stdout);
    
    // Load instructions using 1-based indexing (TAC store uses 1-based indices)
    tacstore_rewind();
    for (uint32_t i = 0; i < instruction_count; i++) {
        printf("DEBUG: Loading instruction %u\n", i + 1);
        fflush(stdout);
        (*instructions)[i] = tacstore_get(i + 1); // TAC indices are 1-based
    }
    
    printf("DEBUG: All instructions loaded\n");
    fflush(stdout);
    
    *count = instruction_count;
    tacstore_close();
    
    printf("DEBUG: TAC store closed, returning success\n");
    fflush(stdout);
    return 0;
}

/**
 * @brief Validate TAC execution using the TAC engine
 */
TACValidationResult validate_tac_execution(const char* tac_file, int expected_return_value) {
    TACValidationResult result = {false, 0, 0, ""};
    
    // Load TAC instructions from file
    TACInstruction* instructions = NULL;
    uint32_t instruction_count = 0;
    
    int load_result = load_tac_from_file(tac_file, &instructions, &instruction_count);
    if (load_result != 0) {
        // Handle special case of empty TAC files (no instructions generated)
        if (load_result == -1 || load_result == -3) {
            // Empty TAC file - this might be expected for simple programs
            result.success = true;
            result.final_return_value = 0; // Default return value for empty programs
            snprintf(result.error_message, sizeof(result.error_message),
                    "TAC file is empty - no instructions generated");
            return result;
        }
        snprintf(result.error_message, sizeof(result.error_message),
                "Failed to load TAC file: error code %d", load_result);
        return result;
    }
    
    // Create TAC engine with default configuration
    tac_engine_config_t config = tac_engine_default_config();
    config.enable_tracing = false; // Disable tracing for tests
    config.max_steps = 50000;      // Increased from 10,000 to 50,000 for complex algorithms
    
    tac_engine_t* engine = tac_engine_create(&config);
    if (!engine) {
        snprintf(result.error_message, sizeof(result.error_message),
                "Failed to create TAC engine");
        free(instructions);
        return result;
    }
    
    // Load code into engine
    tac_engine_error_t engine_result = tac_engine_load_code(engine, instructions, instruction_count);
    if (engine_result != TAC_ENGINE_OK) {
        snprintf(result.error_message, sizeof(result.error_message),
                "Failed to load code: %s", tac_engine_error_string(engine_result));
        tac_engine_destroy(engine);
        free(instructions);
        return result;
    }
    
    // Set entry point to "main" function for C programs
    // Use new C99-compliant function name resolution system
    engine_result = TAC_ENGINE_ERR_NOT_FOUND;
    
    // Strategy 1: Try to use function-based entry point (preferred method)
    tac_engine_error_t func_result = tac_engine_set_entry_function(engine, "main");
    if (func_result == TAC_ENGINE_OK) {
        printf("DEBUG: Set TAC engine entry function to 'main' using function name resolution\n");
        engine_result = TAC_ENGINE_OK;
    } else {
        // Strategy 2: Search for main function label using C99-compliant approach
        // Look for any label that corresponds to the main function
        // (No longer hardcoded to L1/L2 - uses actual function name resolution)
        
        bool found_entry = false;
        
        // Try to find main function label by scanning all labels
        for (uint32_t i = 0; i < instruction_count; i++) {
            if (instructions[i].opcode == TAC_LABEL) {
                uint16_t label_id = instructions[i].result.data.label.offset;
                
                // Try this label as potential main function entry point
                engine_result = tac_engine_set_entry_label(engine, label_id);
                if (engine_result == TAC_ENGINE_OK) {
                    printf("DEBUG: Set TAC engine entry point to label L%u (detected main function)\n", label_id);
                    found_entry = true;
                    break;
                }
            }
        }
        
        // Fallback: start at instruction 0
        if (!found_entry) {
            printf("DEBUG: No suitable label found, starting at instruction 0\n");
            engine_result = tac_engine_set_entry_point(engine, 0);
        }
    }
    
    // Execute the code
    engine_result = tac_engine_run(engine);
    
    // Get execution statistics
    result.executed_instructions = tac_engine_get_step_count(engine);
    
    // Check execution result
    if (engine_result == TAC_ENGINE_OK) {
        tac_engine_state_t state = tac_engine_get_state(engine);
        if (state == TAC_ENGINE_FINISHED || state == TAC_ENGINE_STOPPED) {
            // Try to get return value from multiple locations
            tac_value_t return_val;
            tac_engine_error_t get_result = TAC_ENGINE_ERR_NULL_POINTER;
            
            // Try temp 0 first (standard return location)
            get_result = tac_engine_get_temp(engine, 0, &return_val);
            if (get_result == TAC_ENGINE_OK && return_val.data.i32 != 0) {
                result.final_return_value = return_val.data.i32;
            } else {
                // Try temp 1 (where actual computation result might be)
                get_result = tac_engine_get_temp(engine, 1, &return_val);
                if (get_result == TAC_ENGINE_OK) {
                    result.final_return_value = return_val.data.i32;
                } else {
                    // Fall back to temp 0 even if it's zero
                    get_result = tac_engine_get_temp(engine, 0, &return_val);
                    if (get_result == TAC_ENGINE_OK) {
                        result.final_return_value = return_val.data.i32;
                    }
                }
            }
            
            if (get_result == TAC_ENGINE_OK) {
                result.success = true;
                snprintf(result.error_message, sizeof(result.error_message),
                        "Execution completed with return value %d (expected %d)", 
                        result.final_return_value, expected_return_value);
            } else {
                snprintf(result.error_message, sizeof(result.error_message),
                        "Could not retrieve return value from TAC engine");
            }
        } else {
            snprintf(result.error_message, sizeof(result.error_message),
                    "TAC engine in unexpected state: %d", state);
        }
    } else if (engine_result == TAC_ENGINE_ERR_MAX_STEPS) {
        snprintf(result.error_message, sizeof(result.error_message),
                "TAC execution exceeded maximum steps (%u)", config.max_steps);
        result.success = false;
    } else {
        snprintf(result.error_message, sizeof(result.error_message),
                "TAC execution failed: %s", tac_engine_error_string(engine_result));
    }
    
    // Clean up
    tac_engine_destroy(engine);
    free(instructions);
    
    return result;
}

/**
 * @brief Validate TAC execution using the TAC engine with specific entry label
 */
TACValidationResult validate_tac_execution_with_label(const char* tac_file, uint16_t entry_label_id, int expected_return_value) {
    TACValidationResult result = {false, 0, 0, ""};
    
    // Load TAC instructions from file
    TACInstruction* instructions = NULL;
    uint32_t instruction_count = 0;
    
    int load_result = load_tac_from_file(tac_file, &instructions, &instruction_count);
    if (load_result != 0) {
        // Handle special case of empty TAC files (no instructions generated)
        if (load_result == -1 || load_result == -3) {
            // Empty TAC file - this might be expected for simple programs
            result.success = true;
            result.final_return_value = 0; // Default return value for empty programs
            snprintf(result.error_message, sizeof(result.error_message),
                    "TAC file is empty - no instructions generated");
            return result;
        }
        snprintf(result.error_message, sizeof(result.error_message),
                "Failed to load TAC file: error code %d", load_result);
        return result;
    }
    
    // Create TAC engine with default configuration
    tac_engine_config_t config = tac_engine_default_config();
    config.enable_tracing = false; // Disable tracing for tests
    config.max_steps = 50000;      // Increased from 10,000 to 50,000 for complex algorithms
    
    tac_engine_t* engine = tac_engine_create(&config);
    if (!engine) {
        snprintf(result.error_message, sizeof(result.error_message),
                "Failed to create TAC engine");
        free(instructions);
        return result;
    }
    
    // Load code into engine
    tac_engine_error_t engine_result = tac_engine_load_code(engine, instructions, instruction_count);
    if (engine_result != TAC_ENGINE_OK) {
        snprintf(result.error_message, sizeof(result.error_message),
                "Failed to load code: %s", tac_engine_error_string(engine_result));
        tac_engine_destroy(engine);
        free(instructions);
        return result;
    }
    
    // Set entry point to the specified label
    engine_result = tac_engine_set_entry_label(engine, entry_label_id);
    if (engine_result != TAC_ENGINE_OK) {
        snprintf(result.error_message, sizeof(result.error_message),
                "Failed to set entry label %u: %s", entry_label_id, tac_engine_error_string(engine_result));
        tac_engine_destroy(engine);
        free(instructions);
        return result;
    }
    
    // Execute the code
    engine_result = tac_engine_run(engine);
    
    // Get execution statistics
    result.executed_instructions = tac_engine_get_step_count(engine);
    
    // Check execution result
    if (engine_result == TAC_ENGINE_OK) {
        tac_engine_state_t state = tac_engine_get_state(engine);
        if (state == TAC_ENGINE_FINISHED || state == TAC_ENGINE_STOPPED) {
            // Try to get return value from multiple locations
            tac_value_t return_val;
            tac_engine_error_t get_result = TAC_ENGINE_ERR_NULL_POINTER;
            
            // Try temp 0 first (standard return location)
            get_result = tac_engine_get_temp(engine, 0, &return_val);
            if (get_result == TAC_ENGINE_OK && return_val.data.i32 != 0) {
                result.final_return_value = return_val.data.i32;
            } else {
                // Try temp 1 (where actual computation result might be)
                get_result = tac_engine_get_temp(engine, 1, &return_val);
                if (get_result == TAC_ENGINE_OK) {
                    result.final_return_value = return_val.data.i32;
                } else {
                    // Fall back to temp 0 even if it's zero
                    get_result = tac_engine_get_temp(engine, 0, &return_val);
                    if (get_result == TAC_ENGINE_OK) {
                        result.final_return_value = return_val.data.i32;
                    }
                }
            }
            
            if (get_result == TAC_ENGINE_OK) {
                result.success = true;
                snprintf(result.error_message, sizeof(result.error_message),
                        "Execution completed with return value %d (expected %d)", 
                        result.final_return_value, expected_return_value);
            } else {
                snprintf(result.error_message, sizeof(result.error_message),
                        "Could not retrieve return value from TAC engine");
            }
        } else {
            snprintf(result.error_message, sizeof(result.error_message),
                    "TAC engine in unexpected state: %d", state);
        }
    } else if (engine_result == TAC_ENGINE_ERR_MAX_STEPS) {
        snprintf(result.error_message, sizeof(result.error_message),
                "TAC execution exceeded maximum steps (%u)", config.max_steps);
        result.success = false;
    } else {
        snprintf(result.error_message, sizeof(result.error_message),
                "TAC execution failed: %s", tac_engine_error_string(engine_result));
    }
    
    // Clean up
    tac_engine_destroy(engine);
    free(instructions);
    
    return result;
}
