# TAC Engine Symbol Table Integration Plan

**Author:** GitHub Copilot  
**Date:** August 3, 2025  
**Purpose:** Design and implement symbol table integration for TAC engine  
**Issue:** TAC engine cannot properly resolve variables without symbol table access

---

## 🚨 **Current Problem**

The TAC engine currently handles `TAC_OP_VAR` operands as simple array indices:

```c
case TAC_OP_VAR:
    if (operand->data.variable.id >= engine->config.max_variables) {
        return TAC_ENGINE_ERR_INVALID_OPERAND;
    }
    *value = engine->variables[operand->data.variable.id];
    break;
```

**This approach fails because:**
1. **No name resolution** - Cannot debug or trace variable names
2. **No type information** - Cannot handle different data types properly  
3. **No scope awareness** - Cannot distinguish global vs local variables
4. **No validation** - Cannot verify if variable exists or is initialized

---

## 🎯 **Solution Architecture**

### **Phase 1: Add Symbol Table Access to TAC Engine**

#### **1.1 Extend TAC Engine Configuration**

**File:** `src/tools/tac_engine/tac_engine.h`

```c
/**
 * @brief Engine configuration options
 */
typedef struct tac_engine_config {
    uint32_t max_temporaries;     // Maximum temporary variables (default: 1024)
    uint32_t max_variables;       // Maximum named variables (default: 1024)
    uint32_t max_memory_size;     // Virtual memory size in bytes (default: 64KB)
    uint32_t max_call_depth;      // Maximum function call depth (default: 256)
    uint32_t max_steps;           // Maximum execution steps (default: 1M)
    bool enable_tracing;          // Enable instruction tracing
    bool enable_bounds_check;     // Enable array bounds checking
    bool enable_type_check;       // Enable type checking
    
    // NEW: Symbol table integration
    const char* symtab_file;      // Path to symbol table file (optional)
    const char* sstore_file;      // Path to string store file (optional)
    bool enable_symbol_resolution; // Enable symbol name resolution for debugging
} tac_engine_config_t;
```

#### **1.2 Add Symbol Table Storage to Engine**

**File:** `src/tools/tac_engine/tac_engine_internal.h`

```c
/**
 * @brief Symbol table integration for TAC engine
 */
typedef struct tac_symbol_context {
    bool loaded;                    // Symbol table loaded flag
    char symtab_filename[256];      // Symbol table file path
    char sstore_filename[256];      // String store file path
    
    // Symbol resolution cache (for performance)
    struct {
        uint16_t symbol_id;         // Cached symbol ID
        char name[64];              // Cached symbol name
        uint8_t type;               // Cached type information
        bool valid;                 // Cache entry valid flag
    } symbol_cache[256];            // LRU cache for symbol resolution
    
    uint32_t cache_hits;            // Performance statistics
    uint32_t cache_misses;
} tac_symbol_context_t;

/**
 * @brief Main engine structure (UPDATED)
 */
struct tac_engine {
    // ... existing fields ...
    
    // NEW: Symbol table integration
    tac_symbol_context_t symbols;
};
```

#### **1.3 Implement Symbol Table Loading**

**File:** `src/tools/tac_engine/tac_engine.c`

