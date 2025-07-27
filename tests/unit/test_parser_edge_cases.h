/**
 * @file test_parser_edge_cases.h
 * @brief Header for parser edge case tests - break weak parsing
 * 
 * Following PROJECT_MANIFEST.md principle: "FIX the code, not the test"
 * These tests use invalid syntax to stress-test the parser.
 */

#ifndef TEST_PARSER_EDGE_CASES_H
#define TEST_PARSER_EDGE_CASES_H

// Parser edge case test function prototypes
void test_parser_deeply_nested_expressions(void);
void test_parser_malformed_declarations(void);
void test_parser_operator_precedence_edge_cases(void);
void test_parser_unmatched_delimiters(void);
void test_parser_extreme_control_structure_nesting(void);
void test_parser_invalid_function_definitions(void);
void test_parser_symbol_table_stress(void);
void test_parser_memory_exhaustion(void);
void test_parser_error_recovery(void);
void test_parser_empty_token_stream(void);

// Main test runner
void run_parser_edge_case_tests(void);

#endif // TEST_PARSER_EDGE_CASES_H
