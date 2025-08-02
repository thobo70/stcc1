/**
 * @file symtab.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-10
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef SYMTAB_H  // NOLINT
#define SYMTAB_H

#include "sstore.h"

typedef enum {
    SYM_FREE,
    SYM_VARIABLE,
    SYM_FUNCTION,
    SYM_TYPEDEF,
    SYM_LABEL,
    SYM_ENUMERATOR,
    SYM_STRUCT,
    SYM_UNION,
    SYM_ENUM,
    SYM_CONSTANT,
    SYM_UNKNOWN,            // Fixed typo from SYM_UNKOWN
    // C99-specific symbol types
    SYM_VLA_PARAMETER,      // Variable Length Array parameters
    SYM_FLEXIBLE_MEMBER,    // Flexible array members
    SYM_ANONYMOUS_STRUCT,   // Anonymous struct/union members
    SYM_UNIVERSAL_CHAR      // Universal character names
} SymType;

typedef unsigned short SymIdx_t;    // NOLINT
typedef unsigned short TypeIdx_t;   // NOLINT - For type table integration

// C99 symbol attribute flags
#define SYM_FLAG_NONE          0x0000
#define SYM_FLAG_INLINE        0x0001  // inline functions (C99 6.7.4)
#define SYM_FLAG_RESTRICT      0x0002  // restrict pointers (C99 6.7.3)
#define SYM_FLAG_VLA           0x0004  // Variable Length Arrays (C99 6.7.5.2)
#define SYM_FLAG_FLEXIBLE      0x0008  // Flexible array members (C99 6.7.2.1)
#define SYM_FLAG_COMPLEX       0x0010  // Complex types (C99 6.2.5.11)
#define SYM_FLAG_IMAGINARY     0x0020  // Imaginary types (C99 6.2.5.11)
#define SYM_FLAG_VARIADIC      0x0040  // Variadic functions (C99 6.7.5.3)
#define SYM_FLAG_UNIVERSAL_CHAR 0x0080 // Universal character names (C99 6.4.3)
#define SYM_FLAG_DESIGNATED    0x0100  // Designated initializers (C99 6.7.8)
#define SYM_FLAG_COMPOUND_LIT  0x0200  // Compound literals (C99 6.5.2.5)
#define SYM_FLAG_MIXED_DECL    0x0400  // Mixed declarations (C99 6.8.2)
#define SYM_FLAG_CONST         0x0800  // const qualified (C99 6.7.3)
#define SYM_FLAG_VOLATILE      0x1000  // volatile qualified (C99 6.7.3)

// Extended data for C99 features
typedef union SymExtraData {
    struct {
        unsigned short size_expr_idx;  // AST index for VLA size expression
        unsigned char dimensions;      // Number of VLA dimensions
        unsigned char padding;
    } vla;
    struct {
        unsigned short field_count;    // Number of struct/union fields
        unsigned short first_field;    // First field symbol index
    } aggregate;
    struct {
        unsigned short param_count;    // Number of function parameters
        unsigned short first_param;    // First parameter symbol index
    } function;
    unsigned int raw;                  // Raw 32-bit access
} SymExtraData;

typedef struct SymTabEntry {
    SymType type;               // Symbol type (variable, function, etc.)
    sstore_pos_t name;          // Name in string store
    SymIdx_t parent;            // Parent scope (for hierarchical navigation)
    SymIdx_t next, prev;        // Linked list navigation
    SymIdx_t child, sibling;    // Hierarchical navigation
    sstore_pos_t value;         // Additional data (backwards compatibility)
    int line;                   // Declaration line number
    int scope_depth;            // C99 block scope depth (0=file, 1=function, 2+=block)
    
    // C99 enhancements
    unsigned int flags;         // C99 symbol attribute flags (SYM_FLAG_*)
    TypeIdx_t type_idx;         // Detailed type information index
    SymExtraData extra;         // Extended C99-specific data
} SymTabEntry;



int symtab_init(const char *filename);
int symtab_open(const char *filename);
void symtab_close(void);
SymIdx_t symtab_add(SymTabEntry *entry);
SymIdx_t symtab_update(SymIdx_t idx,
                     SymTabEntry *entry);
SymTabEntry symtab_get(SymIdx_t idx);
SymIdx_t symtab_get_count(void);

// C99 convenience functions
SymIdx_t symtab_add_c99_symbol(SymType type, sstore_pos_t name, 
                               int scope_depth, unsigned int flags);
int symtab_set_c99_flags(SymIdx_t idx, unsigned int flags);
int symtab_get_c99_flags(SymIdx_t idx);
int symtab_is_c99_feature(SymIdx_t idx, unsigned int flag);
SymIdx_t symtab_lookup_in_scope(sstore_pos_t name, int max_scope_depth);
void symtab_set_vla_info(SymIdx_t idx, unsigned short size_expr_idx, 
                         unsigned char dimensions);
void symtab_set_function_info(SymIdx_t idx, unsigned short param_count,
                              unsigned short first_param);


#endif  // SYMTAB_H  // NOLINT