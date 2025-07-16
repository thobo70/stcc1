# Modular Error Handling System

## Overview

The error handling system has been completely restructured to provide modular, stage-specific error handling while maintaining a consistent core infrastructure. This design separates common error handling functionality from stage-dependent specifics.

## Architecture

### Core Components

#### 1. **Error Core (`error_core.h/.c`)**
**Purpose**: Stage-independent error infrastructure
- Universal error types and severity levels
- Source location tracking and formatting
- Error storage and statistics
- Configuration management
- Output formatting and filtering

**Key Features:**
```c
// Initialize with custom configuration
ErrorConfig_t config = error_get_default_config();
config.max_errors = 50;
config.show_suggestions = 1;
error_core_init(&config);

// Report errors with full context
SourceLocation_t location = error_create_location(token_idx);
error_core_report(ERROR_ERROR, ERROR_SYNTAX, &location, 
                 error_code, message, suggestion, stage_name, context);
```

#### 2. **Error Stages (`error_stages.h/.c`)**
**Purpose**: Stage-specific error handling interfaces
- Lexical analysis errors (invalid chars, unterminated strings)
- Syntax analysis errors (missing tokens, unmatched delimiters)
- Semantic analysis errors (undefined symbols, type mismatches)
- Code generation errors (unsupported features, register spills)

**Stage-Specific APIs:**
```c
// Lexical errors
lex_error_invalid_char(token_idx, invalid_char);
lex_error_unterminated_string(token_idx);

// Syntax errors  
syntax_error_missing_token(token_idx, T_SEMICOLON);
syntax_error_unexpected_token(token_idx, expected, found);

// Semantic errors
semantic_error_undefined_symbol(token_idx, "variable_name");
semantic_error_type_mismatch(token_idx, expected_type, found_type, "assignment");
```

#### 3. **Error Recovery (`error_recovery.h`)**
**Purpose**: Intelligent error recovery strategies
- Context-aware recovery suggestions
- Quality assessment of recovery options
- Learning from recovery effectiveness
- Stage-specific recovery patterns

**Recovery Interface:**
```c
RecoveryContext_t context;
recovery_init_context(&context, error_token);
recovery_add_expected_token(&context, T_ID);
recovery_add_sync_token(&context, T_SEMICOLON);

RecoveryResult_t result = recovery_suggest_action(&context);
if (result.confidence_level > 70) {
    recovery_attempt_action(&result);
}
```

## Modular Design Benefits

### 1. **Separation of Concerns**
- **Core**: Handles universal error mechanics (storage, formatting, statistics)
- **Stages**: Handle stage-specific error types and context
- **Recovery**: Handles intelligent error recovery strategies

### 2. **Consistent Interface**
All stages use the same core reporting mechanism:
```c
void stage_error_function(TokenIdx_t token_idx, /* stage-specific params */) {
    // Stage-specific error message generation
    char message[256], suggestion[256];
    create_stage_specific_message(message, suggestion, /* params */);
    
    // Common core reporting
    error_report_with_context(ERROR_ERROR, ERROR_CATEGORY, token_idx,
                             STAGE_ERROR_CODE, message, suggestion,
                             stage_context);
}
```

### 3. **Extensible Design**
Adding new error types or stages requires minimal changes:
```c
// Add new error code
typedef enum {
    NEW_STAGE_ERROR_INVALID_CONSTRUCT = 5000,
    // ... other codes
} NewStageErrorCode_t;

// Add stage-specific handler
void new_stage_error_invalid_construct(TokenIdx_t token_idx, const char* construct) {
    error_report_with_context(ERROR_ERROR, ERROR_NEW_CATEGORY, token_idx,
                             NEW_STAGE_ERROR_INVALID_CONSTRUCT,
                             "Invalid construct", "Check syntax",
                             &new_stage_context);
}
```

## Usage Examples

### Basic Error Reporting
```c
// Initialize error system
error_core_init(NULL);  // Use default config
error_stages_init_all();

// Set current compilation stage
error_set_current_stage("Parser");

// Report stage-specific errors
syntax_error_missing_token(current_token_idx, T_SEMICOLON);
semantic_error_undefined_symbol(token_idx, "undefined_var");

// Check if should abort
if (error_core_should_abort()) {
    error_core_print_summary();
    return 1;
}
```

