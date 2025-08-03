/**
 * @file cc1t.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Test program for the astore and symtab modules, displays AST and symbol table content
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <stdio.h>
#include <string.h>

#include "../storage/sstore.h"
#include "../storage/astore.h"
#include "../storage/symtab.h"
#include "../ast/ast_types.h"
#include "../utils/hmapbuf.h"

// Function prototypes
static const char* ast_type_to_string(ASTNodeType type);
static const char* sym_type_to_string(SymType type);
static void print_ast_tree(ASTNodeIdx_t root_idx);
static void print_symbol_table(const char* symfile_path);

/**
 * @brief Convert AST node type to string representation
 */
static const char* ast_type_to_string(ASTNodeType type) {
    switch (type) {
        // Special nodes
        case AST_FREE: return "FREE";
        case AST_PROGRAM: return "PROGRAM";
        case AST_TRANSLATION_UNIT: return "TRANSLATION_UNIT";
        case AST_EOF: return "EOF";
        case AST_ERROR: return "ERROR";

        // Declaration nodes
        case AST_FUNCTION_DECL: return "FUNCTION_DECL";
        case AST_FUNCTION_DEF: return "FUNCTION_DEF";
        case AST_VAR_DECL: return "VAR_DECL";
        case AST_PARAM_DECL: return "PARAM_DECL";
        case AST_FIELD_DECL: return "FIELD_DECL";
        case AST_TYPEDEF_DECL: return "TYPEDEF_DECL";
        case AST_STRUCT_DECL: return "STRUCT_DECL";
        case AST_UNION_DECL: return "UNION_DECL";
        case AST_ENUM_DECL: return "ENUM_DECL";
        case AST_ENUM_CONSTANT: return "ENUM_CONSTANT";

        // Type nodes
        case AST_TYPE_BASIC: return "TYPE_BASIC";
        case AST_TYPE_POINTER: return "TYPE_POINTER";
        case AST_TYPE_ARRAY: return "TYPE_ARRAY";
        case AST_TYPE_FUNCTION: return "TYPE_FUNCTION";
        case AST_TYPE_STRUCT: return "TYPE_STRUCT";
        case AST_TYPE_UNION: return "TYPE_UNION";
        case AST_TYPE_ENUM: return "TYPE_ENUM";
        case AST_TYPE_TYPEDEF: return "TYPE_TYPEDEF";
        case AST_TYPE_QUALIFIER: return "TYPE_QUALIFIER";
        case AST_TYPE_STORAGE: return "TYPE_STORAGE";

        // Statement nodes
        case AST_STMT_COMPOUND: return "STMT_COMPOUND";
        case AST_STMT_EXPRESSION: return "STMT_EXPRESSION";
        case AST_STMT_IF: return "STMT_IF";
        case AST_STMT_WHILE: return "STMT_WHILE";
        case AST_STMT_FOR: return "STMT_FOR";
        case AST_STMT_DO_WHILE: return "STMT_DO_WHILE";
        case AST_STMT_SWITCH: return "STMT_SWITCH";
        case AST_STMT_CASE: return "STMT_CASE";
        case AST_STMT_DEFAULT: return "STMT_DEFAULT";
        case AST_STMT_BREAK: return "STMT_BREAK";
        case AST_STMT_CONTINUE: return "STMT_CONTINUE";
        case AST_STMT_RETURN: return "STMT_RETURN";
        case AST_STMT_GOTO: return "STMT_GOTO";
        case AST_STMT_LABEL: return "STMT_LABEL";
        case AST_STMT_EMPTY: return "STMT_EMPTY";

        // Expression nodes
        case AST_EXPR_LITERAL: return "EXPR_LITERAL";
        case AST_EXPR_IDENTIFIER: return "EXPR_IDENTIFIER";
        case AST_EXPR_BINARY_OP: return "EXPR_BINARY_OP";
        case AST_EXPR_UNARY_OP: return "EXPR_UNARY_OP";
        case AST_EXPR_ASSIGN: return "EXPR_ASSIGN";
        case AST_EXPR_CALL: return "EXPR_CALL";
        case AST_EXPR_MEMBER: return "EXPR_MEMBER";
        case AST_EXPR_MEMBER_PTR: return "EXPR_MEMBER_PTR";
        case AST_EXPR_INDEX: return "EXPR_INDEX";
        case AST_EXPR_CAST: return "EXPR_CAST";
        case AST_EXPR_SIZEOF: return "EXPR_SIZEOF";
        case AST_EXPR_CONDITIONAL: return "EXPR_CONDITIONAL";
        case AST_EXPR_COMMA: return "EXPR_COMMA";
        case AST_EXPR_INIT_LIST: return "EXPR_INIT_LIST";
        case AST_EXPR_COMPOUND_LITERAL: return "EXPR_COMPOUND_LITERAL";

        // Literal subtypes
        case AST_LIT_INTEGER: return "LIT_INTEGER";
        case AST_LIT_FLOAT: return "LIT_FLOAT";
        case AST_LIT_CHAR: return "LIT_CHAR";
        case AST_LIT_STRING: return "LIT_STRING";
        
        // C99-specific node types
        case AST_EXPR_DESIGNATED_FIELD: return "DESIGNATED_FIELD";
        case AST_EXPR_DESIGNATED_INDEX: return "DESIGNATED_INDEX"; 
        case AST_INITIALIZER: return "INITIALIZER"; 
        case AST_PARAM_VARIADIC: return "PARAM_VARIADIC";
        case AST_TYPE_COMPLEX: return "TYPE_COMPLEX";
        case AST_TYPE_IMAGINARY: return "TYPE_IMAGINARY";
        case AST_LIT_COMPLEX: return "LIT_COMPLEX";

        default: return "UNKNOWN";
    }
}

