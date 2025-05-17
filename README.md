# Badem

**A collection of C++ static analysis tools built with Clang.**  
While primarily designed for use with the [Zeytin](https://github.com/berkaysahiin/Zeytin), these utilities can be adapted for other C++ codebases.

## Overview

Badem is a growing collection of static analysis tools that examines C++ code. The current implementation focuses on identifying and tracking query calls between variant classes in the Zeytin Engine ecosystem. It analyzes methods like `on_init`, `on_update`, and other lifecycle functions to extract read and write dependencies between components, generating dependency reports as CSV files.

This project will eventually include multiple Clang-based utilities for code analysis, refactoring, and validation.

## Features

### Current Features

- Analyzes tracked lifecycle methods (`on_init`, `on_post_init`, `on_update`, etc.)
- Detects `Query` read/write operations on `VariantBase` pointers
- Extracts template argument dependencies
- Generates `.astrequires` CSV files with read/write dependencies for each variant class

### Planned Features

- Code pattern detection
- Custom linting rules
- Automated refactoring tools
- Interface compliance validation

## Prerequisites

- LLVM/Clang 14
- C++17 compatible compiler
- [premake5](https://premake.github.io/)

## Building

```bash
# Generate build files
premake5 gmake

# Build the project
cd build
make
```

## Usage

```bash
./bin/Debug/method-analyzer --dir=/path/to/your/project
```





