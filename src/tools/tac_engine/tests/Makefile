# TAC Engine Test Suite Makefile
# Unified test framework following PROJECT_MANIFEST.md
# PHILOSOPHY: One test target, one test runner, strong tests that break weak code

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -g -O2
INCLUDES = -I.. -I../../../ -I../../../ir -I../../../utils -I../../../storage -I../../../../Unity/src -I../../../tests

# Build directories (NEVER mix with sources!)
BUILD_DIR = ../build/tests
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files - ONE unified test runner
TEST_SRCS = test_tac_engine_unified.c

TEST_OBJS = $(TEST_SRCS:%.c=$(OBJ_DIR)/%.o)

# Unity framework source
UNITY_SRCS = ../../../../Unity/src/unity.c

# Dependencies
TAC_ENGINE_LIB = ../build/lib/libtac_engine.a
UNITY_OBJS = $(OBJ_DIR)/unity.o

# Executables - ONE test executable
TEST_EXEC = $(BIN_DIR)/tac_engine_tests

# Default target
all: directories $(TEST_EXEC)

# Create build directories
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Build unified test suite
$(TEST_EXEC): $(TEST_OBJS) $(UNITY_OBJS) $(TAC_ENGINE_LIB)
	@echo "Building test suite..."
	@$(CC) $(CFLAGS) -o $@ $^

# Build working test suite
# Build working test suite
$(WORKING_TEST_EXEC): $(OBJ_DIR)/test_tac_engine_working.o $(UNITY_OBJS) $(TAC_ENGINE_LIB)
	$(CC) $(CFLAGS) -o $@ $^

# Build comprehensive test suite
$(COMPREHENSIVE_TEST_EXEC): $(COMPREHENSIVE_TEST_OBJS) $(UNITY_OBJS) $(TAC_ENGINE_LIB)
	$(CC) $(CFLAGS) -o $@ $^

# Build Unity object file
$(UNITY_OBJS): $(UNITY_SRCS) | directories
	@echo "Building Unity framework..."
	@$(CC) $(CFLAGS) -I../../../../Unity/src -c $(UNITY_SRCS) -o $(UNITY_OBJS)

# Compile test files
$(OBJ_DIR)/%.o: %.c | directories
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Ensure TAC Engine library is built
$(TAC_ENGINE_LIB):
	$(MAKE) -C .. $(shell basename $(TAC_ENGINE_LIB))

# Run tests - ONE test target as requested
test: $(TEST_EXEC)
	@echo "Running TAC Engine Tests..."
	@./$(TEST_EXEC)

test-edge: $(UNITY_TEST_EXEC)
	./$(UNITY_TEST_EXEC) edge

test-stress: $(UNITY_TEST_EXEC)
	./$(UNITY_TEST_EXEC) stress

# Clean test artifacts
clean:
	rm -f $(TEST_OBJS) $(UNITY_OBJS) $(UNITY_TEST_EXEC) $(WORKING_TEST_EXEC)
	rm -rf $(BUILD_DIR)

# Clean everything including library
clean-all: clean
	$(MAKE) -C .. clean

# Show help
help:
	@echo "TAC Engine Test Suite - Manifest Compliant"
	@echo "==========================================="
	@echo ""
	@echo "Targets:"
	@echo "  all            - Build test suite"
	@echo "  test           - Run all tests (unified test runner)"
	@echo "  clean          - Clean test build files"
	@echo "  clean-all      - Clean everything including dependencies"
	@echo "  help           - Show this help"
	@echo ""
	@echo "Philosophy: One test target, one test runner, strong tests that break weak code"
	@echo ""

.PHONY: all test clean clean-all help directories

# Dependencies
$(OBJ_DIR)/test_tac_engine_main.o: test_tac_engine_main.c test_tac_engine.h
$(OBJ_DIR)/test_tac_engine_lifecycle.o: test_tac_engine_lifecycle.c test_tac_engine.h
$(OBJ_DIR)/test_tac_engine_execution.o: test_tac_engine_execution.c test_tac_engine.h
$(OBJ_DIR)/test_tac_engine_debugging.o: test_tac_engine_debugging.c test_tac_engine.h
$(OBJ_DIR)/test_tac_engine_edge_cases.o: test_tac_engine_edge_cases.c test_tac_engine.h
$(OBJ_DIR)/test_tac_engine_working.o: test_tac_engine_working.c test_tac_engine.h
$(OBJ_DIR)/test_working_runner.o: test_working_runner.c test_tac_engine.h

.PHONY: all test test-working test-lifecycle test-execution test-debugging test-edge test-stress clean clean-all help directories
