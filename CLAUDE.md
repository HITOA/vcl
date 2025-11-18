# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

VCL (Vector Computation Language) is a C++ compiler project heavily inspired by Clang's architecture. It implements a custom language with SIMD vector operations, templating, and LLVM-based code generation.

## Build System

### Prerequisites
- CMake 3.16+
- C++23 compliant compiler
- LLVM development libraries
- Catch2 3.x (for tests)
- Doxygen (for documentation)

### Build Commands
```bash
# Configure and build
mkdir build && cd build
cmake ..
make

# Build with tests
cmake -DVCL_BUILD_TEST=ON ..
make

# Build with documentation
cmake -DVCL_BUILD_DOC=ON ..
make

# Run tests (if built)
./test/vcl_test

# Run compiler
./cvcl/cvcl -i input.vcl
```

### Development Environment
The project uses Nix for development environment setup:
```bash
nix-shell  # Enters development shell with all dependencies
```

## Architecture

### Core Components

**Frontend Pipeline:**
- `CompilerContext` - Central coordinator for compilation resources
- `CompilerInstance` - Per-compilation instance managing the pipeline
- `SourceManager` - File and source location management
- `Lexer` - Tokenization of source code
- `Parser` - Syntax analysis and AST construction
- `Sema` - Semantic analysis and type checking

**AST System:**
- `ASTContext` - Memory management for AST nodes using bump allocator
- `TypeCache` - Centralized type management and canonicalization
- AST node hierarchy under `include/VCL/AST/`

**Code Generation:**
- `CodeGenModule` - LLVM module generation
- `CodeGenFunction` - Function-level code generation
- `ExecutionSession` - JIT compilation and execution

### Key Design Patterns

1. **Clang-inspired Architecture**: Follows Clang's multi-phase compilation model
2. **RAII Resource Management**: Extensive use of LLVM's reference counting and allocators
3. **Template System**: Full template support with specialization
4. **SIMD-first Design**: Native vector types with hardware acceleration

### Language Features

VCL supports:
- Primitive types (int8-64, uint8-64, float32/64, bool)
- SIMD vector types: `Vec<T>`
- Fixed arrays: `Array<T, Size>`
- Memory views: `Span<T>`
- Structs with member functions
- Templates (functions and structs)
- Namespaces with `using` declarations
- Qualified globals: `in`, `out`, `inout`

### File Organization

- `include/VCL/` - Public headers organized by component
- `src/` - Implementation files mirroring header structure  
- `cvcl/` - Compiler executable
- `test/` - Unit tests using Catch2
- `vcl/` - Example VCL language files

### Testing

Uses Catch2 framework with tests organized by component. Test files are located in `test/` directory and copy VCL source files to build directory for testing.

### Diagnostic System

Implements comprehensive error reporting with source location tracking, following Clang's diagnostic infrastructure patterns.