```c
/**
 * @brief Load symbol table and string store for variable resolution
 */
static tac_engine_error_t tac_engine_load_symbols(tac_engine_t* engine) {
    if (!engine || !engine->config.symtab_file) {
        return TAC_ENGINE_OK; // Symbol resolution disabled
    }
    
    // Initialize symbol table
    if (symtab_init(engine->config.symtab_file) != 0) {
        snprintf(engine->error_message, sizeof(engine->error_message),
                "Failed to load symbol table from %s", engine->config.symtab_file);
        return TAC_ENGINE_ERR_INVALID_OPERAND;
    }
    
    // Initialize string store if provided
    if (engine->config.sstore_file) {
        if (sstore_init(engine->config.sstore_file) != 0) {
            snprintf(engine->error_message, sizeof(engine->error_message),
                    "Failed to load string store from %s", engine->config.sstore_file);
            return TAC_ENGINE_ERR_INVALID_OPERAND;
        }
    }
    
    // Mark as loaded
    engine->symbols.loaded = true;
    strncpy(engine->symbols.symtab_filename, engine->config.symtab_file, 255);
    if (engine->config.sstore_file) {
        strncpy(engine->symbols.sstore_filename, engine->config.sstore_file, 255);
    }
    
    // Initialize symbol cache
    for (int i = 0; i < 256; i++) {
        engine->symbols.symbol_cache[i].valid = false;
    }
    
    printf("DEBUG: Symbol table integration enabled\n");
    printf("  Symbol table: %s\n", engine->config.symtab_file);
    if (engine->config.sstore_file) {
        printf("  String store: %s\n", engine->config.sstore_file);
    }
    
    return TAC_ENGINE_OK;
}
```

#### **1.4 Implement Symbol Resolution Functions**

```c
/**
 * @brief Resolve symbol ID to name and type information
 */
static tac_engine_error_t tac_resolve_symbol(tac_engine_t* engine, uint16_t symbol_id, 
                                            char* name_out, size_t name_size, uint8_t* type_out) {
    if (!engine->symbols.loaded) {
        snprintf(name_out, name_size, "var_%u", symbol_id);
        *type_out = 0; // Unknown type
        return TAC_ENGINE_OK;
    }
    
    // Check cache first
    uint8_t cache_idx = symbol_id % 256;
    if (engine->symbols.symbol_cache[cache_idx].valid && 
        engine->symbols.symbol_cache[cache_idx].symbol_id == symbol_id) {
        strncpy(name_out, engine->symbols.symbol_cache[cache_idx].name, name_size - 1);
        name_out[name_size - 1] = '\0';
        *type_out = engine->symbols.symbol_cache[cache_idx].type;
        engine->symbols.cache_hits++;
        return TAC_ENGINE_OK;
    }
    
    // Look up in symbol table
    SymTabEntry entry = symtab_get(symbol_id);
    if (entry.name == 0) {
        snprintf(name_out, name_size, "UNKNOWN_%u", symbol_id);
        *type_out = 0;
        return TAC_ENGINE_ERR_NOT_FOUND;
    }
    
    // Get name from string store
    char* symbol_name = sstore_get(entry.name);
    if (symbol_name) {
        strncpy(name_out, symbol_name, name_size - 1);
        name_out[name_size - 1] = '\0';
    } else {
        snprintf(name_out, name_size, "NONAME_%u", symbol_id);
    }
    
    *type_out = entry.type;
    
    // Update cache
    engine->symbols.symbol_cache[cache_idx].symbol_id = symbol_id;
    strncpy(engine->symbols.symbol_cache[cache_idx].name, name_out, 63);
    engine->symbols.symbol_cache[cache_idx].name[63] = '\0';
    engine->symbols.symbol_cache[cache_idx].type = *type_out;
    engine->symbols.symbol_cache[cache_idx].valid = true;
    engine->symbols.cache_misses++;
    
    return TAC_ENGINE_OK;
}
```

#### **1.5 Update Variable Access Functions**

