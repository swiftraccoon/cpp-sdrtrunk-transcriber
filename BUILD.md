# Build Instructions for SDRTrunk Transcriber

This document provides comprehensive instructions for building the SDRTrunk Transcriber application using the modernized CMake build system.

## Prerequisites

### Required Dependencies

- **CMake** 3.25 or newer
- **C++ Compiler** with C++23 support:
  - GCC 13+ (Linux)
  - Clang 19+ (Linux/macOS)
  - Visual Studio 2022+ (Windows)

### System Dependencies

The following libraries are required and can be installed via your system package manager or vcpkg:

- **libcurl** - HTTP client library
- **sqlite3** - SQLite database library
- **yaml-cpp** - YAML parsing library (optional, will be fetched if not found)
- **CLI11** - Command line parsing library (optional, will be fetched if not found)
- **nlohmann/json** - JSON library (optional, will be fetched if not found)
- **Google Test** - Unit testing framework (optional, for running tests)

## Installation Instructions

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install build-essential cmake libcurl4-openssl-dev libsqlite3-dev libyaml-cpp-dev
```

### RHEL/CentOS/Fedora

```bash
# Fedora
sudo dnf install gcc-c++ cmake libcurl-devel sqlite-devel yaml-cpp-devel

# RHEL/CentOS (with EPEL)
sudo yum install gcc-c++ cmake3 libcurl-devel sqlite-devel
```

### macOS

```bash
# Using Homebrew
brew install cmake curl sqlite yaml-cpp

# Using MacPorts
sudo port install cmake curl sqlite3 yaml-cpp
```

### Windows

#### Using vcpkg (Recommended)

1. Install vcpkg:
```cmd
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

2. Set environment variable:
```cmd
set VCPKG_ROOT=C:\path\to\vcpkg
```

3. Dependencies will be automatically installed when building with the vcpkg preset.

## Build Options

The build system provides several configuration options:

- `BUILD_TESTS` - Enable/disable building tests (default: ON)
- `USE_SYSTEM_DEPS` - Prefer system dependencies over bundled ones (default: ON)
- `ENABLE_INSTALL` - Enable install targets (default: ON)
- `ENABLE_PACKAGING` - Enable CPack packaging (default: ON)
- `ENABLE_COVERAGE` - Enable code coverage reporting (default: OFF)
- `BUILD_SHARED_LIBS` - Build shared libraries instead of static (default: OFF)

## Build Methods

### Method 1: Using CMake Presets (Recommended)

The project includes CMake presets for different build configurations:

```bash
# List available presets
cmake --list-presets

# Configure with default preset
cmake --preset=default

# Build
cmake --build build

# Run tests
cmake --build build --target test

# Install (optional)
cmake --build build --target install
```

Available presets:
- `default` - Standard release build with system dependencies
- `debug` - Debug build with coverage support
- `release` - Optimized release build
- `vcpkg` - Build using vcpkg dependencies (Windows recommended)
- `bundled` - Build using FetchContent for all dependencies
- `sanitizer` - Debug build with AddressSanitizer and UBSan (Linux/macOS)

### Method 2: Manual CMake Configuration

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

# Build
cmake --build .

# Run tests
ctest --output-on-failure

# Install (optional)
cmake --build . --target install
```

### Method 3: Platform-Specific Builds

#### Linux Debug Build with Sanitizers

```bash
cmake --preset=sanitizer
cmake --build build-sanitizer
cd build-sanitizer && ctest --output-on-failure
```

#### Windows Build with Visual Studio

```bash
cmake --preset=windows-vs2022
cmake --build build-windows --config Release
```

#### macOS Universal Binary

```bash
cmake -B build-macos -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build build-macos
```

## Testing

### Running Tests

```bash
# Run all tests
cmake --build build --target test

# Run tests with verbose output
ctest --verbose --output-on-failure

# Run specific test
ctest -R "TranscriptionProcessor"

# Run tests with memory checking (Linux/macOS)
cmake --build build --target test-memcheck
```

### Code Coverage

Enable coverage and generate report:

```bash
cmake --preset=debug -DENABLE_COVERAGE=ON
cmake --build build-debug
cd build-debug && ctest
cmake --build . --target coverage
# Open build-debug/coverage/html/index.html in browser
```

## Packaging

### Creating Packages

```bash
# Configure for packaging
cmake --preset=release
cmake --build build-release

# Create packages
cd build-release
cpack

# Or create specific package types
cpack -G DEB      # Debian package
cpack -G RPM      # RPM package  
cpack -G NSIS     # Windows installer
cpack -G TGZ      # Tar archive
```

### Installation Locations

Default installation paths:
- **Linux**: `/usr/local/bin/sdrtrunk-transcriber`
- **Windows**: `C:\Program Files\sdrtrunk-transcriber\`
- **macOS**: `/usr/local/bin/sdrtrunk-transcriber`

Configuration files:
- **Linux**: `/etc/sdrtrunk-transcriber/sample-config.yaml`
- **Windows**: `%ProgramFiles%\sdrtrunk-transcriber\config\`
- **macOS**: `/usr/local/etc/sdrtrunk-transcriber/sample-config.yaml`

## Troubleshooting

### Common Issues

1. **CMake too old**: Upgrade to CMake 3.16+
2. **Missing dependencies**: Install required libraries or use bundled preset
3. **Compiler not found**: Install appropriate C++17 compiler
4. **vcpkg not found**: Set `VCPKG_ROOT` environment variable

### Dependency Issues

If system dependencies are not found:

```bash
# Use bundled dependencies
cmake --preset=bundled

# Or disable system dependency preference
cmake -B build -DUSE_SYSTEM_DEPS=OFF
```

### Windows-Specific Issues

1. **Visual Studio not detected**: Run from Developer Command Prompt
2. **vcpkg integration**: Ensure `VCPKG_ROOT` is set correctly
3. **Path length limits**: Use short build directory paths

### Memory Issues During Build

For systems with limited memory:

```bash
# Reduce parallel build jobs
cmake --build build -j2

# Or build sequentially
cmake --build build -j1
```

## Development Workflow

### Quick Development Build

```bash
# One-time setup
cmake --preset=debug

# Iterative development
cmake --build build-debug && cd build-debug && ctest
```

### Adding Dependencies

1. Add to `vcpkg.json` for vcpkg support
2. Update `cmake/modules/Dependencies.cmake` for system/fetch support
3. Link in main `CMakeLists.txt`

### IDE Integration

#### VS Code

Install CMake Tools extension. The project includes `.vscode/settings.json` with appropriate CMake preset configurations.

#### Visual Studio

Open folder in Visual Studio 2019+ with CMake support. Select appropriate preset from CMake configuration dropdown.

#### CLion

Import CMake project. CLion will automatically detect presets and configurations.

## Performance Optimization

### Release Build Optimizations

The release build includes:
- Link-time optimization (LTO/IPO) when supported
- Compiler-specific optimizations (-O3 for GCC/Clang)
- Security hardening flags on Unix systems

### Profile-Guided Optimization

For maximum performance on target hardware:

```bash
# Build instrumented binary
cmake -B build-pgo -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fprofile-generate"
cmake --build build-pgo

# Run training workload
./build-pgo/sdrtrunk-transcriber <typical-usage>

# Rebuild with profile data
cmake -B build-optimized -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fprofile-use"
cmake --build build-optimized
```

## Continuous Integration

The project includes GitHub Actions workflows for:
- Ubuntu builds with multiple compilers
- Windows builds with Visual Studio
- macOS builds with Xcode
- Code quality checks and security scanning

See `.github/workflows/` for implementation details.