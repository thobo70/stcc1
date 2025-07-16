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
#include "../ast/ast_types.h"

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
/**
 * @brief Print AST tree recursively
 * @param idx Root node index
 * @param depth Current depth for indentation
 * @note Currently unused in main display but available for tree visualization
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void print_ast_tree(ASTNodeIdx_t idx, int depth) {
    if (idx == 0) return;

    ASTNode node = astore_get(idx);

    // Print indentation
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }

    printf("[%d] %s", idx, ast_type_to_string(node.type));

    if (node.token_idx != 0) {
        printf(" (token: %d)", node.token_idx);
    }

    // Print flags if present
    if (node.flags != 0) {
        printf(" (flags: 0x%x)", node.flags);
    }

    // Print type info if present
    if (node.type_idx != 0) {
        printf(" (type: %d)", node.type_idx);
    }

    // Print node values based on type category
    switch (node.type) {
        case AST_LIT_INTEGER:
        case AST_LIT_FLOAT:
        case AST_LIT_CHAR:
        case AST_LIT_STRING:
            if (node.binary.value.long_value != 0) {
                printf(" value: %ld", node.binary.value.long_value);
            }
            break;
        case AST_EXPR_IDENTIFIER:
            if (node.binary.value.string_pos != 0) {
                printf(" name_pos: %d", node.binary.value.string_pos);
            }
            break;
        case AST_EXPR_BINARY_OP:
            printf(" left: %d, right: %d", node.binary.left, node.binary.right);
            break;
        case AST_EXPR_UNARY_OP:
            printf(" operand: %d, op: %d", node.unary.operand, node.unary.operator);
            break;
        case AST_STMT_IF:
        case AST_STMT_WHILE:
            printf(" condition: %d, then: %d",
                   node.conditional.condition, node.conditional.then_stmt);
            if (node.conditional.else_stmt != 0) {
                printf(", else: %d", node.conditional.else_stmt);
            }
            break;
        case AST_STMT_COMPOUND:
            printf(" declarations: %d, statements: %d",
                   node.compound.declarations, node.compound.statements);
            break;
        case AST_EXPR_CALL:
            printf(" function: %d, args: %d, count: %d",
                   node.call.function, node.call.arguments, node.call.arg_count);
            break;
        default:
            // Generic child display
            if (node.children.child1 != 0 || node.children.child2 != 0 ||
                node.children.child3 != 0 || node.children.child4 != 0) {
                printf(" children: %d, %d, %d, %d",
                       node.children.child1, node.children.child2,
                       node.children.child3, node.children.child4);
            }
            break;
    }

    printf("\n");

    // Recursively print child nodes based on node type
    switch (node.type) {
        case AST_EXPR_BINARY_OP:
            if (node.binary.left != 0) print_ast_tree(node.binary.left, depth + 1);
            if (node.binary.right != 0) print_ast_tree(node.binary.right, depth + 1);
            break;
        case AST_EXPR_UNARY_OP:
            if (node.unary.operand != 0) print_ast_tree(node.unary.operand, depth + 1);
            break;
        case AST_STMT_IF:
        case AST_STMT_WHILE:
            if (node.conditional.condition != 0) print_ast_tree(node.conditional.condition, depth + 1);
            if (node.conditional.then_stmt != 0) print_ast_tree(node.conditional.then_stmt, depth + 1);
            if (node.conditional.else_stmt != 0) print_ast_tree(node.conditional.else_stmt, depth + 1);
            break;
        case AST_STMT_COMPOUND:
            if (node.compound.declarations != 0) print_ast_tree(node.compound.declarations, depth + 1);
            if (node.compound.statements != 0) print_ast_tree(node.compound.statements, depth + 1);
            break;
        case AST_EXPR_CALL:
            if (node.call.function != 0) print_ast_tree(node.call.function, depth + 1);
            if (node.call.arguments != 0) print_ast_tree(node.call.arguments, depth + 1);
            break;
        default:
            // Generic child traversal
            if (node.children.child1 != 0) print_ast_tree(node.children.child1, depth + 1);
            if (node.children.child2 != 0) print_ast_tree(node.children.child2, depth + 1);
            if (node.children.child3 != 0) print_ast_tree(node.children.child3, depth + 1);
            if (node.children.child4 != 0) print_ast_tree(node.children.child4, depth + 1);
            break;
    }
}
#pragma GCC diagnostic pop

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
            if (node.token_idx != 0) printf(" (token: %d)", node.token_idx);

            // Print basic node info without recursion to avoid deep traversal
            if (node.children.child1 != 0 || node.children.child2 != 0 ||
                node.children.child3 != 0 || node.children.child4 != 0) {
                printf(" children: %d, %d, %d, %d",
                       node.children.child1, node.children.child2,
                       node.children.child3, node.children.child4);
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
