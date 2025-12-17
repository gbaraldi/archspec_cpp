# Copyright 2024 archspec_cpp contributors
# SPDX-License-Identifier: (Apache-2.0 OR MIT)
#
# Cross-platform Makefile for archspec_cpp
# Supports: Linux, macOS, FreeBSD, Windows (with MinGW or MSVC via nmake)

# Detect OS
ifeq ($(OS),Windows_NT)
    UNAME := Windows
else
    UNAME := $(shell uname -s)
endif

# Compiler settings
CXX ?= g++
AR ?= ar

# Base flags (no exceptions/RTTI for smaller binaries)
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -O2 -fno-exceptions -fno-rtti -DJSON_NOEXCEPTION
INCLUDES = -I./include -I./extern/json/single_include

# Platform-specific settings
ifeq ($(UNAME),Darwin)
    # macOS
    LDFLAGS ?=
    SHARED_EXT = .dylib
    SHARED_FLAGS = -dynamiclib
    STATIC_EXT = .a
else ifeq ($(UNAME),Linux)
    # Linux
    LDFLAGS ?= -ldl
    SHARED_EXT = .so
    SHARED_FLAGS = -shared -fPIC
    STATIC_EXT = .a
else ifeq ($(UNAME),FreeBSD)
    # FreeBSD
    LDFLAGS ?=
    SHARED_EXT = .so
    SHARED_FLAGS = -shared -fPIC
    STATIC_EXT = .a
else ifeq ($(UNAME),Windows)
    # Windows (MinGW)
    LDFLAGS ?=
    SHARED_EXT = .dll
    SHARED_FLAGS = -shared
    STATIC_EXT = .a
    EXE_EXT = .exe
endif

# Directories
SRCDIR = src
INCDIR = include
TESTDIR = tests
EXAMPLEDIR = examples
BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj
LIBDIR = $(BUILDDIR)/lib
BINDIR = $(BUILDDIR)/bin

# Source files
SOURCES = $(SRCDIR)/cpuid.cpp $(SRCDIR)/microarchitecture.cpp $(SRCDIR)/detect.cpp $(SRCDIR)/archspec_c.cpp
OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))

# Library names
LIB_NAME = archspec
STATIC_LIB = $(LIBDIR)/lib$(LIB_NAME)$(STATIC_EXT)

