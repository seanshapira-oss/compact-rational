# Makefile for CompactRational Library and Programs

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = -lm

# Library
LIB_SRC = compact_rational_lib.c
LIB_OBJ = compact_rational_lib.o
LIB_HEADER = compact_rational.h

# Programs that use the library
PROGS_WITH_LIB = compact_rational test_e_representation canonicalize
PROGS_WITH_LIB_SRC = $(addsuffix .c, $(PROGS_WITH_LIB))

# Analysis programs (library-based, optional - use 'make analysis')
ANALYSIS_PROGS = analyze_e_fraction find_e_convergents test_best_e_convergent
ANALYSIS_SRC = $(addsuffix .c, $(ANALYSIS_PROGS))

# Standalone programs (don't use the library)
STANDALONE_PROGS = find_best_e optimal_encoding
STANDALONE_SRC = $(addsuffix .c, $(STANDALONE_PROGS))

# All programs
ALL_PROGS = $(PROGS_WITH_LIB) $(STANDALONE_PROGS)

# Default target: build everything
all: $(LIB_OBJ) $(ALL_PROGS)

# Build the library object file
$(LIB_OBJ): $(LIB_SRC) $(LIB_HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

# Build programs that use the library
$(PROGS_WITH_LIB): %: %.c $(LIB_OBJ) $(LIB_HEADER)
	$(CC) $(CFLAGS) $< $(LIB_OBJ) $(LDFLAGS) -o $@

# Build standalone programs
$(STANDALONE_PROGS): %: %.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

# Build analysis programs
$(ANALYSIS_PROGS): %: %.c $(LIB_OBJ) $(LIB_HEADER)
	$(CC) $(CFLAGS) $< $(LIB_OBJ) $(LDFLAGS) -o $@

# Build all analysis programs
analysis: $(LIB_OBJ) $(ANALYSIS_PROGS)

# Clean build artifacts
clean:
	rm -f $(LIB_OBJ) $(ALL_PROGS) $(ANALYSIS_PROGS)

# Test all programs
test: all
	@echo "=== Testing compact_rational ==="
	./compact_rational
	@echo ""
	@echo "=== Testing canonicalize ==="
	./canonicalize
	@echo ""
	@echo "=== Testing test_e_representation ==="
	./test_e_representation

# Help
help:
	@echo "CompactRational Library Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build library and all programs (default)"
	@echo "  analysis - Build analysis/research programs"
	@echo "  clean    - Remove all build artifacts"
	@echo "  test     - Build and run test programs"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Programs:"
	@echo "  Library-based:"
	@echo "    compact_rational       - Main test suite"
	@echo "    test_e_representation  - Test e constant representations"
	@echo "    canonicalize           - Test canonicalization"
	@echo ""
	@echo "  Analysis (build with 'make analysis'):"
	@echo "    analyze_e_fraction     - Analyze arbitrary e fraction approximations"
	@echo "    find_e_convergents     - Find continued fraction convergents of e"
	@echo "    test_best_e_convergent - Test superior e convergents in CompactRational"
	@echo ""
	@echo "  Standalone:"
	@echo "    find_best_e            - Find optimal e representations"
	@echo "    optimal_encoding       - Explore encoding strategies"

.PHONY: all clean test help analysis
