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

#include "../storage/sstore.h"
#include "../storage/astore.h"
#include "../storage/symtab.h"
#include "../ast/astnode.h"

// Function prototypes
static const char* ast_type_to_string(ASTNodeType type);
static const char* sym_type_to_string(SymType type);
static void print_ast_tree(ASTNodeIdx_t idx, int depth);
static void print_symbol_table(const char* symfile_path);

/**
 * @brief Convert AST node type to string representation
 */
static const char* ast_type_to_string(ASTNodeType type) {
    switch (type) {
        case AST_FREE: return "FREE";
        case AST_PROGRAM: return "PROGRAM";
        case AST_FUNCTION: return "FUNCTION";
        case AST_TYPEDEF: return "TYPEDEF";
        case AST_DECLARATION: return "DECLARATION";
        case AST_STATEMENT: return "STATEMENT";
        case AST_EXPRESSION: return "EXPRESSION";
        case AST_LITERAL: return "LITERAL";
        case AST_IDENTIFIER: return "IDENTIFIER";
        case AST_OPERATOR: return "OPERATOR";
        case AST_ASSIGNMENT: return "ASSIGNMENT";
        case AST_IF: return "IF";
        case AST_WHILE: return "WHILE";
        case AST_FOR: return "FOR";
        case AST_DO: return "DO";
        case AST_SWITCH: return "SWITCH";
        case AST_CASE: return "CASE";
        case AST_DEFAULT: return "DEFAULT";
        case AST_BREAK: return "BREAK";
        case AST_CONTINUE: return "CONTINUE";
        case AST_GOTO: return "GOTO";
        case AST_RETURN: return "RETURN";
        case AST_SIZEOF: return "SIZEOF";
        case AST_CALL: return "CALL";
        case AST_CAST: return "CAST";
        case AST_INDEX: return "INDEX";
        case AST_MEMBER: return "MEMBER";
        case AST_POINTER: return "POINTER";
        case AST_DEREFERENCE: return "DEREFERENCE";
        case AST_ADDRESS: return "ADDRESS";
        case AST_LOGICAL: return "LOGICAL";
        case AST_CONDITIONAL: return "CONDITIONAL";
        case AST_COMMA: return "COMMA";
        case AST_BLOCK: return "BLOCK";
        case AST_LABEL: return "LABEL";
        case AST_GOTO_LABEL: return "GOTO_LABEL";
        case AST_EMPTY: return "EMPTY";
        case AST_EOF: return "EOF";
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
        case SYM_UNKOWN: return "UNKNOWN";
        default: return "INVALID";
    }
}

/**
 * @brief Print AST tree in hierarchical format
 */
static void print_ast_tree(ASTNodeIdx_t idx, int depth) {
    if (idx == 0) return;
    
    ASTNode node = astore_get(idx);
    
    // Print indentation
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    
    printf("[%d] %s", idx, ast_type_to_string(node.type));
    
    if (node.tid != 0) {
        printf(" (token: %d)", node.tid);
    }
    
    // Print node values based on type
    switch (node.type) {
        case AST_LITERAL:
        case AST_IDENTIFIER:
            if (node.uvalue != 0) {
                printf(" value: %lu", node.uvalue);
            }
            break;
        case AST_OPERATOR:
        case AST_ASSIGNMENT:
            printf(" op1: %d, op2: %d", node.o1, node.o2);
            if (node.o3 != 0) printf(", op3: %d", node.o3);
            if (node.o4 != 0) printf(", op4: %d", node.o4);
            break;
        default:
            if (node.o1 != 0 || node.o2 != 0 || node.o3 != 0 || node.o4 != 0) {
                printf(" children: %d, %d, %d, %d", node.o1, node.o2, node.o3, node.o4);
            }
            break;
    }
    
    printf("\n");
    
    // Recursively print child nodes
    if (node.o1 != 0) print_ast_tree(node.o1, depth + 1);
    if (node.o2 != 0) print_ast_tree(node.o2, depth + 1);
    if (node.o3 != 0) print_ast_tree(node.o3, depth + 1);
    if (node.o4 != 0) print_ast_tree(node.o4, depth + 1);
}

/**
 * @brief Print symbol table content
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
    printf("Symbol table contains %d entries\n", max_entries);
    
    for (SymIdx_t idx = 1; idx <= max_entries && idx <= 50; idx++) {
        SymTabEntry entry = symtab_get(idx);
        
        if (entry.type == SYM_FREE) {
            continue;  // Skip free entries
        }
        
        printf("[%d] %s", idx, sym_type_to_string(entry.type));
        
        if (entry.name != 0) {
            printf(" name: \"%s\"", sstore_get(entry.name));
        }
        
        if (entry.parent != 0) {
            printf(" parent: %d", entry.parent);
        }
        
        if (entry.next != 0) {
            printf(" next: %d", entry.next);
        }
        
        if (entry.prev != 0) {
            printf(" prev: %d", entry.prev);
        }
        
        if (entry.child != 0) {
            printf(" child: %d", entry.child);
        }
        
        if (entry.sibling != 0) {
            printf(" sibling: %d", entry.sibling);
        }
        
        if (entry.value != 0) {
            printf(" value: \"%s\"", sstore_get(entry.value));
        }
        
        if (entry.line > 0) {
            printf(" line: %d", entry.line);
        }
        
        printf("\n");
    }
    
    if (max_entries > 50) {
        printf("... (showing first 50 entries, %d total entries)\n", max_entries);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <sstorefile> <astfile> <symfile>\n", argv[0]);
        return 1;
    }
    
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
    
    // Print AST tree starting from root (typically index 1)
    printf("\n=== ABSTRACT SYNTAX TREE ===\n");
    ASTNodeIdx_t current_idx = astore_getidx();
    printf("Current AST index: %d\n", current_idx);
    
    // Limit output to prevent infinite loops or excessive output
    int max_ast_nodes = (current_idx > 20) ? 20 : current_idx;
    
    printf("Showing first %d AST nodes:\n", max_ast_nodes);
    for (ASTNodeIdx_t i = 1; i <= max_ast_nodes; i++) {
        ASTNode node = astore_get(i);
        if (node.type != AST_FREE) {
            printf("[%d] %s", i, ast_type_to_string(node.type));
            if (node.tid != 0) printf(" (token: %d)", node.tid);
            
            // Print basic node info without recursion to avoid deep traversal
            if (node.o1 != 0 || node.o2 != 0 || node.o3 != 0 || node.o4 != 0) {
                printf(" children: %d, %d, %d, %d", node.o1, node.o2, node.o3, node.o4);
            }
            printf("\n");
        }
    }
    
    if (current_idx > max_ast_nodes) {
        printf("... (%d total AST nodes, showing first %d)\n", current_idx, max_ast_nodes);
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
    
    return 0;
}
