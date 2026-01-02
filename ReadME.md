# ALPACA &middot; [![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)


ALPACA (Automated Library and Program API Change Analyzer) is a Clang compiler-frontend based tool designed to detect and analyze changes between different versions of C/C++ source code. By leveraging AST-based semantic information from the Clang compiler combined with syntactic similarity analysis, ALPACA can identify and classify various types of code changes and refactorings.

## Key Features

- Detection of various entity (functions, variables, records) changes between two versions of a code base
- Support for complex C/C++ codebases (>1M LOC)
- High accuracy (92% change detection rate, 91% correct classification) -> dont know if we can/should write that in before the paper is published
- Can handle template specializations and overloaded functions
- Built on Clang LibTooling for robust C/C++ parsing

## Build Requirements

### Core Requirements
- CMake 3.0 or higher
- LLVM/Clang 12
- C++17 compatible compiler

## Building ALPACA

### Native Build

```bash
# Clone the repository
git clone https://github.com/tudasc/Alpaca
cd ALPACA

# Configure with CMake
cmake -B build

# Build
cmake --build build

# Verify LLVM/Clang version
./build/APIAnalysis --help
```

### Exemplary Docker Build

We provide a Dockerfile that sets up an example from the evaluation with an OpenMPI codebase analysis and all the required build dependencies:

```bash
# Build the Docker image
docker build -t alpaca .

# Run ALPACA in Docker
docker run -v $(pwd):/work -w /work alpaca APIAnalysis
```

## Usage

ALPACA requires two versions of the codebase to perform the analysis:

```bash
./APIAnalysis --oldDir=/path/to/old/version \
              --newDir=/path/to/new/version \
              [options]
```

### Common Options

- `--oldDir, --newDir`: Paths to the old and new versions of the project (required)
- `--oldCD, --newCD`: Paths to compilation databases
- `--extra-args`: Additional Clang arguments for both versions
- `--extra-args-old, --extra-args-new`: Version-specific Clang arguments
- `--exclude`: Directories/files to ignore (comma-separated)
- `--doc`: Enable the second analysis step using Levenshtein Matching
- `--json`: Output results in JSON format
- `--ipf`: Include private functions in analysis

### Using with Compilation Databases

For best results, provide compilation databases for both versions:

1. Generate compilation databases (for example using [Bear](https://github.com/rizsotto/Bear)):
```bash
cd old_version
bear -- make
cd ../new_version
bear -- make
```

2. Run analysis with compilation databases:
```bash
./APIAnalysis --oldDir=old_version \
              --newDir=new_version \
              --oldCD=old_version/compile_commands.json \
              --newCD=new_version/compile_commands.json
```

## Future Work (currently work in progress)

- Parallel Processing: Implementation of parallel extraction and analysis using OpenMPI to significantly reduce runtime for large codebases
- Robust Handling of Overloaded Functions: Reworking of C++ overloaded function handling for improved accuracy and coverage
- Extended Change Detection: Support for additional types of code changes and refactorings
- Reference Graph Matcher: Introduction of a new matching algorithm using Reference Graphs to improve entity matching between project versions

## License

ALPACA is licensed under the BSD 3-Clause License. See LICENSE for details.