# Test sources
TEST_SOURCES = $(wildcard $(TESTDIR)/*.cpp)
TEST_BINARIES = $(patsubst $(TESTDIR)/%.cpp,$(BINDIR)/%$(EXE_EXT),$(TEST_SOURCES))

# Example sources
EXAMPLE_SOURCES = $(wildcard $(EXAMPLEDIR)/*.cpp)
EXAMPLE_BINARIES = $(patsubst $(EXAMPLEDIR)/%.cpp,$(BINDIR)/%$(EXE_EXT),$(EXAMPLE_SOURCES))

# Default target
all: directories $(STATIC_LIB) examples tests

# Create directories
directories:
	@mkdir -p $(OBJDIR) $(LIBDIR) $(BINDIR)

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Build static library
$(STATIC_LIB): $(OBJECTS)
	$(AR) rcs $@ $^

# Build examples
examples: $(EXAMPLE_BINARIES) $(BINDIR)/c_api_example$(EXE_EXT)

$(BINDIR)/%$(EXE_EXT): $(EXAMPLEDIR)/%.cpp $(STATIC_LIB)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIBDIR) -l$(LIB_NAME) $(LDFLAGS) -o $@

# C example - compile as C, link with C++ runtime
$(BINDIR)/c_api_example$(EXE_EXT): $(EXAMPLEDIR)/c_api_example.c $(STATIC_LIB)
	$(CC) -Wall -Wextra -Werror -O2 $(INCLUDES) -c $< -o $(OBJDIR)/c_api_example.o
	$(CXX) $(OBJDIR)/c_api_example.o -L$(LIBDIR) -l$(LIB_NAME) $(LDFLAGS) -o $@

# Build tests
tests: $(TEST_BINARIES)

$(BINDIR)/%$(EXE_EXT): $(TESTDIR)/%.cpp $(STATIC_LIB)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIBDIR) -l$(LIB_NAME) $(LDFLAGS) -o $@

# Run tests
check: tests
	@echo "Running tests..."
	@for test in $(TEST_BINARIES); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done
	@echo "All tests passed!"

# Run examples
run-examples: examples
	@echo "Running examples..."
	@for example in $(EXAMPLE_BINARIES); do \
		echo "=== Running $$example ==="; \
		$$example; \
		echo ""; \
	done

# Clean
clean:
	rm -rf $(BUILDDIR)

# Install (optional, for system-wide installation)
PREFIX ?= /usr/local
install: $(STATIC_LIB)
	mkdir -p $(PREFIX)/lib $(PREFIX)/include/archspec
	cp $(STATIC_LIB) $(PREFIX)/lib/
	cp -r $(INCDIR)/archspec/* $(PREFIX)/include/archspec/

# Uninstall
uninstall:
	rm -f $(PREFIX)/lib/lib$(LIB_NAME)$(STATIC_EXT)
	rm -rf $(PREFIX)/include/archspec

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: all

# Generate compile_commands.json for IDE support
compile_commands: clean
	@which bear > /dev/null || (echo "bear not installed, skipping" && exit 0)
	bear -- $(MAKE) all

# Path to archspec JSON data (from submodule)
ARCHSPEC_JSON_DIR = extern/archspec/archspec/json/cpu

# Regenerate embedded JSON data (convenience target)
regenerate-data:
	@echo "Regenerating embedded JSON data from archspec submodule..."
	@echo '// Auto-generated embedded JSON data - DO NOT EDIT' > $(SRCDIR)/microarchitectures_data.inc
	@echo '// Generated from $(ARCHSPEC_JSON_DIR)/microarchitectures.json' >> $(SRCDIR)/microarchitectures_data.inc
	@echo '//' >> $(SRCDIR)/microarchitectures_data.inc
	@echo '// To regenerate: make regenerate-data' >> $(SRCDIR)/microarchitectures_data.inc
	@echo '' >> $(SRCDIR)/microarchitectures_data.inc
	@echo '#ifndef ARCHSPEC_MICROARCHITECTURES_DATA_INC' >> $(SRCDIR)/microarchitectures_data.inc
	@echo '#define ARCHSPEC_MICROARCHITECTURES_DATA_INC' >> $(SRCDIR)/microarchitectures_data.inc
	@echo '' >> $(SRCDIR)/microarchitectures_data.inc
	@echo 'static const char* MICROARCHITECTURES_JSON = R"JSON_DATA(' >> $(SRCDIR)/microarchitectures_data.inc
	@cat $(ARCHSPEC_JSON_DIR)/microarchitectures.json >> $(SRCDIR)/microarchitectures_data.inc
	@echo ')JSON_DATA";' >> $(SRCDIR)/microarchitectures_data.inc
	@echo '' >> $(SRCDIR)/microarchitectures_data.inc
	@echo '#endif // ARCHSPEC_MICROARCHITECTURES_DATA_INC' >> $(SRCDIR)/microarchitectures_data.inc
	@echo "Done!"

# Source files for formatting
FORMAT_SOURCES = $(shell find src include examples tests \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' \) 2>/dev/null)

# Format source code
format:
	@echo "Formatting source files..."
	@for f in $(FORMAT_SOURCES); do clang-format -i "$$f"; done
	@echo "Done!"

# Check formatting (for CI)
format-check:
	@echo "Checking format..."
	@for f in $(FORMAT_SOURCES); do clang-format --dry-run --Werror "$$f" || exit 1; done
	@echo "Format OK!"

.PHONY: all directories examples tests check run-examples clean install uninstall debug compile_commands regenerate-data format format-check

