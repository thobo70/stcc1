# STCC1 Small C Compiler - Restructured Source Organization
# Project directories
SRCDIR = src
LEXER_SRC = $(SRCDIR)/lexer
PARSER_SRC = $(SRCDIR)/parser
AST_SRC = $(SRCDIR)/ast
STORAGE_SRC = $(SRCDIR)/storage
ERROR_SRC = $(SRCDIR)/error
UTILS_SRC = $(SRCDIR)/utils
DEMO_SRC = $(SRCDIR)/demo

OBJDIR = obj
BINDIR = bin
TESTDIR = tdir
DOCDIR = docs

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Og -Wall -Wextra -std=c99

# Source files with new directory structure
SRC = $(LEXER_SRC)/cc0.c $(LEXER_SRC)/cc0t.c $(PARSER_SRC)/cc1.c $(STORAGE_SRC)/sstore.c $(STORAGE_SRC)/tstore.c $(STORAGE_SRC)/astore.c $(UTILS_SRC)/hash.c $(STORAGE_SRC)/symtab.c $(UTILS_SRC)/hmapbuf.c

# Enhanced source files - AST and advanced parsing components
ENHANCED_SRC = $(AST_SRC)/ast_builder.c $(ERROR_SRC)/error_core.c $(ERROR_SRC)/error_stages.c

# Object files
OBJ0 = $(OBJDIR)/cc0.o $(OBJDIR)/sstore.o $(OBJDIR)/tstore.o $(OBJDIR)/hash.o
OBJ0t = $(OBJDIR)/cc0t.o $(OBJDIR)/sstore.o $(OBJDIR)/tstore.o $(OBJDIR)/hash.o

# Enhanced cc1 with core features (simplified AST integration)
OBJ1 = $(OBJDIR)/cc1.o $(OBJDIR)/sstore.o $(OBJDIR)/tstore.o $(OBJDIR)/astore.o $(OBJDIR)/hash.o $(OBJDIR)/symtab.o $(OBJDIR)/hmapbuf.o $(OBJDIR)/error_core.o $(OBJDIR)/error_stages.o

# Output executable
OUT0 = $(BINDIR)/cc0
OUT0t = $(BINDIR)/cc0t
OUT1 = $(BINDIR)/cc1

# Dependency generation for all source files including enhanced components
.depend: $(SRC) $(ENHANCED_SRC)
	gcc -MM $^ > .depend

include .depend

# Default target
all: $(OBJDIR) $(BINDIR) $(OUT0) $(OUT0t) $(OUT1)

# Doxygen documentation
$(DOCDIR)/html/index.html: $(DOCDIR) Doxyfile $(SRC)
	doxygen Doxyfile

doc: $(DOCDIR)/html/index.html

# Create the output directories
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

$(DOCDIR):
	mkdir -p $(DOCDIR)

# Build the executable
$(OUT0): $(OBJ0)
	$(CC) $(CFLAGS) -o $(OUT0) $(OBJ0)

# Enhanced cc1 with all advanced features: error handling, AST building, and enhanced parsing
$(OUT1): $(OBJ1)
	$(CC) $(CFLAGS) -o $(OUT1) $(OBJ1)

$(OUT0t): $(OBJ0t)
	$(CC) $(CFLAGS) -o $(OUT0t) $(OBJ0t)

# Pattern rules to compile .c files to .o files with new directory structure
$(OBJDIR)/cc0.o: $(LEXER_SRC)/cc0.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/cc0t.o: $(LEXER_SRC)/cc0t.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/cc1.o: $(PARSER_SRC)/cc1.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/sstore.o: $(STORAGE_SRC)/sstore.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/tstore.o: $(STORAGE_SRC)/tstore.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/astore.o: $(STORAGE_SRC)/astore.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/symtab.o: $(STORAGE_SRC)/symtab.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/hash.o: $(UTILS_SRC)/hash.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/hmapbuf.o: $(UTILS_SRC)/hmapbuf.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/error_core.o: $(ERROR_SRC)/error_core.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/error_stages.o: $(ERROR_SRC)/error_stages.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/ast_builder.o: $(AST_SRC)/ast_builder.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean up build files
clean:
	rm -fr $(OBJDIR) $(BINDIR) $(TESTDIR) $(DOCDIR) .depend

test: all
	rm -rf $(TESTDIR)
	mkdir -p $(TESTDIR)
	$(CC) $(CFLAGS) -E $(LEXER_SRC)/cc0.c > $(TESTDIR)/t.in
	$(OUT0) $(TESTDIR)/t.in $(TESTDIR)/sstore.out $(TESTDIR)/tokens.out 2>&1 > $(TESTDIR)/cc0.out
	$(OUT0t) $(TESTDIR)/sstore.out $(TESTDIR)/tokens.out > $(TESTDIR)/t.out
	$(OUT1) $(TESTDIR)/sstore.out $(TESTDIR)/tokens.out $(TESTDIR)/ast.out $(TESTDIR)/sym.out

# Phony targets
.PHONY: all clean doc test