/**
 * @brief Convert symbol type to string representation
 */
static const char* sym_type_to_string(SymType type) {
    switch (type) {
        case SYM_FREE: return "FREE";
        case SYM_VARIABLE: return "VARIABLE";
        case SYM_FUNCTION: return "FUNCTION";
        case SYM_TYPEDEF: return "TYPEDEF";
        case SYM_LABEL: return "LABEL";
        case SYM_ENUMERATOR: return "ENUMERATOR";
        case SYM_STRUCT: return "STRUCT";
        case SYM_UNION: return "UNION";
        case SYM_ENUM: return "ENUM";
        case SYM_CONSTANT: return "CONSTANT";
        case SYM_UNKNOWN: return "UNKNOWN";                      // Fixed typo
        // C99-specific types
        case SYM_VLA_PARAMETER: return "VLA_PARAM";
        case SYM_FLEXIBLE_MEMBER: return "FLEX_MEMBER";
        case SYM_ANONYMOUS_STRUCT: return "ANON_STRUCT";
        case SYM_UNIVERSAL_CHAR: return "UNIV_CHAR";
        default: return "INVALID";
    }
}

/**
 * @brief Print AST tree in hierarchical format
 */
/**
 * @brief Print ASCII tree prefix for tree visualization
 */
static void print_tree_prefix(const char* prefix, int is_last, int is_root) {
    if (is_root) {
        printf("├─ ");
    } else {
        printf("%s", prefix);
        printf("%s", is_last ? "└─ " : "├─ ");
    }
}

/**
 * @brief Get node value as string for display
 */
