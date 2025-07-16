%{
#include <stdio.h>
#include "src/scanner.h" // Include your scanner header file
%}

%union {
    int intval;
    char *strval;
    // Add other types as needed
}

%token <intval> T_INT
%token <intval> T_LONG
%token <intval> T_SHORT
%token <intval> T_FLOAT
%token <intval> T_DOUBLE
%token <intval> T_CHAR
%token <intval> T_VOID
%token <intval> T_RETURN
%token <intval> T_IF
%token <intval> T_ELSE
%token <intval> T_WHILE
%token <intval> T_FOR
%token <intval> T_DO
%token <intval> T_SWITCH
%token <intval> T_CASE
%token <intval> T_DEFAULT
%token <intval> T_BREAK
%token <intval> T_CONTINUE
%token <intval> T_GOTO
%token <intval> T_SIZEOF
%token <intval> T_TYPEDEF
%token <intval> T_EXTERN
%token <intval> T_STATIC
%token <intval> T_AUTO
%token <intval> T_REGISTER
%token <intval> T_CONST
%token <intval> T_VOLATILE
%token <intval> T_SIGNED
%token <intval> T_UNSIGNED
%token <intval> T_STRUCT
%token <intval> T_UNION
%token <intval> T_ENUM
%token <intval> T_PLUS
%token <intval> T_MINUS
%token <intval> T_MUL
%token <intval> T_DIV
%token <intval> T_MOD
%token <intval> T_ASSIGN
%token <intval> T_ANDEQ
%token <intval> T_OREQ
%token <intval> T_XOREQ
%token <intval> T_PLUSEQ
%token <intval> T_MINUSEQ
%token <intval> T_STAREQ
%token <intval> T_SLASHEQ
%token <intval> T_PERCENTEQ
%token <intval> T_LSHIFTEQ
%token <intval> T_RSHIFTEQ
%token <intval> T_EQ
%token <intval> T_NEQ
%token <intval> T_LT
%token <intval> T_GT
%token <intval> T_LTE
%token <intval> T_GTE
%token <intval> T_LOGAND
%token <intval> T_LOGOR
%token <intval> T_NOT
%token <intval> T_LPAREN
%token <intval> T_RPAREN
%token <intval> T_LBRACE
%token <intval> T_RBRACE
%token <intval> T_LBRACKET
%token <intval> T_RBRACKET
%token <intval> T_COMMA
%token <intval> T_SEMICOLON
%token <intval> T_COLON
%token <strval> T_ID
%token <intval> T_EOF
%token <intval> T_ERROR
%token <intval> T_INC
%token <intval> T_DEC
%token <intval> T_STAR
%token <intval> T_SLASH
%token <intval> T_PERCENT
%token <intval> T_LESS
%token <intval> T_GREATER
%token <intval> T_AMPERSAND
%token <intval> T_PIPE
%token <intval> T_CARET
%token <intval> T_TILDE
%token <intval> T_EXCLAMATION
%token <intval> T_QUESTION
%token <intval> T_DOT
%token <intval> T_ARROW
%token <intval> T_LSHIFT
%token <intval> T_RSHIFT
%token <strval> T_LITSTRING
%token <strval> T_LITCHAR
%token <intval> T_LITINT
%token <intval> T_LITFLOAT
%token <intval> T_ELLIPSIS
%token <intval> T_UNKNOWN

%type <intval> translation_unit external_declaration function_definition declaration
%type <intval> declaration_list declaration_specifiers init_declarator_list init_declarator
%type <intval> type_specifier type_specifier_seq struct_or_union_specifier struct_declaration_list
%type <intval> struct_declaration specifier_qualifier_list struct_declarator_list
%type <intval> struct_declarator enum_specifier enumerator_list enumerator
%type <intval> declarator direct_declarator pointer type_qualifier_list
%type <intval> parameter_type_list parameter_list parameter_declaration
%type <intval> identifier_list type_name abstract_declarator direct_abstract_declarator
%type <intval> initializer initializer_list statement labeled_statement
%type <intval> compound_statement statement_list expression_statement
%type <intval> selection_statement iteration_statement jump_statement expression
%type <intval> assignment_expression assignment_operator conditional_expression
%type <intval> constant_expression logical_or_expression logical_and_expression
%type <intval> inclusive_or_expression exclusive_or_expression and_expression
%type <intval> equality_expression relational_expression shift_expression
%type <intval> additive_expression multiplicative_expression cast_expression
%type <intval> unary_expression unary_operator postfix_expression primary_expression

%%

translation_unit
    : external_declaration
    | translation_unit external_declaration
    ;

external_declaration
    : function_definition
    | declaration
    ;

function_definition
    : declaration_specifiers declarator declaration_list compound_statement
    | declaration_specifiers declarator compound_statement
    | declarator declaration_list compound_statement
    | declarator compound_statement
    ;

declaration
    : declaration_specifiers init_declarator_list T_SEMICOLON
    | declaration_specifiers T_SEMICOLON
    ;

declaration_list
    : declaration
    | declaration_list declaration
    ;