```c
/**
 * @brief Get operand value with symbol resolution (ENHANCED)
 */
static tac_engine_error_t tac_get_operand_value(tac_engine_t* engine, const TACOperand* operand, tac_value_t* value) {
    if (!engine || !operand || !value) {
        return TAC_ENGINE_ERR_NULL_POINTER;
    }

    switch (operand->type) {
        case TAC_OP_NONE:
            value->type = TAC_VALUE_INT32;
            value->data.i32 = 0;
            break;
            
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
            
        case TAC_OP_VAR: {
            uint16_t var_id = operand->data.variable.id;
            
            // Bounds check
            if (var_id >= engine->config.max_variables) {
                if (engine->config.enable_symbol_resolution) {
                    char var_name[64];
                    uint8_t var_type;
                    tac_resolve_symbol(engine, var_id, var_name, sizeof(var_name), &var_type);
                    snprintf(engine->error_message, sizeof(engine->error_message),
                            "Variable '%s' (ID=%u) exceeds maximum variables (%u)", 
                            var_name, var_id, engine->config.max_variables);
                } else {
                    snprintf(engine->error_message, sizeof(engine->error_message),
                            "Variable ID %u exceeds maximum variables (%u)", 
                            var_id, engine->config.max_variables);
                }
                return TAC_ENGINE_ERR_INVALID_OPERAND;
            }
            
            // Get variable value
            *value = engine->variables[var_id];
            
            // Enhanced debugging with symbol resolution
            if (engine->config.enable_tracing && engine->config.enable_symbol_resolution) {
                char var_name[64];
                uint8_t var_type;
                if (tac_resolve_symbol(engine, var_id, var_name, sizeof(var_name), &var_type) == TAC_ENGINE_OK) {
                    printf("DEBUG: Accessed variable '%s' (ID=%u, value=%d)\n", 
                           var_name, var_id, value->data.i32);
                }
            }
            break;
        }
        
        default:
            return TAC_ENGINE_ERR_INVALID_OPERAND;
    }

    return TAC_ENGINE_OK;
}
```

#### **1.6 Update TAC Engine Creation**

```c
/**
 * @brief Create TAC engine with symbol table integration (ENHANCED)
 */
tac_engine_t* tac_engine_create(const tac_engine_config_t* config) {
    // ... existing validation and allocation ...
    
    // Copy configuration
    engine->config = *config;
    
    // Initialize symbol table integration
    engine->symbols.loaded = false;
    engine->symbols.cache_hits = 0;
    engine->symbols.cache_misses = 0;
    
    // Load symbol table if configured
    if (config->symtab_file) {
        tac_engine_error_t symbol_result = tac_engine_load_symbols(engine);
        if (symbol_result != TAC_ENGINE_OK) {
            printf("Warning: Symbol table loading failed, continuing without symbol resolution\n");
            // Continue execution but disable symbol resolution
            engine->config.enable_symbol_resolution = false;
        }
    }
    
    // ... rest of existing initialization ...
    
    return engine;
}
```

---

## 🧪 **Integration with Test Framework**

### **Update Test Common Functions**

**File:** `tests/test_common.c`