static void get_node_value_string(ASTNode *node, char *buffer, size_t buffer_size) {
    buffer[0] = '\0';
    
    switch (node->type) {
        case AST_LIT_INTEGER:
            snprintf(buffer, buffer_size, " = %ld", node->binary.value.long_value);
            break;
        case AST_LIT_FLOAT:
            snprintf(buffer, buffer_size, " = %.2f", node->binary.value.float_value);
            break;
        case AST_LIT_CHAR:
            snprintf(buffer, buffer_size, " = '%c'", (char)node->binary.value.long_value);
            break;
        case AST_LIT_STRING:
            if (node->binary.value.string_pos != 0) {
                snprintf(buffer, buffer_size, " = \"%s\"", sstore_get(node->binary.value.string_pos));
            }
            break;
        case AST_EXPR_IDENTIFIER:
            // For identifiers, check symbol_idx first (primary reference)
            if (node->binary.value.symbol_idx != 0) {
                SymTabEntry sym = symtab_get(node->binary.value.symbol_idx);
                if (sym.name != 0) {
                    char* sym_name = sstore_get(sym.name);
                    snprintf(buffer, buffer_size, " '%s' (sym:%d)", 
                            sym_name ? sym_name : "(null)", node->binary.value.symbol_idx);
                } else {
                    snprintf(buffer, buffer_size, " (sym:%d name=0)", node->binary.value.symbol_idx);
                }
            } else if (node->binary.value.string_pos != 0) {
                // Fallback for identifiers that only have string position (shouldn't happen in correct code)
                snprintf(buffer, buffer_size, " '%s' (string_only)", sstore_get(node->binary.value.string_pos));
            } else {
                snprintf(buffer, buffer_size, " (no reference)");
            }
            break;
        case AST_EXPR_BINARY_OP:
            // Show operator if we can determine it from token
            snprintf(buffer, buffer_size, " (L:%d, R:%d)", node->binary.left, node->binary.right);
            break;
        case AST_EXPR_UNARY_OP:
            snprintf(buffer, buffer_size, " (operand:%d)", node->unary.operand);
            break;
        case AST_VAR_DECL:
        case AST_FUNCTION_DEF:
        case AST_FUNCTION_DECL:
            snprintf(buffer, buffer_size, " (sym:%d, init:%d, type:%d)", 
                    node->declaration.symbol_idx, node->declaration.initializer, node->declaration.type_idx);
            break;
        case AST_STMT_COMPOUND:
            snprintf(buffer, buffer_size, " (decls:%d, stmts:%d, scope:%d)", 
                    node->compound.declarations, node->compound.statements, node->compound.scope_idx);
            break;
        case AST_EXPR_CALL:
            snprintf(buffer, buffer_size, " (func:%d, args:%d, count:%d)",
                    node->call.function, node->call.arguments, node->call.arg_count);
            break;
        default:
            // Show non-zero children
            if (node->children.child1 != 0 || node->children.child2 != 0 ||
                node->children.child3 != 0 || node->children.child4 != 0) {
                snprintf(buffer, buffer_size, " (%d,%d,%d,%d)",
                        node->children.child1, node->children.child2,
                        node->children.child3, node->children.child4);
            }
            break;
    }
}

/**
 * @brief Print AST tree in ASCII tree format
 */
static void print_ast_tree_recursive(ASTNodeIdx_t idx, const char* prefix, int is_last, int depth) {
    if (idx == 0 || depth > 15) return; // Prevent infinite recursion
    
    // Simple cycle detection - don't revisit nodes we've already seen at this depth
    static ASTNodeIdx_t visited[100] = {0};
    static int visit_count = 0;
    
    if (depth == 0) {
        visit_count = 0; // Reset for new tree
    }
    
    // Check for cycles
    for (int i = 0; i < visit_count; i++) {
        if (visited[i] == idx) {
            printf("%s%s[%d] **CYCLE DETECTED**\n", prefix, is_last ? "└─ " : "├─ ", idx);
            return;
        }
    }
    
    if (visit_count < 100) {
        visited[visit_count++] = idx;
    }
    
    ASTNode node = astore_get(idx);
    
    // Print tree connector
    print_tree_prefix(prefix, is_last, depth == 0);
    
    // Print node information
    char value_str[256];
    get_node_value_string(&node, value_str, sizeof(value_str));
    
    printf("[%d] %s%s", idx, ast_type_to_string(node.type), value_str);
    
    // Add metadata
    if (node.token_idx != 0) printf(" @t%d", node.token_idx);
    if (node.flags != 0) printf(" flags:0x%x", node.flags);
    if (node.type_idx != 0) printf(" type:%d", node.type_idx);
    
    printf("\n");
    
    // Prepare prefix for children
    char new_prefix[256];
    snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "   " : "│  ");
    
    // Collect child indices based on node type
    ASTNodeIdx_t children[8] = {0}; // Max 8 children for any node type
    int child_count = 0;
    
    switch (node.type) {
        case AST_EXPR_BINARY_OP:
        case AST_EXPR_ASSIGN:
            if (node.binary.left != 0) children[child_count++] = node.binary.left;
            if (node.binary.right != 0) children[child_count++] = node.binary.right;
            break;
            
        case AST_EXPR_UNARY_OP:
            if (node.unary.operand != 0) children[child_count++] = node.unary.operand;
            break;
            
        case AST_STMT_IF:
        case AST_STMT_WHILE:
            if (node.conditional.condition != 0) children[child_count++] = node.conditional.condition;
            if (node.conditional.then_stmt != 0) children[child_count++] = node.conditional.then_stmt;
            if (node.conditional.else_stmt != 0) children[child_count++] = node.conditional.else_stmt;
            break;
            
        case AST_STMT_COMPOUND:
            if (node.compound.declarations != 0) children[child_count++] = node.compound.declarations;
            if (node.compound.statements != 0) {
                // Follow the statement chain starting from the first statement
                ASTNodeIdx_t current_stmt = node.compound.statements;
                while (current_stmt != 0 && child_count < 8) {  // Limit to prevent infinite loops
                    children[child_count++] = current_stmt;
                    
                    // Get the next statement using direct header access
                    HBNode *stmt_node = HBGet(current_stmt, HBMODE_AST);
                    if (stmt_node) {
                        ASTNodeIdx_t next_stmt = stmt_node->ast.next_stmt;  // Direct access!
                        
                        if (next_stmt != 0 && next_stmt != current_stmt) {  // Prevent cycles
                            current_stmt = next_stmt;
                        } else {
                            break;  // End of chain
                        }
                    } else {
                        break;
                    }
                }
            }
            break;
            
        case AST_EXPR_CALL:
            if (node.call.function != 0) children[child_count++] = node.call.function;
            if (node.call.arguments != 0) children[child_count++] = node.call.arguments;
            break;
            
        case AST_VAR_DECL:
        case AST_FUNCTION_DECL:
        case AST_FUNCTION_DEF:
        case AST_PARAM_DECL:
            // Declaration nodes use the declaration structure
            if (node.declaration.initializer != 0) children[child_count++] = node.declaration.initializer;
            // Note: Don't traverse symbol_idx or type_idx as these are indices, not child nodes
            break;
            
        case AST_STMT_RETURN:
            // Return statement - child1 is the return expression
            if (node.children.child1 != 0 && node.children.child1 != idx) 
                children[child_count++] = node.children.child1;
            break;
            
        default:
            // Generic children - but be more careful about circular references
            if (node.children.child1 != 0 && node.children.child1 != idx) 
                children[child_count++] = node.children.child1;
            if (node.children.child2 != 0 && node.children.child2 != idx) 
                children[child_count++] = node.children.child2;
            if (node.children.child3 != 0 && node.children.child3 != idx) 
                children[child_count++] = node.children.child3;
            if (node.children.child4 != 0 && node.children.child4 != idx) 
                children[child_count++] = node.children.child4;
            break;
    }
    
    // Print children
    for (int i = 0; i < child_count; i++) {
        print_ast_tree_recursive(children[i], new_prefix, i == child_count - 1, depth + 1);
    }
}

