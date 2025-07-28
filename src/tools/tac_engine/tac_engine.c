/**
 * @file tac_engine.c
 * @brief TAC Engine implementation - Simplified working version
 * @author STCC1 Project
 * @version 1.0
 * @date 2025-07-28
 * @copyright Copyright (c) 2025
 */

#include "tac_engine.h"
#include "tac_engine_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

// =============================================================================
// LIFECYCLE MANAGEMENT
// =============================================================================

tac_engine_t* tac_engine_create(const tac_engine_config_t* config) {
    if (!config) {
        return NULL;
    }

    // Validate configuration - reject invalid values
    if (config->max_temporaries == 0 || 
        config->max_variables == 0 || 
        config->max_memory_size == 0 ||
        config->max_call_depth == 0) {
        return NULL;
    }

    // Allocate engine structure
    tac_engine_t* engine = calloc(1, sizeof(tac_engine_t));
    if (!engine) {
        return NULL;
    }

    // Copy configuration
    engine->config = *config;
    
    // Initialize state
    engine->state = TAC_ENGINE_STOPPED;
    engine->last_error = TAC_ENGINE_OK;
    engine->pc = 0;
    engine->step_count = 0;
    engine->running = false;

    // Initialize memory manager
    if (tac_memory_init(&engine->memory, config->max_memory_size) != TAC_ENGINE_OK) {
        free(engine);
        return NULL;
    }

    // Allocate variable storage
    engine->temporaries = calloc(config->max_temporaries, sizeof(tac_value_t));
    engine->variables = calloc(config->max_variables, sizeof(tac_value_t));
    
    if (!engine->temporaries || !engine->variables) {
        free(engine->temporaries);
        free(engine->variables);
        free(engine);
        return NULL;
    }

    // Initialize trace buffer if enabled
    if (config->enable_tracing) {
        // Use a fixed size for now
        engine->trace.entries = calloc(1000, sizeof(tac_trace_entry_t));
        if (!engine->trace.entries) {
            free(engine->temporaries);
            free(engine->variables);
            free(engine);
            return NULL;
        }
        engine->trace.capacity = 1000;
        engine->trace.enabled = true;
    }

    return engine;
}

void tac_engine_destroy(tac_engine_t* engine) {
    if (!engine) {
        return;
    }

    // Stop execution
    if (engine->running) {
        engine->running = false;
    }

    // Free instructions
    free(engine->instructions);
    
    // Free variable storage
    free(engine->temporaries);
    free(engine->variables);

    // Free trace buffer
    free(engine->trace.entries);

    // Free call stack
    while (engine->call_stack) {
        tac_stack_frame_t* frame = engine->call_stack;
        engine->call_stack = frame->prev;
        free(frame->locals);
        free(frame);
    }

    // Free hooks
    while (engine->hooks) {
        tac_hook_entry_t* hook = engine->hooks;
        engine->hooks = hook->next;
        free(hook);
    }

    // Free breakpoints
    while (engine->breakpoints) {
        tac_breakpoint_t* bp = engine->breakpoints;
        engine->breakpoints = bp->next;
        free(bp);
    }

    free(engine);
}

// =============================================================================
// STUB IMPLEMENTATIONS FOR INTERNAL FUNCTIONS
// =============================================================================

tac_engine_error_t tac_eval_operand(tac_engine_t* engine,
                                   const TACOperand* operand,
                                   tac_value_t* value) {
    if (!engine || !operand || !value) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }

    switch (operand->type) {
        case TAC_OP_IMMEDIATE:
            value->type = TAC_VALUE_INT32;
            value->data.i32 = operand->data.immediate.value;
            break;
            
        case TAC_OP_TEMP:
            if (operand->data.variable.id >= engine->config.max_temporaries) {
                return TAC_ENGINE_ERR_INVALID_OPERAND;
            }
            *value = engine->temporaries[operand->data.variable.id];
            break;
            
        case TAC_OP_VAR:
            if (operand->data.variable.id >= engine->config.max_variables) {
                return TAC_ENGINE_ERR_INVALID_OPERAND;
            }
            *value = engine->variables[operand->data.variable.id];
            break;
            
        default:
            return TAC_ENGINE_ERR_INVALID_OPERAND;
    }

    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_store_operand(tac_engine_t* engine,
                                    const TACOperand* operand,
                                    const tac_value_t* value) {
    if (!engine || !operand || !value) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }

    switch (operand->type) {
        case TAC_OP_TEMP:
            if (operand->data.variable.id >= engine->config.max_temporaries) {
                return TAC_ENGINE_ERR_INVALID_OPERAND;
            }
            engine->temporaries[operand->data.variable.id] = *value;
            break;
            
        case TAC_OP_VAR:
            if (operand->data.variable.id >= engine->config.max_variables) {
                return TAC_ENGINE_ERR_INVALID_OPERAND;
            }
            engine->variables[operand->data.variable.id] = *value;
            break;
            
        default:
            return TAC_ENGINE_ERR_INVALID_OPERAND;
    }

    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_execute_assign(tac_engine_t* engine,
                                     const TACInstruction* instruction) {
    tac_value_t value;
    tac_engine_error_t err = tac_eval_operand(engine, &instruction->operand1, &value);
    if (err != TAC_ENGINE_OK) {
        return err;
    }
    
    return tac_store_operand(engine, &instruction->result, &value);
}

