# Project directories
SRCDIR = src
OBJDIR = obj
BINDIR = bin
TESTDIR = tdir
DOCDIR = docs
# INCDIR = include

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -Og -Wall -Wextra -std=c99

# Source files
SRC = $(SRCDIR)/cc0.c $(SRCDIR)/cc0t.c $(SRCDIR)/cc1.c $(SRCDIR)/sstore.c $(SRCDIR)/tstore.c $(SRCDIR)/astore.c $(SRCDIR)/hash.c $(SRCDIR)/symtab.c $(SRCDIR)/hmapbuf.c

# Object files
OBJ0 = $(OBJDIR)/cc0.o $(OBJDIR)/sstore.o $(OBJDIR)/tstore.o $(OBJDIR)/hash.o
OBJ0t = $(OBJDIR)/cc0t.o $(OBJDIR)/sstore.o $(OBJDIR)/tstore.o $(OBJDIR)/hash.o
OBJ1 = $(OBJDIR)/cc1.o $(OBJDIR)/sstore.o $(OBJDIR)/tstore.o $(OBJDIR)/astore.o $(OBJDIR)/hash.o $(OBJDIR)/symtab.o $(OBJDIR)/hmapbuf.o

# Output executable
OUT0 = $(BINDIR)/cc0
OUT0t = $(BINDIR)/cc0t
OUT1 = $(BINDIR)/cc1

.depend: $(SRC)
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

$(OUT1): $(OBJ1)
	$(CC) $(CFLAGS) -o $(OUT1) $(OBJ1)

$(OUT0t): $(OBJ0t)
	$(CC) $(CFLAGS) -o $(OUT0t) $(OBJ0t)

# Pattern rule to compile .c files to .o files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean up build files
clean:
	rm -fr $(OBJDIR) $(BINDIR) $(TESTDIR) $(DOCDIR) .depend

test: all
	rm -rf $(TESTDIR)
	mkdir -p $(TESTDIR)
	$(CC) $(CFLAGS) -E $(SRCDIR)/cc0.c > $(TESTDIR)/t.in
	$(OUT0) $(TESTDIR)/t.in $(TESTDIR)/sstore.out $(TESTDIR)/tokens.out 2>&1 > $(TESTDIR)/cc0.out
	$(OUT0t) $(TESTDIR)/sstore.out $(TESTDIR)/tokens.out > $(TESTDIR)/t.out
	$(OUT1) $(TESTDIR)/sstore.out $(TESTDIR)/tokens.out $(TESTDIR)/ast.out $(TESTDIR)/sym.out

# Phony targets
.PHONY: all clean doc test