/**
 * @brief Print AST tree starting from root
 */
static void print_ast_tree(ASTNodeIdx_t root_idx) {
    if (root_idx == 0) {
        printf("No AST root to display\n");
        return;
    }
    
    printf("AST Tree Structure:\n");
    print_ast_tree_recursive(root_idx, "", 1, 0);
}

/**
 * @brief Print detailed symbol table content with enhanced formatting
 */
static void print_symbol_table(const char* symfile_path) {
    printf("\n=== SYMBOL TABLE ===\n");

    // Get file size to determine number of entries
    FILE* symfile_check = fopen(symfile_path, "rb");
    if (symfile_check == NULL) {
        printf("Cannot determine symbol table size for %s\n", symfile_path);
        return;
    }

    fseek(symfile_check, 0, SEEK_END);
    size_t file_size = ftell(symfile_check);
    fclose(symfile_check);

    int max_entries = (int)(file_size / sizeof(SymTabEntry));
    printf("Symbol table contains %d entries (file size: %zu bytes)\n\n", max_entries, file_size);

    // Print table header
    printf("┌─────┬──────────┬────────────────────┬──────┬─────┬─────┬──────┬────────┬────────────────────┬──────┬───────┐\n");
    printf("│ Idx │   Type   │       Name         │ Prnt │ Nxt │ Prv │ Chld │ Siblng │       Value        │ Line │ Scope │\n");
    printf("├─────┼──────────┼────────────────────┼──────┼─────┼─────┼──────┼────────┼────────────────────┼──────┼───────┤\n");

    int shown_entries = 0;
    int total_active = 0;
    
    for (SymIdx_t idx = 1; idx <= max_entries && idx <= 100; idx++) {
        SymTabEntry entry = symtab_get(idx);

        if (entry.type == SYM_FREE) {
            continue;  // Skip free entries for main display
        }

        total_active++;
        shown_entries++;

        // Format name
        char name_str[21] = "";
        if (entry.name != 0) {
            const char* name = sstore_get(entry.name);
            snprintf(name_str, sizeof(name_str), "%.20s", name);
        } else {
            strcpy(name_str, "<no name>");
        }

        // Format value
        char value_str[21] = "";
        if (entry.value != 0) {
            const char* value = sstore_get(entry.value);
            snprintf(value_str, sizeof(value_str), "%.20s", value);
        } else {
            strcpy(value_str, "");
        }

        // Format type string
        char type_str[11];
        snprintf(type_str, sizeof(type_str), "%.10s", sym_type_to_string(entry.type));

        printf("│%4d │%10s│%-20s│%5d │%4d │%4d │%5d │%7d │%-20s│%5d │%6d │\n",
               idx, type_str, name_str,
               entry.parent, entry.next, entry.prev, entry.child, entry.sibling,
               value_str, entry.line, entry.scope_depth);
    }

    printf("└─────┴──────────┴────────────────────┴──────┴─────┴─────┴──────┴────────┴────────────────────┴──────┴───────┘\n");

    if (max_entries > 100) {
        printf("... (showing first %d active entries, %d total active, %d total entries)\n", 
               shown_entries, total_active, max_entries);
    } else {
        printf("Total: %d active entries out of %d total entries\n", total_active, max_entries);
    }

    // Print scope analysis
    printf("\n=== SCOPE ANALYSIS ===\n");
    int scope_counts[10] = {0}; // Track up to depth 9
    int max_scope = 0;
    
    for (SymIdx_t idx = 1; idx <= max_entries; idx++) {
        SymTabEntry entry = symtab_get(idx);
        if (entry.type != SYM_FREE && entry.scope_depth >= 0 && entry.scope_depth < 10) {
            scope_counts[entry.scope_depth]++;
            if (entry.scope_depth > max_scope) {
                max_scope = entry.scope_depth;
            }
        }
    }

    for (int depth = 0; depth <= max_scope; depth++) {
        if (scope_counts[depth] > 0) {
            const char* scope_name;
            switch (depth) {
                case 0: scope_name = "File/Global"; break;
                case 1: scope_name = "Function"; break;
                default: scope_name = "Block"; break;
            }
            printf("Scope depth %d (%s): %d symbols\n", depth, scope_name, scope_counts[depth]);
        }
    }
    
    // C99 FLAGS ANALYSIS
    printf("\n=== C99 FLAGS ANALYSIS ===\n");
    int c99_flag_counts[13] = {0}; // Track all C99 flags
    const char* c99_flag_names[] = {
        "inline", "restrict", "VLA", "flexible", "complex", "imaginary",
        "variadic", "universal_char", "designated", "compound_lit", "mixed_decl", "const", "volatile"
    };
    const unsigned int c99_flag_values[] = {
        SYM_FLAG_INLINE, SYM_FLAG_RESTRICT, SYM_FLAG_VLA, SYM_FLAG_FLEXIBLE,
        SYM_FLAG_COMPLEX, SYM_FLAG_IMAGINARY, SYM_FLAG_VARIADIC, 
        SYM_FLAG_UNIVERSAL_CHAR, SYM_FLAG_DESIGNATED, SYM_FLAG_COMPOUND_LIT,
        SYM_FLAG_MIXED_DECL, SYM_FLAG_CONST, SYM_FLAG_VOLATILE
    };
    
    for (SymIdx_t idx = 1; idx <= max_entries; idx++) {
        SymTabEntry entry = symtab_get(idx);
        if (entry.type != SYM_FREE) {
            for (int i = 0; i < 13; i++) {
                if (entry.flags & c99_flag_values[i]) {
                    c99_flag_counts[i]++;
                }
            }
        }
    }
    
    for (int i = 0; i < 13; i++) {
        if (c99_flag_counts[i] > 0) {
            printf("C99 %s symbols: %d\n", c99_flag_names[i], c99_flag_counts[i]);
        }
    }

    // Print symbol type statistics
    printf("\n=== SYMBOL TYPE STATISTICS ===\n");
    int type_counts[20] = {0}; // Enough for all symbol types
    
    for (SymIdx_t idx = 1; idx <= max_entries; idx++) {
        SymTabEntry entry = symtab_get(idx);
        if (entry.type != SYM_FREE && entry.type < 20) {
            type_counts[entry.type]++;
        }
    }

    for (int type = 0; type < 20; type++) {
        if (type_counts[type] > 0) {
            printf("%-12s: %d\n", sym_type_to_string((SymType)type), type_counts[type]);
        }
    }

    // Show relationships
    printf("\n=== SYMBOL RELATIONSHIPS ===\n");
    int with_parent = 0, with_children = 0, with_siblings = 0, with_next = 0;
    
    for (SymIdx_t idx = 1; idx <= max_entries; idx++) {
        SymTabEntry entry = symtab_get(idx);
        if (entry.type != SYM_FREE) {
            if (entry.parent != 0) with_parent++;
            if (entry.child != 0) with_children++;
            if (entry.sibling != 0) with_siblings++;
            if (entry.next != 0) with_next++;
        }
    }
    
    printf("Symbols with parent:   %d\n", with_parent);
    printf("Symbols with children: %d\n", with_children);
    printf("Symbols with siblings: %d\n", with_siblings);
    printf("Symbols with next:     %d\n", with_next);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <sstorefile> <astfile> <symfile>\n", argv[0]);
        return 1;
    }

    // Initialize hash map buffer system for AST node access
    HBInit();

    // Open string store
    if (sstore_open(argv[1]) != 0) {
        fprintf(stderr, "Error: Cannot open sstorefile %s\n", argv[1]);
        return 1;
    }

    // Open AST store
    if (astore_open(argv[2]) != 0) {
        fprintf(stderr, "Error: Cannot open astfile %s\n", argv[2]);
        sstore_close();
        return 1;
    }

    // Open symbol table
    if (symtab_open(argv[3]) != 0) {
        fprintf(stderr, "Error: Cannot open symfile %s\n", argv[3]);
        astore_close();
        sstore_close();
        return 1;
    }

    printf("=== CC1T: AST and Symbol Table Viewer ===\n");

    // Print AST tree starting from root 
    printf("\n=== ABSTRACT SYNTAX TREE ===\n");
    ASTNodeIdx_t current_idx = astore_getidx();
    printf("Current AST index: %d\n", current_idx);

    if (current_idx > 1) {
        // Find the root node (typically AST_PROGRAM or first non-FREE node)
        ASTNodeIdx_t root_idx = 0;
        for (ASTNodeIdx_t i = 1; i < current_idx; i++) {
            ASTNode node = astore_get(i);
            if (node.type == AST_PROGRAM || node.type == AST_TRANSLATION_UNIT) {
                root_idx = i;
                break;
            }
        }
        
        // If no program node found, use the first non-FREE node
        if (root_idx == 0) {
            for (ASTNodeIdx_t i = 1; i < current_idx; i++) {
                ASTNode node = astore_get(i);
                if (node.type != AST_FREE) {
                    root_idx = i;
                    break;
                }
            }
        }
        
        if (root_idx != 0) {
            printf("\n");
            print_ast_tree(root_idx);
        } else {
            printf("No valid root node found\n");
        }
        
        // Also show flat list for debugging
        printf("\n=== ALL AST NODES (FLAT VIEW) ===\n");
        printf("┌─────┬─────────────────────┬───────┬───────┬──────┬────────────────────────────────────────┐\n");
        printf("│ Idx │        Type         │ Token │ Flags │ TIdx │                Details                 │\n");
        printf("├─────┼─────────────────────┼───────┼───────┼──────┼────────────────────────────────────────┤\n");
        
        for (ASTNodeIdx_t i = 1; i < current_idx; i++) {
            ASTNode node = astore_get(i);
            
            // Format type string
            char type_str[22];
            snprintf(type_str, sizeof(type_str), "%.21s", ast_type_to_string(node.type));
            
            // Format details
            char details[41] = "";
            if (node.type != AST_FREE) {
                char value_str[256];
                get_node_value_string(&node, value_str, sizeof(value_str));
                snprintf(details, sizeof(details), "%.40s", value_str);
            }
            
            printf("│%4d │%-21s│%6d │ 0x%03x │%5d │%-40s│\n",
                   i, type_str, node.token_idx, node.flags, node.type_idx, details);
        }
        printf("└─────┴─────────────────────┴───────┴───────┴──────┴────────────────────────────────────────┘\n");
        
    } else {
        printf("No AST nodes to display\n");
    }

    // Print symbol table
    print_symbol_table(argv[3]);

    // Print summary statistics
    printf("\n=== SUMMARY ===\n");
    printf("AST nodes processed: %d\n", current_idx);
    printf("String store available\n");
    printf("Symbol table available\n");

    // Clean up
    symtab_close();
    astore_close();
    sstore_close();
    HBEnd();

    return 0;
}
