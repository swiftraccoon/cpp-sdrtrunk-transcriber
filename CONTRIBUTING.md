# Contributing to C++ SDRTrunk Transcriber

Thank you for your interest in contributing to the C++ SDRTrunk Transcriber project! This document provides guidelines and information for contributors.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Environment Setup](#development-environment-setup)
- [Project Architecture](#project-architecture)
- [Coding Standards](#coding-standards)
- [Testing Guidelines](#testing-guidelines)
- [Submitting Changes](#submitting-changes)
- [Issue Reporting](#issue-reporting)
- [Branch Naming Conventions](#branch-naming-conventions)
- [Commit Message Guidelines](#commit-message-guidelines)

## Code of Conduct

This project follows a standard code of conduct. Please be respectful and constructive in all interactions.

## Getting Started

### Prerequisites

- C++17 compatible compiler
- CMake 3.16+
- Git
- Basic understanding of audio processing and transcription concepts
- Familiarity with SDRTrunk and P25 radio systems (helpful but not required)

### Fork and Clone

1. Fork the repository on GitHub
2. Clone your fork locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/cpp-sdrtrunk-transcriber.git
   cd cpp-sdrtrunk-transcriber
   ```
3. Add the upstream repository:
   ```bash
   git remote add upstream https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber.git
   ```

## Development Environment Setup

### Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install build-essential cmake git pkg-config \
    ffmpeg libavcodec-dev libcurl4-openssl-dev \
    libsqlite3-dev libyaml-cpp-dev \
    valgrind gdb clang-format clang-tidy
```

**Fedora:**
```bash
sudo dnf install gcc-c++ cmake git pkg-config \
    ffmpeg ffmpeg-devel libcurl-devel \
    sqlite-devel yaml-cpp-devel \
    valgrind gdb clang-tools-extra
```

### Build for Development

```bash
# Clone external dependencies
git clone https://github.com/jbeder/yaml-cpp.git external/yaml-cpp
git clone https://github.com/CLIUtils/CLI11.git external/CLI11

# Debug build with tests enabled
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON \
    -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build build

# Run tests
cd build && ctest --output-on-failure
```

### IDE Setup

**VS Code (recommended):**
1. Install C/C++ extension
2. Install CMake Tools extension
3. Open the project folder
4. Use Ctrl+Shift+P → "CMake: Configure" to set up

**CLion:**
1. Open the project directory
2. CLion will automatically detect CMake configuration
3. Select Debug build configuration

## Project Architecture

### Core Components

- **Main Application** (`src/main.cpp`): Entry point and main loop
- **Configuration System** (`src/ConfigSingleton.cpp`): YAML-based configuration management
- **File Processing** (`src/fileProcessor.cpp`): SDRTrunk file parsing and validation
- **Transcription Processing** (`src/transcriptionProcessor.cpp`): Transcription logic and glossary handling
- **Database Management** (`src/DatabaseManager.cpp`): SQLite operations
- **HTTP Communication** (`src/curlHelper.cpp`): OpenAI API interaction
- **Local Transcription** (`src/fasterWhisper.cpp`): Local processing integration

### Key Data Structures

- **FileData** (`include/FileData.h`): Core data structure for file metadata
- **TalkgroupFiles** (`include/transcriptionProcessor.h`): Glossary mapping structure

### Design Patterns

- **Singleton Pattern**: Used for configuration management
- **RAII**: Consistent resource management throughout
- **Error Handling**: Comprehensive error handling with logging

## Coding Standards

### C++ Style Guidelines

We follow a modified version of the Google C++ Style Guide:

**Naming Conventions:**
```cpp
// Classes: PascalCase
class DatabaseManager {
    // Methods: camelCase
    void insertRecording();
    
    // Private members: camelCase with trailing underscore
    sqlite3* db_;
    std::string connectionString_;
};

// Functions: camelCase
void processAudioFile();

// Variables: camelCase
std::string fileName;
int talkgroupId;

// Constants: ALL_CAPS
const int MAX_RETRIES = 3;

// Enums: PascalCase with k prefix for values
enum class TranscriptionMode {
    kLocal,
    kOpenAI
};
```

**Code Organization:**
```cpp
// Header file structure
#pragma once

// Standard Library Headers (alphabetical)
#include <string>
#include <vector>

// Third-Party Library Headers (alphabetical)
#include <yaml-cpp/yaml.h>

// Project-Specific Headers (alphabetical)
#include "FileData.h"

class ExampleClass {
public:
    // Constructors first
    ExampleClass();
    explicit ExampleClass(const std::string& param);
    
    // Destructor
    ~ExampleClass();
    
    // Public methods
    void publicMethod();
    
private:
    // Private methods
    void privateMethod();
    
    // Member variables
    std::string data_;
    int count_;
};
```

**Formatting:**
- Use clang-format with the provided configuration
- 4-space indentation (no tabs)
- 100-character line limit
- Always use braces for control structures

### Code Quality

**Memory Management:**
- Prefer smart pointers over raw pointers
- Use RAII for resource management
- No memory leaks (verify with valgrind)

**Error Handling:**
- Use exceptions for exceptional conditions
- Return error codes for expected failure cases
- Always check return values from C APIs
- Log errors appropriately

**Performance:**
- Avoid premature optimization
- Profile before optimizing
- Use const references for function parameters
- Minimize copying of large objects

### Documentation

**Code Comments:**
```cpp
/**
 * @brief Brief description of the function
 * @param param1 Description of parameter 1
 * @param param2 Description of parameter 2
 * @return Description of return value
 * @throws std::exception Description of when exception is thrown
 */
int exampleFunction(const std::string& param1, int param2);

// Inline comments for complex logic
if (complexCondition) {
    // Explain why this condition exists
    performComplexOperation();
}
```

## Testing Guidelines

### Test Structure

- Tests are located in the `test/` directory
- Each source file should have corresponding tests
- Use Google Test framework

### Test Categories

**Unit Tests:**
```cpp
// test/test_fileProcessor.cpp
#include <gtest/gtest.h>
#include "fileProcessor.h"

TEST(FileProcessorTest, ParsesFilenameCorrectly) {
    // Arrange
    std::string filename = "20231201_143022_52197_123.mp3";
    
    // Act
    FileData result = parseSDRTrunkFilename(filename);
    
    // Assert
    EXPECT_EQ(result.talkgroupID, 52197);
    EXPECT_EQ(result.radioID, 123);
}
```

**Integration Tests:**
- Test component interactions
- Use temporary databases and files
- Clean up resources after tests

**Test Best Practices:**
- One assertion per test when possible
- Use descriptive test names
- Test both success and failure cases
- Mock external dependencies

### Running Tests

```bash
# Build and run all tests
cmake --build build
cd build && ctest --output-on-failure

# Run specific test
./test/test_fileProcessor

# Run with memory checking
valgrind --leak-check=full ./test/test_fileProcessor
```

## Submitting Changes

### Pull Request Process

1. **Create a feature branch:**
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes:**
   - Follow coding standards
   - Add tests for new functionality
   - Update documentation as needed

3. **Test your changes:**
   ```bash
   # Run tests
   cmake --build build && cd build && ctest
   
   # Check for memory leaks
   valgrind --leak-check=full ./build/sdrtrunk-transcriber --help
   
   # Format code
   clang-format -i src/*.cpp include/*.h
   
   # Static analysis
   clang-tidy src/*.cpp -- -Iinclude
   ```

4. **Commit your changes:**
   ```bash
   git add .
   git commit -m "feat: add new feature description"
   ```

5. **Push and create PR:**
   ```bash
   git push origin feature/your-feature-name
   ```
   Then create a pull request on GitHub.

### PR Requirements

- [ ] All tests pass
- [ ] Code follows style guidelines
- [ ] New functionality includes tests
- [ ] Documentation updated if needed
- [ ] No memory leaks detected
- [ ] Changes are backwards compatible (if applicable)

## Issue Reporting

### Before Submitting an Issue

1. Search existing issues to avoid duplicates
2. Try the latest version from main branch
3. Gather relevant information:
   - OS and version
   - Compiler and version
   - CMake version
   - Build configuration
   - Configuration file (sanitized)
   - Debug output (if available)

### Bug Reports

Use the bug report template and include:
- Clear description of the bug
- Steps to reproduce
- Expected vs actual behavior
- Environment details
- Log output (with sensitive information removed)
- Sample files (if relevant)

### Feature Requests

Use the feature request template and include:
- Clear description of the desired feature
- Use case and motivation
- Proposed implementation approach
- Potential alternatives considered
- Backwards compatibility considerations

## Branch Naming Conventions

- `feature/description` - New features
- `fix/description` - Bug fixes
- `docs/description` - Documentation updates
- `refactor/description` - Code refactoring
- `test/description` - Test improvements
- `chore/description` - Maintenance tasks

## Commit Message Guidelines

We follow the [Conventional Commits](https://conventionalcommits.org/) specification:

```
type(scope): description

[optional body]

[optional footer]
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or modifying tests
- `chore`: Maintenance tasks

**Examples:**
```
feat(transcription): add support for custom whisper models

fix(database): handle connection timeout gracefully

docs(readme): update installation instructions for Windows

test(fileProcessor): add tests for edge cases in filename parsing
```

**Rules:**
- Use present tense ("add feature" not "added feature")
- Use lowercase for type and description
- Keep description under 72 characters
- Reference issues when relevant: "fixes #123"

## Development Workflow

### Daily Development

1. **Sync with upstream:**
   ```bash
   git checkout main
   git pull upstream main
   git push origin main
   ```

2. **Create feature branch:**
   ```bash
   git checkout -b feature/new-feature
   ```

3. **Regular commits:**
   ```bash
   git add .
   git commit -m "feat: implement initial version of feature"
   ```

4. **Keep branch updated:**
   ```bash
   git rebase main  # or merge main if preferred
   ```

### Release Process

For maintainers:

1. Update version numbers
2. Update CHANGELOG.md
3. Create release tag
4. Build and test release artifacts
5. Create GitHub release

## Getting Help

- **Questions**: Use GitHub Discussions
- **Bugs**: File an issue with bug report template
- **Features**: File an issue with feature request template
- **Chat**: Join our community discussions

## Recognition

Contributors are recognized in:
- GitHub contributors list
- CHANGELOG.md for significant contributions
- Release notes for major features

Thank you for contributing to the C++ SDRTrunk Transcriber project!