tac_engine_error_t tac_execute_binary_op(tac_engine_t* engine,
                                        const TACInstruction* instruction) {
    // Get operand values
    tac_value_t val1, val2, result_val = {0};
    
    tac_engine_error_t err1 = tac_eval_operand(engine, &instruction->operand1, &val1);
    tac_engine_error_t err2 = tac_eval_operand(engine, &instruction->operand2, &val2);
    
    if (err1 != TAC_ENGINE_OK) return err1;
    if (err2 != TAC_ENGINE_OK) return err2;

    // Perform operation based on opcode
    switch (instruction->opcode) {
        case TAC_ADD:
            if (val1.type == TAC_VALUE_INT32) {
                result_val.data.i32 = val1.data.i32 + val2.data.i32;
                result_val.type = TAC_VALUE_INT32;
            } else if (val1.type == TAC_VALUE_FLOAT) {
                result_val.data.f32 = val1.data.f32 + val2.data.f32;
                result_val.type = TAC_VALUE_FLOAT;
            }
            break;
            
        case TAC_SUB:
            if (val1.type == TAC_VALUE_INT32) {
                result_val.data.i32 = val1.data.i32 - val2.data.i32;
                result_val.type = TAC_VALUE_INT32;
            } else if (val1.type == TAC_VALUE_FLOAT) {
                result_val.data.f32 = val1.data.f32 - val2.data.f32;
                result_val.type = TAC_VALUE_FLOAT;
            }
            break;
            
        case TAC_MUL:
            if (val1.type == TAC_VALUE_INT32) {
                result_val.data.i32 = val1.data.i32 * val2.data.i32;
                result_val.type = TAC_VALUE_INT32;
            } else if (val1.type == TAC_VALUE_FLOAT) {
                result_val.data.f32 = val1.data.f32 * val2.data.f32;
                result_val.type = TAC_VALUE_FLOAT;
            }
            break;
            
        case TAC_DIV:
            if (val1.type == TAC_VALUE_INT32) {
                if (val2.data.i32 == 0) {
                    tac_set_error(engine, TAC_ENGINE_ERR_DIVISION_BY_ZERO,
                                 "Division by zero");
                    return TAC_ENGINE_ERR_DIVISION_BY_ZERO;
                }
                result_val.data.i32 = val1.data.i32 / val2.data.i32;
                result_val.type = TAC_VALUE_INT32;
            } else if (val1.type == TAC_VALUE_FLOAT) {
                if (val2.data.f32 == 0.0f) {
                    tac_set_error(engine, TAC_ENGINE_ERR_DIVISION_BY_ZERO,
                                 "Division by zero");
                    return TAC_ENGINE_ERR_DIVISION_BY_ZERO;
                }
                result_val.data.f32 = val1.data.f32 / val2.data.f32;
                result_val.type = TAC_VALUE_FLOAT;
            }
            break;
            
        case TAC_GT:
            if (val1.type == TAC_VALUE_INT32) {
                result_val.data.i32 = (val1.data.i32 > val2.data.i32) ? 1 : 0;
                result_val.type = TAC_VALUE_INT32;
            } else if (val1.type == TAC_VALUE_FLOAT) {
                result_val.data.i32 = (val1.data.f32 > val2.data.f32) ? 1 : 0;
                result_val.type = TAC_VALUE_INT32;
            }
            break;
            
        case TAC_LT:
            if (val1.type == TAC_VALUE_INT32) {
                result_val.data.i32 = (val1.data.i32 < val2.data.i32) ? 1 : 0;
                result_val.type = TAC_VALUE_INT32;
            } else if (val1.type == TAC_VALUE_FLOAT) {
                result_val.data.i32 = (val1.data.f32 < val2.data.f32) ? 1 : 0;
                result_val.type = TAC_VALUE_INT32;
            }
            break;
            
        case TAC_EQ:
            if (val1.type == TAC_VALUE_INT32) {
                result_val.data.i32 = (val1.data.i32 == val2.data.i32) ? 1 : 0;
                result_val.type = TAC_VALUE_INT32;
            } else if (val1.type == TAC_VALUE_FLOAT) {
                result_val.data.i32 = (val1.data.f32 == val2.data.f32) ? 1 : 0;
                result_val.type = TAC_VALUE_INT32;
            }
            break;
            
        case TAC_NE:
            if (val1.type == TAC_VALUE_INT32) {
                result_val.data.i32 = (val1.data.i32 != val2.data.i32) ? 1 : 0;
                result_val.type = TAC_VALUE_INT32;
            } else if (val1.type == TAC_VALUE_FLOAT) {
                result_val.data.i32 = (val1.data.f32 != val2.data.f32) ? 1 : 0;
                result_val.type = TAC_VALUE_INT32;
            }
            break;
            
        default:
            return TAC_ENGINE_ERR_INVALID_OPCODE;
    }

    // Store result
    return tac_store_operand(engine, &instruction->result, &result_val);
}