declaration_specifiers
    : storage_class_specifier declaration_specifiers
    | storage_class_specifier
    | type_specifier_seq declaration_specifiers
    | type_specifier_seq
    | type_specifier declaration_specifiers
    | type_specifier
    | type_qualifier declaration_specifiers
    | type_qualifier
    ;

init_declarator_list
    : init_declarator
    | init_declarator_list T_COMMA init_declarator
    ;

declarator
    : pointer direct_declarator
    | direct_declarator
    ;

initializer
    : assignment_expression
    | T_LBRACE initializer_list T_RBRACE
    | T_LBRACE initializer_list T_COMMA T_RBRACE
    ;

storage_class_specifier
    : T_TYPEDEF
    | T_EXTERN
    | T_STATIC
    | T_AUTO
    | T_REGISTER
    ;

type_qualifier
    : T_CONST
    | T_VOLATILE
    ;

init_declarator
    : declarator
    | declarator T_ASSIGN initializer
    ;

type_specifier
    : T_INT
    | T_LONG
    | T_SHORT
    | T_FLOAT
    | T_DOUBLE
    | T_CHAR
    | T_VOID
    | T_SIGNED
    | T_UNSIGNED
    | struct_or_union_specifier
    | enum_specifier
    | T_ID
    ;

type_specifier_seq
    : type_specifier
    | type_specifier_seq type_specifier
    ;

struct_or_union_specifier
    : T_STRUCT T_ID T_LBRACE struct_declaration_list T_RBRACE
    | T_STRUCT T_LBRACE struct_declaration_list T_RBRACE
    | T_STRUCT T_ID
    | T_UNION T_ID T_LBRACE struct_declaration_list T_RBRACE
    | T_UNION T_LBRACE struct_declaration_list T_RBRACE
    | T_UNION T_ID
    ;

struct_declaration_list
    : struct_declaration
    | struct_declaration_list struct_declaration
    ;

struct_declaration
    : specifier_qualifier_list struct_declarator_list T_SEMICOLON
    ;

specifier_qualifier_list
    : type_specifier_seq specifier_qualifier_list
    | type_specifier_seq
    | type_specifier specifier_qualifier_list
    | type_specifier
    | type_qualifier specifier_qualifier_list
    | type_qualifier
    ;

struct_declarator_list
    : struct_declarator
    | struct_declarator_list T_COMMA struct_declarator
    ;

struct_declarator
    : declarator
    | T_COLON constant_expression
    | declarator T_COLON constant_expression
    ;

enum_specifier
    : T_ENUM T_LBRACE enumerator_list T_RBRACE
    | T_ENUM T_ID T_LBRACE enumerator_list T_RBRACE
    | T_ENUM T_ID
    ;

enumerator_list
    : enumerator
    | enumerator_list T_COMMA enumerator
    ;

enumerator
    : T_ID
    | T_ID T_ASSIGN constant_expression
    ;

declarator
    : pointer direct_declarator
    | direct_declarator
    ;

direct_declarator
    : T_ID
    | T_LPAREN declarator T_RPAREN
    | direct_declarator T_LBRACKET constant_expression T_RBRACKET
    | direct_declarator T_LBRACKET T_RBRACKET
    | direct_declarator T_LPAREN parameter_type_list T_RPAREN
    | direct_declarator T_LPAREN identifier_list T_RPAREN
    | direct_declarator T_LPAREN T_RPAREN
    ;

pointer
    : T_STAR
    | T_STAR type_qualifier_list
    | T_STAR pointer
    | T_STAR type_qualifier_list pointer
    ;

type_qualifier_list
    : type_qualifier
    | type_qualifier_list type_qualifier
    ;

parameter_type_list
    : parameter_list
    | parameter_list T_COMMA T_ELLIPSIS
    ;

parameter_list
    : parameter_declaration
    | parameter_list T_COMMA parameter_declaration
    ;

parameter_declaration
    : declaration_specifiers declarator
    | declaration_specifiers abstract_declarator
    | declaration_specifiers
    ;

identifier_list
    : T_ID
    | identifier_list T_COMMA T_ID
    ;

type_name
    : specifier_qualifier_list
    | specifier_qualifier_list abstract_declarator
    ;

abstract_declarator
    : pointer
    | pointer direct_abstract_declarator
    | direct_abstract_declarator
    ;

direct_abstract_declarator
    : T_LPAREN abstract_declarator T_RPAREN
    | T_LBRACKET T_RBRACKET
    | T_LBRACKET constant_expression T_RBRACKET
    | direct_abstract_declarator T_LBRACKET T_RBRACKET
    | direct_abstract_declarator T_LBRACKET constant_expression T_RBRACKET
    | T_LPAREN T_RPAREN
    | T_LPAREN parameter_type_list T_RPAREN
    | direct_abstract_declarator T_LPAREN T_RPAREN
    | direct_abstract_declarator T_LPAREN parameter_type_list T_RPAREN
    ;

initializer
    : assignment_expression
    | T_LBRACE initializer_list T_RBRACE
    | T_LBRACE initializer_list T_COMMA T_RBRACE
    ;

initializer_list
    : initializer
    | initializer_list T_COMMA initializer
    ;

