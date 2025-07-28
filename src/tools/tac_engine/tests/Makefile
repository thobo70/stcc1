# TAC Engine Test Suite Makefile
# Comprehensive Unity test framework for TAC Engine
# Following PROJECT_MANIFEST.md: NEVER MIX SOURCES WITH BUILD ARTIFACTS

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -g -O2
INCLUDES = -I.. -I../../../ -I../../../ir -I../../../utils -I../../../storage -I../../../../Unity/src -I../../../tests

# Build directories (NEVER mix with sources!)
BUILD_DIR = ../build/tests
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files
TEST_SRCS = test_tac_engine_main.c \
           test_tac_engine_lifecycle.c \
           test_tac_engine_execution.c \
           test_tac_engine_debugging.c \
           test_tac_engine_edge_cases.c \
           test_tac_engine_working.c

TEST_OBJS = $(TEST_SRCS:%.c=$(OBJ_DIR)/%.o)

# Unity framework source
UNITY_SRCS = ../../../../Unity/src/unity.c

# Dependencies
TAC_ENGINE_LIB = ../build/lib/libtac_engine.a
UNITY_OBJS = $(OBJ_DIR)/unity.o

# Executables (in build directory!)
UNITY_TEST_EXEC = $(BIN_DIR)/tac_engine_unity_tests
WORKING_TEST_EXEC = $(BIN_DIR)/tac_engine_working_tests

# Default target
all: directories $(UNITY_TEST_EXEC) $(WORKING_TEST_EXEC)

# Create build directories
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Build Unity test suite
$(UNITY_TEST_EXEC): $(TEST_OBJS) $(UNITY_OBJS) $(TAC_ENGINE_LIB)
	$(CC) $(CFLAGS) -o $@ $^

# Build working test suite
$(WORKING_TEST_EXEC): $(OBJ_DIR)/test_working_runner.o $(OBJ_DIR)/test_tac_engine_working.o $(UNITY_OBJS) $(TAC_ENGINE_LIB)
	$(CC) $(CFLAGS) -o $@ $^

# Build Unity object file
$(UNITY_OBJS): $(UNITY_SRCS)
	$(CC) $(CFLAGS) -I../../../../Unity/src -c $(UNITY_SRCS) -o $(UNITY_OBJS)

# Compile test files
$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Ensure TAC Engine library is built
$(TAC_ENGINE_LIB):
	$(MAKE) -C .. $(shell basename $(TAC_ENGINE_LIB))

# Run all tests
test: $(UNITY_TEST_EXEC)
	./$(UNITY_TEST_EXEC)

# Run all working tests
test-working: $(WORKING_TEST_EXEC)
	./$(WORKING_TEST_EXEC)

# Individual test categories
test-lifecycle: $(UNITY_TEST_EXEC)
	./$(UNITY_TEST_EXEC) lifecycle

test-execution: $(UNITY_TEST_EXEC)
	./$(UNITY_TEST_EXEC) execution

test-debugging: $(UNITY_TEST_EXEC)
	./$(UNITY_TEST_EXEC) debugging

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
	@echo "TAC Engine Test Suite"
	@echo "===================="
	@echo ""
	@echo "Targets:"
	@echo "  all           - Build test suite (default)"
	@echo "  test          - Run all tests"
	@echo "  test-working  - Run working tests (demonstrates framework)"
	@echo "  test-*        - Run specific test categories:"
	@echo "    lifecycle   - Engine creation/destruction tests"
	@echo "    execution   - TAC instruction execution tests"
	@echo "    debugging   - Breakpoint, hook, tracing tests"
	@echo "    edge        - Edge case and boundary tests"
	@echo "    stress      - Stress and resource exhaustion tests"
	@echo "  clean         - Remove test artifacts"
	@echo "  clean-all     - Remove test artifacts and library"
	@echo "  help          - Show this help"
	@echo ""
	@echo "Test Philosophy:"
	@echo "  Following PROJECT_MANIFEST.md principles:"
	@echo "  - NEVER weaken a test to make it pass"
	@echo "  - FIX the code, not the test"
	@echo "  - Create strong tests that break weak code"

# Dependencies
$(OBJ_DIR)/test_tac_engine_main.o: test_tac_engine_main.c test_tac_engine.h
$(OBJ_DIR)/test_tac_engine_lifecycle.o: test_tac_engine_lifecycle.c test_tac_engine.h
$(OBJ_DIR)/test_tac_engine_execution.o: test_tac_engine_execution.c test_tac_engine.h
$(OBJ_DIR)/test_tac_engine_debugging.o: test_tac_engine_debugging.c test_tac_engine.h
$(OBJ_DIR)/test_tac_engine_edge_cases.o: test_tac_engine_edge_cases.c test_tac_engine.h
$(OBJ_DIR)/test_tac_engine_working.o: test_tac_engine_working.c test_tac_engine.h
$(OBJ_DIR)/test_working_runner.o: test_working_runner.c test_tac_engine.h

.PHONY: all test test-working test-lifecycle test-execution test-debugging test-edge test-stress clean clean-all help directories
