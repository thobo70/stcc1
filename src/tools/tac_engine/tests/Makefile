# TAC Engine Test Suite Makefile
# Tests TAC ENGINE functionality ONLY - Manifest Compliant
# PHILOSOPHY: Strong tests that break weak TAC engine code

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -g -O2
INCLUDES = -I.. -I../../../../Unity/src

# Build directories
BUILD_DIR = ../build/tests
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files - TAC ENGINE tests only
ENGINE_TEST_SRCS = test_tac_engine_unified.c test_tac_engine_debugging.c
ENGINE_TEST_OBJS = $(ENGINE_TEST_SRCS:%.c=$(OBJ_DIR)/%.o)

# Unity framework
UNITY_SRCS = ../../../../Unity/src/unity.c
UNITY_OBJS = $(OBJ_DIR)/unity.o

# Dependencies
TAC_ENGINE_LIB = ../build/lib/libtac_engine.a

# Main test executable
ENGINE_TEST_EXEC = $(BIN_DIR)/tac_engine_tests

# Default target
all: directories $(ENGINE_TEST_EXEC)

# Create directories
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Build TAC engine test suite
$(ENGINE_TEST_EXEC): $(ENGINE_TEST_OBJS) $(UNITY_OBJS) $(TAC_ENGINE_LIB)
	@echo "Building TAC Engine tests..."
	@$(CC) $(CFLAGS) -o $@ $^

# Build Unity
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

# Main test target - tests TAC ENGINE functionality
test: $(ENGINE_TEST_EXEC)
	@echo "Running TAC Engine Tests..."
	@./$(ENGINE_TEST_EXEC)

# Clean test artifacts
clean:
	rm -f $(ENGINE_TEST_OBJS) $(UNITY_OBJS)
	rm -f $(ENGINE_TEST_EXEC)
	rm -rf $(BUILD_DIR)

# Clean everything including library
clean-all: clean
	$(MAKE) -C .. clean

# Help target
help:
	@echo "TAC Engine Test Suite - Manifest Compliant"
	@echo "Available targets:"
	@echo "  test      - Run TAC engine functionality tests (includes debugging)"
	@echo "  clean     - Remove test artifacts"
	@echo "  clean-all - Remove everything including TAC engine library"
	@echo "  help      - Show this help"

.PHONY: all test clean clean-all help directories