statement
    : labeled_statement
    | compound_statement
    | expression_statement
    | selection_statement
    | iteration_statement
    | jump_statement
    ;

labeled_statement
    : T_ID T_COLON statement
    | T_CASE constant_expression T_COLON statement
    | T_DEFAULT T_COLON statement
    ;

compound_statement
    : T_LBRACE T_RBRACE
    | T_LBRACE statement_list T_RBRACE
    | T_LBRACE declaration_list T_RBRACE
    | T_LBRACE declaration_list statement_list T_RBRACE
    ;

declaration_list
    : declaration
    | declaration_list declaration
    ;

statement_list
    : statement
    | statement_list statement
    ;

expression_statement
    : T_SEMICOLON
    | expression T_SEMICOLON
    ;

selection_statement
    : T_IF T_LPAREN expression T_RPAREN statement
    | T_IF T_LPAREN expression T_RPAREN statement T_ELSE statement
    | T_SWITCH T_LPAREN expression T_RPAREN statement
    ;

iteration_statement
    : T_WHILE T_LPAREN expression T_RPAREN statement
    | T_DO statement T_WHILE T_LPAREN expression T_RPAREN T_SEMICOLON
    | T_FOR T_LPAREN expression_statement expression_statement T_RPAREN statement
    | T_FOR T_LPAREN expression_statement expression_statement expression T_RPAREN statement
    ;

jump_statement
    : T_GOTO T_ID T_SEMICOLON
    | T_CONTINUE T_SEMICOLON
    | T_BREAK T_SEMICOLON
    | T_RETURN T_SEMICOLON
    | T_RETURN expression T_SEMICOLON
    ;

expression
    : assignment_expression
    | expression T_COMMA assignment_expression
    ;

assignment_expression
    : conditional_expression
    | unary_expression assignment_operator assignment_expression
    ;

assignment_operator
    : T_ASSIGN
    | T_STAREQ
    | T_SLASHEQ
    | T_PERCENTEQ
    | T_PLUSEQ
    | T_MINUSEQ
    | T_LSHIFTEQ
    | T_RSHIFTEQ
    | T_ANDEQ
    | T_XOREQ
    | T_OREQ
    ;

conditional_expression
    : logical_or_expression
    | logical_or_expression T_QUESTION expression T_COLON conditional_expression
    ;

constant_expression
    : conditional_expression
    ;

logical_or_expression
    : logical_and_expression
    | logical_or_expression T_LOGOR logical_and_expression
    ;

logical_and_expression
    : inclusive_or_expression
    | logical_and_expression T_LOGAND inclusive_or_expression
    ;

inclusive_or_expression
    : exclusive_or_expression
    | inclusive_or_expression T_PIPE exclusive_or_expression
    ;

exclusive_or_expression
    : and_expression
    | exclusive_or_expression T_CARET and_expression
    ;

and_expression
    : equality_expression
    | and_expression T_AMPERSAND equality_expression
    ;

equality_expression
    : relational_expression
    | equality_expression T_EQ relational_expression
    | equality_expression T_NEQ relational_expression
    ;

relational_expression
    : shift_expression
    | relational_expression T_LT shift_expression
    | relational_expression T_GT shift_expression
    | relational_expression T_LTE shift_expression
    | relational_expression T_GTE shift_expression
    ;

shift_expression
    : additive_expression
    | shift_expression T_LSHIFT additive_expression
    | shift_expression T_RSHIFT additive_expression
    ;

additive_expression
    : multiplicative_expression
    | additive_expression T_PLUS multiplicative_expression
    | additive_expression T_MINUS multiplicative_expression
    ;

multiplicative_expression
    : cast_expression
    | multiplicative_expression T_STAR cast_expression
    | multiplicative_expression T_SLASH cast_expression
    | multiplicative_expression T_PERCENT cast_expression
    ;

cast_expression
    : unary_expression
    | T_LPAREN type_name T_RPAREN cast_expression
    ;

unary_expression
    : postfix_expression
    | T_INC unary_expression
    | T_DEC unary_expression
    | unary_operator cast_expression
    | T_SIZEOF unary_expression
    | T_SIZEOF T_LPAREN type_name T_RPAREN
    ;

unary_operator
    : T_AMPERSAND
    | T_STAR
    | T_PLUS
    | T_MINUS
    | T_TILDE
    | T_EXCLAMATION
    ;

postfix_expression
    : primary_expression
    | postfix_expression T_LBRACKET expression T_RBRACKET
    | postfix_expression T_LPAREN T_RPAREN
    | postfix_expression T_LPAREN argument_expression_list T_RPAREN
    | postfix_expression T_DOT T_ID
    | postfix_expression T_ARROW T_ID
    | postfix_expression T_INC
    | postfix_expression T_DEC
    ;

primary_expression
    : T_ID
    | T_LITINT
    | T_LITFLOAT
    | T_LITCHAR
    | T_LITSTRING
    | T_LPAREN expression T_RPAREN
    ;

argument_expression_list
    : assignment_expression
    | argument_expression_list T_COMMA assignment_expression
    ;

%%

int main(int argc, char **argv) {
    yyparse();
    return 0;
}

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
}