### Advanced Error Correlation
```c
// Create primary error
CompilerError_t* primary = syntax_error_missing_token(100, T_SEMICOLON);

// Create related cascade error
CompilerError_t* cascade = semantic_error_undefined_symbol(105, "var");

// Link errors for better diagnostics
error_add_related_error(primary, cascade);

// Print error chain
error_print_error_chain(primary);
```

### Custom Error Recovery
```c
// Set up recovery context
RecoveryContext_t recovery_ctx;
recovery_init_context(&recovery_ctx, error_token_idx);
recovery_add_expected_token(&recovery_ctx, T_ID);
recovery_add_sync_token(&recovery_ctx, T_SEMICOLON);

// Get recovery suggestion
RecoveryResult_t recovery = syntax_recovery_missing_semicolon(&recovery_ctx);

if (recovery.action == RECOVERY_ACTION_INSERT_TOKEN) {
    // Insert suggested token and continue parsing
    insert_token(recovery.suggested_token);
    printf("Recovered by inserting %s\n", get_token_name(recovery.suggested_token));
}
```

### Error System Configuration
```c
// Configure error handling behavior
ErrorConfig_t config = error_get_default_config();
config.max_errors = 25;                    // Stop after 25 errors
config.show_suggestions = 1;               // Show fix suggestions
config.show_source_context = 1;            // Show source code context
config.colorize_output = 1;                // Use colored output
config.output_stream = custom_log_file;    // Custom output destination

error_core_init(&config);
```

## Integration with Existing System

### Memory Efficiency
- Error structures use fixed-size allocations where possible
- String references point to existing string storage (`sstore`)
- Token references use existing token indices
- Minimal memory overhead for error tracking

### Token Integration
```c
// Errors automatically extract location from tokens
SourceLocation_t location = error_create_location(token_idx);
// This uses tstore to get file/line information

// Manual location creation for non-token errors
SourceLocation_t location = error_create_location_with_pos("file.c", 42, 10);
```

### Stage Integration
Each compiler stage initializes its error handler:
```c
// In lexer
void lexer_init(void) {
    lex_error_init();
    error_set_current_stage("Lexical Analysis");
}

// In parser  
void parser_init(void) {
    syntax_error_init();
    error_set_current_stage("Syntax Analysis");
}

// In semantic analyzer
void semantic_init(void) {
    semantic_error_init();
    error_set_current_stage("Semantic Analysis");
}
```

## Build Integration

The modular error system integrates with the existing build system:

```makefile
# Enhanced compiler with modular error handling
make enhanced

# Test error handling
make test_enhanced

# Demonstrate error features
make demo_errors

# Profile memory usage with error handling
make profile_enhanced
```

## Error Categories and Codes

### Systematic Error Numbering
- **1000-1999**: Lexical errors
- **2000-2999**: Syntax errors  
- **3000-3999**: Semantic errors
- **4000-4999**: Code generation errors
- **5000+**: Future expansion

### Error Severity Levels
- **INFO**: Informational messages
- **WARNING**: Non-fatal issues
- **ERROR**: Compilation errors  
- **FATAL**: Unrecoverable errors

## Performance Characteristics

### Memory Usage
- **Error Core**: ~1KB static overhead
- **Per Error**: ~128 bytes (including context)
- **Stage Handlers**: ~256 bytes each
- **Total Overhead**: <2KB for typical compilation

### Error Limits
- Default: 50 errors, 100 warnings before abort
- Configurable based on available memory
- Cascade error detection to prevent explosion

## Future Enhancements

1. **Machine-Readable Output**: JSON/XML error formats
2. **Error Clustering**: Group related errors
3. **Fix Suggestions**: Automated code fixes
4. **IDE Integration**: Language server protocol support
5. **Internationalization**: Multi-language error messages

This modular design provides a robust foundation for comprehensive error handling while maintaining the project's low-memory goals.
