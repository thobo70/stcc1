/**
 * @file test_lexer_edge_cases.h
 * @brief Header for lexer edge case tests - break weak tokenization
 * 
 * Following PROJECT_MANIFEST.md principle: "FIX the code, not the test"
 * These tests use malformed inputs to stress-test the lexical analyzer.
 */

#ifndef TEST_LEXER_EDGE_CASES_H
#define TEST_LEXER_EDGE_CASES_H

// Lexer edge case test function prototypes
void test_lexer_malformed_identifiers(void);
void test_lexer_extreme_numeric_literals(void);
void test_lexer_malformed_string_literals(void);
void test_lexer_extreme_whitespace_comments(void);
void test_lexer_binary_and_control_chars(void);
void test_lexer_extremely_long_lines(void);
void test_lexer_operator_edge_cases(void);
void test_lexer_empty_and_minimal_files(void);
void test_lexer_line_counting_edge_cases(void);

// Main test runner
void run_lexer_edge_case_tests(void);

#endif // TEST_LEXER_EDGE_CASES_H