tac_engine_error_t tac_execute_jump(tac_engine_t* engine,
                                   const TACInstruction* instruction) {
    // For unconditional jump, target should be in operand1
    if (instruction->operand1.type != TAC_OP_IMMEDIATE) {
        tac_set_error(engine, TAC_ENGINE_ERR_INVALID_OPERAND,
                     "Jump target must be immediate value");
        return TAC_ENGINE_ERR_INVALID_OPERAND;
    }

    uint32_t target = (uint32_t)instruction->operand1.data.immediate.value;
    
    if (target >= engine->instruction_count) {
        tac_set_error(engine, TAC_ENGINE_ERR_INVALID_MEMORY,
                     "Jump target %u out of bounds", target);
        return TAC_ENGINE_ERR_INVALID_MEMORY;
    }

    engine->pc = target;
    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_execute_conditional_jump(tac_engine_t* engine,
                                               const TACInstruction* instruction) {
    // Get condition value
    tac_value_t condition;
    tac_engine_error_t err = tac_eval_operand(engine, &instruction->operand1, &condition);
    if (err != TAC_ENGINE_OK) {
        return err;
    }

    // Evaluate condition
    bool should_jump = false;
    if (condition.type == TAC_VALUE_INT32) {
        should_jump = (instruction->opcode == TAC_IF_FALSE) ? 
                     (condition.data.i32 == 0) : (condition.data.i32 != 0);
    } else if (condition.type == TAC_VALUE_FLOAT) {
        should_jump = (instruction->opcode == TAC_IF_FALSE) ? 
                     (condition.data.f32 == 0.0f) : (condition.data.f32 != 0.0f);
    }

    if (should_jump) {
        return tac_execute_jump(engine, instruction);
    } else {
        engine->pc++; // Fall through
        return TAC_ENGINE_OK;
    }
}

tac_engine_error_t tac_execute_call(tac_engine_t* engine,
                                   const TACInstruction* instruction) {
    // Get call target
    uint32_t target = (uint32_t)instruction->operand1.data.immediate.value;
    
    if (target >= engine->instruction_count) {
        tac_set_error(engine, TAC_ENGINE_ERR_INVALID_MEMORY,
                     "Call target %u out of bounds", target);
        return TAC_ENGINE_ERR_INVALID_MEMORY;
    }

    // Push current PC + 1 as return address
    tac_engine_error_t err = tac_push_frame(engine, engine->pc + 1, 0);
    if (err != TAC_ENGINE_OK) {
        return err;
    }

    // Jump to function
    engine->pc = target;
    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_execute_return(tac_engine_t* engine,
                                     const TACInstruction* instruction) {
    // Handle return value if present
    if (instruction->operand1.type != TAC_OP_NONE) {
        // Store return value in temp 0 for retrieval by tests
        tac_value_t return_value;
        tac_engine_error_t err = tac_eval_operand(engine, &instruction->operand1, &return_value);
        if (err == TAC_ENGINE_OK) {
            engine->temporaries[0] = return_value;
        }
    }
    
    // Check if we're in a function call (have call stack)
    if (engine->call_stack) {
        // Pop return address from call stack
        tac_stack_frame_t* frame = engine->call_stack;
        engine->pc = frame->return_address;
        engine->call_stack = frame->prev;
        engine->call_depth--;

        free(frame->locals);
        free(frame);
        
        return TAC_ENGINE_OK;
    } else {
        // Top-level return - finish program execution
        engine->state = TAC_ENGINE_FINISHED;
        // Set PC to end of program to stop execution
        engine->pc = engine->instruction_count;
        return TAC_ENGINE_OK;
    }
}

tac_engine_error_t tac_push_frame(tac_engine_t* engine,
                                 uint32_t return_address,
                                 uint32_t param_count) {
    if (!engine) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }

    if (engine->call_depth >= engine->config.max_call_depth) {
        return TAC_ENGINE_ERR_STACK_OVERFLOW;
    }

    tac_stack_frame_t* frame = calloc(1, sizeof(tac_stack_frame_t));
    if (!frame) {
        return TAC_ENGINE_ERR_OUT_OF_MEMORY;
    }

    frame->return_address = return_address;
    frame->param_count = param_count;
    frame->locals = calloc(64, sizeof(tac_value_t)); // Fixed size for now
    frame->prev = engine->call_stack;

    if (!frame->locals) {
        free(frame);
        return TAC_ENGINE_ERR_OUT_OF_MEMORY;
    }

    engine->call_stack = frame;
    engine->call_depth++;

    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_validate_operand(tac_engine_t* engine,
                                       const TACOperand* operand) {
    if (!engine || !operand) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }

    switch (operand->type) {
        case TAC_OP_TEMP:
            return (operand->data.variable.id < engine->config.max_temporaries) ? 
                   TAC_ENGINE_OK : TAC_ENGINE_ERR_INVALID_OPERAND;
                   
        case TAC_OP_VAR:
            return (operand->data.variable.id < engine->config.max_variables) ? 
                   TAC_ENGINE_OK : TAC_ENGINE_ERR_INVALID_OPERAND;
                   
        case TAC_OP_IMMEDIATE:
        case TAC_OP_NONE:
            return TAC_ENGINE_OK;
            
        default:
            return TAC_ENGINE_ERR_INVALID_OPERAND;
    }
}

void tac_set_error(tac_engine_t* engine, 
                   tac_engine_error_t error,
                   const char* format, ...) {
    if (!engine) {
        return;
    }
    
    engine->last_error = error;
    engine->error_address = engine->pc;
    
    if (format) {
        va_list args;
        va_start(args, format);
        vsnprintf(engine->error_message, sizeof(engine->error_message), format, args);
        va_end(args);
    } else {
        snprintf(engine->error_message, sizeof(engine->error_message), 
                "Error %d at address %u", error, engine->pc);
    }
}

bool tac_trigger_hooks(tac_engine_t* engine, tac_hook_type_t type, uint32_t address) {
    if (!engine) {
        return false;
    }
    
    tac_hook_entry_t* hook = engine->hooks;
    while (hook) {
        if (hook->enabled && hook->type == type) {
            if (!hook->callback(engine, type, address, hook->user_data)) {
                return false; // Hook requested stop
            }
        }
        hook = hook->next;
    }
    
    return true; // Continue execution
}

bool tac_has_breakpoint(tac_engine_t* engine, uint32_t address) {
    if (!engine) {
        return false;
    }
    
    tac_breakpoint_t* bp = engine->breakpoints;
    while (bp) {
        if (bp->enabled && bp->address == address) {
            return true;
        }
        bp = bp->next;
    }
    
    return false;
}

void tac_add_trace(tac_engine_t* engine,
                  const TACInstruction* instruction,
                  const tac_value_t* before,
                  const tac_value_t* after) {
    if (!engine || !engine->trace.enabled || !engine->trace.entries) {
        return;
    }
    
    // Add trace entry (simplified)
    tac_trace_entry_t* entry = &engine->trace.entries[engine->trace.head];
    entry->step = engine->step_count;
    entry->address = engine->pc;
    if (instruction) entry->instruction = *instruction;
    if (before) entry->result_before = *before;
    if (after) entry->result_after = *after;
    
    engine->trace.head = (engine->trace.head + 1) % engine->trace.capacity;
    if (engine->trace.count < engine->trace.capacity) {
        engine->trace.count++;
    }
}

// =============================================================================
// MINIMAL STUBS FOR PUBLIC API
// =============================================================================

tac_engine_error_t tac_engine_load_code(tac_engine_t* engine,
                                        const TACInstruction* instructions,
                                        uint32_t count) {
    if (!engine || !instructions) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }

    if (engine->state != TAC_ENGINE_STOPPED) {
        return TAC_ENGINE_ERR_INVALID_OPERAND;
    }

    // Free existing instructions
    free(engine->instructions);

    // Allocate and copy new instructions
    engine->instructions = malloc(count * sizeof(TACInstruction));
    if (!engine->instructions) {
        return TAC_ENGINE_ERR_OUT_OF_MEMORY;
    }

    memcpy(engine->instructions, instructions, count * sizeof(TACInstruction));
    engine->instruction_count = count;
    engine->pc = 0;

    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_engine_run(tac_engine_t* engine) {
    if (!engine) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    if (!engine->instructions || engine->instruction_count == 0) {
        return TAC_ENGINE_ERR_INVALID_OPERAND;
    }
    
    // Execute all instructions starting from PC
    engine->state = TAC_ENGINE_RUNNING;
    
    while (engine->pc < engine->instruction_count) {
        const TACInstruction* instruction = &engine->instructions[engine->pc];
        
        // Execute the instruction based on opcode
        tac_engine_error_t err = TAC_ENGINE_OK;
        
        switch (instruction->opcode) {
            case TAC_ASSIGN:
                err = tac_execute_assign(engine, instruction);
                break;
                
            case TAC_ADD:
            case TAC_SUB:
            case TAC_MUL:
            case TAC_DIV:
            case TAC_GT:
            case TAC_LT:
            case TAC_EQ:
            case TAC_NE:
                err = tac_execute_binary_op(engine, instruction);
                break;
                
            case TAC_GOTO:
                err = tac_execute_jump(engine, instruction);
                continue; // Jump handles PC update
                
            case TAC_IF_TRUE:
            case TAC_IF_FALSE:
                err = tac_execute_conditional_jump(engine, instruction);
                continue; // Conditional jump handles PC update
                
            case TAC_CALL:
                err = tac_execute_call(engine, instruction);
                continue; // Call handles PC update
                
            case TAC_RETURN:
                err = tac_execute_return(engine, instruction);
                continue; // Return handles PC update
                
            case TAC_NOP:
                // No operation
                break;
                
            default:
                tac_set_error(engine, TAC_ENGINE_ERR_INVALID_OPCODE,
                             "Unknown opcode: %d", instruction->opcode);
                engine->state = TAC_ENGINE_ERROR;
                return TAC_ENGINE_ERR_INVALID_OPCODE;
        }
        
        if (err != TAC_ENGINE_OK) {
            engine->state = TAC_ENGINE_ERROR;
            return err;
        }
        
        // Advance to next instruction
        engine->pc++;
        engine->step_count++;
    }
    
    engine->state = TAC_ENGINE_FINISHED;
    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_engine_step(tac_engine_t* engine) {
    if (!engine) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    if (!engine->instructions || engine->instruction_count == 0) {
        return TAC_ENGINE_ERR_INVALID_OPERAND;
    }
    
    if (engine->pc >= engine->instruction_count) {
        engine->state = TAC_ENGINE_FINISHED;
        return TAC_ENGINE_OK; // Execution finished normally
    }
    
    const TACInstruction* instruction = &engine->instructions[engine->pc];
    
    // Execute the instruction based on opcode
    tac_engine_error_t err = TAC_ENGINE_OK;
    
    switch (instruction->opcode) {
        case TAC_ASSIGN:
            err = tac_execute_assign(engine, instruction);
            break;
            
        case TAC_ADD:
        case TAC_SUB:
        case TAC_MUL:
        case TAC_DIV:
        case TAC_GT:
        case TAC_LT:
        case TAC_EQ:
        case TAC_NE:
            err = tac_execute_binary_op(engine, instruction);
            break;
            
        case TAC_GOTO:
            err = tac_execute_jump(engine, instruction);
            engine->step_count++;
            return err; // Jump handles PC update
            
        case TAC_IF_TRUE:
        case TAC_IF_FALSE:
            err = tac_execute_conditional_jump(engine, instruction);
            engine->step_count++;
            return err; // Conditional jump handles PC update
            
        case TAC_CALL:
            err = tac_execute_call(engine, instruction);
            engine->step_count++;
            return err; // Call handles PC update
            
        case TAC_RETURN:
            err = tac_execute_return(engine, instruction);
            engine->step_count++;
            return err; // Return handles PC update
            
        case TAC_NOP:
            // No operation
            break;
            
        default:
            tac_set_error(engine, TAC_ENGINE_ERR_INVALID_OPCODE,
                         "Unknown opcode: %d", instruction->opcode);
            engine->state = TAC_ENGINE_ERROR;
            return TAC_ENGINE_ERR_INVALID_OPCODE;
    }
    
    if (err != TAC_ENGINE_OK) {
        engine->state = TAC_ENGINE_ERROR;
        return err;
    }
    
    // Advance to next instruction
    engine->pc++;
    engine->step_count++;
    
    // Check if we've finished
    if (engine->pc >= engine->instruction_count) {
        engine->state = TAC_ENGINE_FINISHED;
        return TAC_ENGINE_OK; // Execution finished normally
    }
    
    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_engine_run_until(tac_engine_t* engine, uint32_t address) {
    if (!engine) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    (void)address; // Unused
    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_engine_stop(tac_engine_t* engine) {
    if (!engine) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    engine->running = false;
    engine->state = TAC_ENGINE_STOPPED;
    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_engine_reset(tac_engine_t* engine) {
    if (!engine) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    engine->pc = 0;
    engine->step_count = 0;
    engine->running = false;
    engine->state = TAC_ENGINE_STOPPED;
    engine->last_error = TAC_ENGINE_OK;
    
    return TAC_ENGINE_OK;
}

tac_engine_state_t tac_engine_get_state(tac_engine_t* engine) {
    return engine ? engine->state : TAC_ENGINE_ERROR;
}

tac_engine_error_t tac_engine_get_last_error(tac_engine_t* engine) {
    return engine ? engine->last_error : TAC_ENGINE_ERR_NULL_POINTER;
}

uint32_t tac_engine_get_pc(tac_engine_t* engine) {
    return engine ? engine->pc : 0;
}

uint32_t tac_engine_get_step_count(tac_engine_t* engine) {
    return engine ? engine->step_count : 0;
}

const char* tac_engine_get_error_message(tac_engine_t* engine) {
    return engine ? engine->error_message : "Invalid engine";
}

const char* tac_engine_error_string(tac_engine_error_t error) {
    switch (error) {
        case TAC_ENGINE_OK: return "No error";
        case TAC_ENGINE_ERR_NULL_POINTER: return "NULL pointer passed";
        case TAC_ENGINE_ERR_INVALID_OPCODE: return "Unknown TAC opcode";
        case TAC_ENGINE_ERR_INVALID_OPERAND: return "Invalid operand type/value";
        case TAC_ENGINE_ERR_OUT_OF_MEMORY: return "Memory allocation failed";
        case TAC_ENGINE_ERR_STACK_OVERFLOW: return "Call stack overflow";
        case TAC_ENGINE_ERR_STACK_UNDERFLOW: return "Stack underflow";
        case TAC_ENGINE_ERR_DIVISION_BY_ZERO: return "Division by zero";
        case TAC_ENGINE_ERR_INVALID_MEMORY: return "Invalid memory access";
        case TAC_ENGINE_ERR_BREAKPOINT: return "Hit breakpoint";
        case TAC_ENGINE_ERR_MAX_STEPS: return "Maximum steps exceeded";
        default: return "Unknown error";
    }
}

tac_engine_config_t tac_engine_default_config(void) {
    tac_engine_config_t config = {
        .max_temporaries = 1024,
        .max_variables = 1024,
        .max_memory_size = 64 * 1024,  // 64KB
        .max_call_depth = 256,
        .max_steps = 1000000,
        .enable_tracing = false,
        .enable_bounds_check = true,
        .enable_type_check = true
    };
    return config;
}

tac_engine_error_t tac_engine_get_stats(tac_engine_t* engine,
                                       uint32_t* steps_executed,
                                       uint32_t* memory_used) {
    if (!engine || !steps_executed || !memory_used) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    *steps_executed = engine->step_count;
    *memory_used = engine->memory.total_allocated;
    
    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_engine_get_temp(tac_engine_t* engine, 
                                       uint16_t temp_id, 
                                       tac_value_t* value) {
    if (!engine || !value) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    if (temp_id >= engine->config.max_temporaries) {
        return TAC_ENGINE_ERR_INVALID_OPERAND;
    }
    
    *value = engine->temporaries[temp_id];
    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_engine_set_temp(tac_engine_t* engine,
                                       uint16_t temp_id,
                                       const tac_value_t* value) {
    if (!engine || !value) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    if (temp_id >= engine->config.max_temporaries) {
        return TAC_ENGINE_ERR_INVALID_OPERAND;
    }
    
    engine->temporaries[temp_id] = *value;
    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_engine_get_var(tac_engine_t* engine,
                                      uint16_t var_id,
                                      tac_value_t* value) {
    if (!engine || !value) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    if (var_id >= engine->config.max_variables) {
        return TAC_ENGINE_ERR_INVALID_OPERAND;
    }
    
    *value = engine->variables[var_id];
    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_engine_set_var(tac_engine_t* engine,
                                      uint16_t var_id,
                                      const tac_value_t* value) {
    if (!engine || !value) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    if (var_id >= engine->config.max_variables) {
        return TAC_ENGINE_ERR_INVALID_OPERAND;
    }
    
    engine->variables[var_id] = *value;
    return TAC_ENGINE_OK;
}

// =============================================================================
// STUB IMPLEMENTATIONS FOR MISSING MEMORY FUNCTIONS
// =============================================================================

tac_engine_error_t tac_memory_init(tac_memory_manager_t* memory, uint32_t max_size) {
    if (!memory) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    memory->blocks = NULL;
    memory->next_address = 0x1000; // Start at 4KB
    memory->total_allocated = 0;
    memory->max_size = max_size;
    
    return TAC_ENGINE_OK;
}

void tac_memory_cleanup(tac_memory_manager_t* memory) {
    if (!memory) {
        return;
    }
    
    // Free all memory blocks
    tac_memory_block_t* block = memory->blocks;
    while (block) {
        tac_memory_block_t* next = block->next;
        free(block->data);
        free(block);
        block = next;
    }
    
    memory->blocks = NULL;
    memory->total_allocated = 0;
}

uint32_t tac_memory_alloc(tac_memory_manager_t* memory, uint32_t size) {
    if (!memory) {
        return 0; // Invalid address
    }
    
    if (memory->total_allocated + size > memory->max_size) {
        return 0; // Out of memory
    }
    
    // Simple linear allocation
    uint32_t address = memory->next_address;
    memory->next_address += size;
    memory->total_allocated += size;
    
    return address;
}

tac_engine_error_t tac_memory_free(tac_memory_manager_t* memory, uint32_t address) {
    if (!memory) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    // Simple stub - in real implementation would free the block
    (void)address;
    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_memory_read(tac_memory_manager_t* memory,
                                  uint32_t address,
                                  void* buffer,
                                  uint32_t size) {
    if (!memory || !buffer) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    // Simple stub - in real implementation would read from virtual memory
    (void)address;
    (void)size;
    memset(buffer, 0, size);
    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_memory_write(tac_memory_manager_t* memory,
                                   uint32_t address,
                                   const void* buffer,
                                   uint32_t size) {
    if (!memory || !buffer) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    // Simple stub - in real implementation would write to virtual memory
    (void)address;
    (void)size;
    return TAC_ENGINE_OK;
}

tac_engine_error_t tac_convert_value(const tac_value_t* from,
                                    tac_value_type_t target_type,
                                    tac_value_t* to) {
    if (!from || !to) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }
    
    to->type = target_type;
    
    // Simple conversion (very basic)
    switch (target_type) {
        case TAC_VALUE_INT32:
            if (from->type == TAC_VALUE_INT32) {
                to->data.i32 = from->data.i32;
            } else if (from->type == TAC_VALUE_FLOAT) {
                to->data.i32 = (int32_t)from->data.f32;
            } else {
                to->data.i32 = 0;
            }
            break;
            
        case TAC_VALUE_FLOAT:
            if (from->type == TAC_VALUE_INT32) {
                to->data.f32 = (float)from->data.i32;
            } else if (from->type == TAC_VALUE_FLOAT) {
                to->data.f32 = from->data.f32;
            } else {
                to->data.f32 = 0.0f;
            }
            break;
            
        default:
            return TAC_ENGINE_ERR_INVALID_OPERAND;
    }
    
    return TAC_ENGINE_OK;
}