```c
/**
 * @brief Enhanced TAC validation with symbol table support
 */
TACValidationResult validate_tac_execution_with_symbols(const char* tac_file, 
                                                       const char* symtab_file,
                                                       const char* sstore_file,
                                                       int expected_return_value) {
    TACValidationResult result = {0};
    
    // Configure engine with symbol table support
    tac_engine_config_t config = tac_engine_default_config();
    config.enable_symbol_resolution = true;
    config.enable_tracing = true;
    config.symtab_file = symtab_file;
    config.sstore_file = sstore_file;
    
    // Create engine with symbol table
    tac_engine_t* engine = tac_engine_create(&config);
    if (!engine) {
        strcpy(result.error_message, "Failed to create TAC engine with symbol table");
        return result;
    }
    
    // Load and execute TAC as before...
    // ... existing TAC loading and execution code ...
    
    return result;
}

/**
 * @brief Update existing validation function to support symbols
 */
TACValidationResult validate_tac_execution_with_main(const char* tac_file, int expected_return_value) {
    // Try to find corresponding symbol table and string store files
    char symtab_file[512];
    char sstore_file[512];
    
    // Derive symtab file name (replace .tac.out with .symtab.out)
    strncpy(symtab_file, tac_file, sizeof(symtab_file) - 1);
    char* ext = strstr(symtab_file, ".tac.out");
    if (ext) {
        strcpy(ext, ".symtab.out");
    } else {
        snprintf(symtab_file, sizeof(symtab_file), "%s.symtab.out", tac_file);
    }
    
    // Derive sstore file name
    strncpy(sstore_file, tac_file, sizeof(sstore_file) - 1);
    ext = strstr(sstore_file, ".tac.out");
    if (ext) {
        strcpy(ext, ".sstore.out");
    } else {
        snprintf(sstore_file, sizeof(sstore_file), "%s.sstore.out", tac_file);
    }
    
    // Check if symbol files exist
    FILE* test_symtab = fopen(symtab_file, "rb");
    FILE* test_sstore = fopen(sstore_file, "rb");
    
    if (test_symtab && test_sstore) {
        fclose(test_symtab);
        fclose(test_sstore);
        printf("DEBUG: Using symbol table: %s\n", symtab_file);
        printf("DEBUG: Using string store: %s\n", sstore_file);
        return validate_tac_execution_with_symbols(tac_file, symtab_file, sstore_file, expected_return_value);
    } else {
        // Fallback to existing implementation without symbols
        if (test_symtab) fclose(test_symtab);
        if (test_sstore) fclose(test_sstore);
        printf("DEBUG: Symbol files not found, using basic TAC execution\n");
        
        // Call existing implementation
        uint16_t main_label = extract_main_label_from_tac_file("tests/temp/tac.out");
        return validate_tac_execution_with_label(tac_file, main_label, expected_return_value);
    }
}
```

---

## 🔧 **Implementation Steps**

### **Step 1: Core Integration (Week 1)**
1. Add symbol table fields to `tac_engine_config_t` and `tac_engine_t`
2. Implement `tac_engine_load_symbols()` function
3. Implement `tac_resolve_symbol()` function
4. Update variable access functions with symbol resolution

### **Step 2: Enhanced Debugging (Week 1)**
1. Update instruction tracing to show variable names
2. Add symbol resolution cache for performance
3. Update error messages with variable names
4. Add symbol table statistics and diagnostics

### **Step 3: Test Framework Integration (Week 2)**
1. Update `validate_tac_execution_with_main()` to auto-detect symbol files
2. Add `validate_tac_execution_with_symbols()` function
3. Update existing tests to use symbol-aware validation
4. Add symbol table validation tests

### **Step 4: Validation and Testing (Week 2)**
1. Test with existing TAC files that have symbol tables
2. Verify variable name resolution works correctly
3. Test performance impact of symbol resolution
4. Add comprehensive test cases for edge conditions

---

## 📊 **Expected Benefits**

### **Before Symbol Integration:**
```
TAC Execution Debug Output:
Step 1: t1 = 5
Step 2: t2 = var_12 add t1  // ❌ No idea what var_12 is
Step 3: var_15 = t2         // ❌ No idea what var_15 is
```

### **After Symbol Integration:**
```
TAC Execution Debug Output:
Step 1: t1 = 5
Step 2: t2 = result add t1       // ✅ Shows actual variable name
Step 3: factorial_n = t2         // ✅ Shows actual variable name
Symbol Resolution Stats: 45 hits, 3 misses (93.7% hit rate)
```

### **Enhanced Error Messages:**
```
Before: "Variable ID 67 exceeds maximum variables (64)"
After:  "Variable 'global_array_ptr' (ID=67) exceeds maximum variables (64)"
```

---

## 🚀 **Implementation Priority**

**Immediate (This Week):**
1. Add symbol table loading to TAC engine
2. Update variable resolution with symbol names
3. Test with existing factorial and distance_squared tests

**Next Week:**
1. Full test framework integration
2. Performance optimization with caching
3. Comprehensive error handling and edge cases

This integration will make the TAC engine **significantly more debuggable** and provide the **proper semantic context** needed for reliable test execution. The symbol table access is **essential** for understanding what variables actually represent during TAC execution.
