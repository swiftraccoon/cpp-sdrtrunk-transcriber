# Build Documentation

Comprehensive build instructions, troubleshooting, and platform-specific guidance for the C++ SDRTrunk Transcriber project.

## Table of Contents

- [Quick Start](#quick-start)
- [System Requirements](#system-requirements)
- [Platform-Specific Instructions](#platform-specific-instructions)
  - [Ubuntu/Debian](#ubuntudebian)
  - [Fedora/RHEL/CentOS](#fedorarhel-centos)
  - [macOS](#macos)
  - [Windows](#windows)
- [Build Configuration Options](#build-configuration-options)
- [Development Builds](#development-builds)
- [Cross-Compilation](#cross-compilation)
- [Troubleshooting](#troubleshooting)
- [Performance Optimization](#performance-optimization)
- [CI/CD Integration](#cicd-integration)

## Quick Start

For experienced developers who want to get building immediately:

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake git pkg-config \
    ffmpeg libavcodec-dev libcurl4-openssl-dev \
    libsqlite3-dev libyaml-cpp-dev

# Clone and build
git clone https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber.git
cd cpp-sdrtrunk-transcriber
git clone https://github.com/jbeder/yaml-cpp.git external/yaml-cpp
git clone https://github.com/CLIUtils/CLI11.git external/CLI11
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)

# Test
./build/sdrtrunk-transcriber --help
```

## System Requirements

### Minimum Requirements

- **CPU**: x86_64 or ARM64 processor
- **RAM**: 2GB available memory (4GB+ recommended for local transcription)
- **Storage**: 100MB for application, additional space for recordings and database
- **Network**: Internet connection (for OpenAI API mode)

### Software Requirements

- **C++ Compiler**: GCC 7+, Clang 7+, or MSVC 2019+
- **CMake**: Version 3.16 or higher
- **Git**: For cloning repositories and submodules
- **FFmpeg**: For audio file processing and validation

### Supported Platforms

| Platform | Architecture | Status | Notes |
|----------|-------------|--------|-------|
| Ubuntu 20.04+ | x86_64 | ✅ Fully Supported | Primary development platform |
| Ubuntu 22.04+ | x86_64 | ✅ Fully Supported | Recommended for production |
| Debian 11+ | x86_64 | ✅ Fully Supported | Stable platform |
| Fedora 35+ | x86_64 | ✅ Fully Supported | Well tested |
| RHEL/CentOS 8+ | x86_64 | ✅ Supported | Enterprise deployments |
| macOS 11+ | x86_64, ARM64 | ✅ Supported | Homebrew dependencies |
| Windows 10+ | x86_64 | ⚠️ Experimental | Limited testing |
| Alpine Linux | x86_64 | ⚠️ Experimental | Container deployments |
| Raspberry Pi OS | ARM64 | ⚠️ Experimental | Single-board computers |

## Platform-Specific Instructions

### Ubuntu/Debian

#### Prerequisites Installation

```bash
# Update package list
sudo apt-get update

# Install build tools
sudo apt-get install -y build-essential cmake git pkg-config

# Install core dependencies
sudo apt-get install -y \
    ffmpeg libavcodec-dev libavformat-dev libavutil-dev \
    libcurl4-openssl-dev libsqlite3-dev libyaml-cpp-dev

# Install optional development tools
sudo apt-get install -y \
    gdb valgrind clang-format clang-tidy \
    doxygen graphviz
```

#### Version-Specific Notes

**Ubuntu 20.04 LTS:**
- Default CMake version is 3.16 (minimum required)
- yaml-cpp version may be older; consider building from source

**Ubuntu 22.04 LTS:**
- Recommended platform with latest stable packages
- All dependencies available in default repositories

**Debian 11 (Bullseye):**
- May need to enable backports for newer CMake:
  ```bash
  echo "deb http://deb.debian.org/debian bullseye-backports main" | \
      sudo tee -a /etc/apt/sources.list
  sudo apt-get update
  sudo apt-get -t bullseye-backports install cmake
  ```

### Fedora/RHEL/CentOS

#### Fedora 35+

```bash
# Install build tools and dependencies
sudo dnf install -y gcc-c++ cmake git pkg-config \
    ffmpeg-devel libcurl-devel sqlite-devel yaml-cpp-devel

# Install development tools (optional)
sudo dnf install -y gdb valgrind clang-tools-extra \
    doxygen graphviz
```

#### RHEL/CentOS 8+

```bash
# Enable PowerTools/CodeReady Builder repository
sudo dnf config-manager --set-enabled powertools  # CentOS
# OR for RHEL:
# sudo subscription-manager repos --enable codeready-builder-for-rhel-8-x86_64-rpms

# Enable EPEL repository
sudo dnf install -y epel-release

# Install dependencies
sudo dnf install -y gcc-c++ cmake git pkg-config \
    ffmpeg-devel libcurl-devel sqlite-devel

# yaml-cpp might need to be built from source on older versions
```

#### Rocky Linux/AlmaLinux

```bash
# Enable repositories
sudo dnf config-manager --set-enabled powertools
sudo dnf install -y epel-release

# Install dependencies (same as RHEL/CentOS)
sudo dnf install -y gcc-c++ cmake git pkg-config \
    ffmpeg-devel libcurl-devel sqlite-devel
```

### macOS

#### Using Homebrew (Recommended)

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake git pkg-config ffmpeg sqlite3 curl yaml-cpp

# Install development tools (optional)
brew install gdb llvm doxygen graphviz
```

#### Using MacPorts

```bash
# Install dependencies
sudo port install cmake git pkgconfig ffmpeg sqlite3 curl yaml-cpp
```

#### Xcode Requirements

- Xcode Command Line Tools: `xcode-select --install`
- Xcode 12+ for full C++17 support
- Set appropriate deployment target:
  ```bash
  export MACOSX_DEPLOYMENT_TARGET=11.0
  ```

### Windows

#### Using vcpkg (Recommended)

```powershell
# Clone and bootstrap vcpkg
git clone https://github.com/Microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg\vcpkg install \
    curl:x64-windows sqlite3:x64-windows \
    yaml-cpp:x64-windows

# Set environment variables
$env:CMAKE_TOOLCHAIN_FILE = "$(pwd)\vcpkg\scripts\buildsystems\vcpkg.cmake"
```

#### FFmpeg Installation

1. Download FFmpeg from https://ffmpeg.org/download.html
2. Extract to `C:\ffmpeg`
3. Add `C:\ffmpeg\bin` to system PATH
4. Verify: `ffmpeg -version`

#### Visual Studio Requirements

- Visual Studio 2019 or 2022 with C++ workload
- Windows SDK 10.0.18362.0 or later
- CMake tools for Visual Studio

#### Build Commands

```powershell
# Configure with vcpkg
cmake -B build -DCMAKE_TOOLCHAIN_FILE="path\to\vcpkg\scripts\buildsystems\vcpkg.cmake" \
    -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release
```

#### Using MSYS2 (Alternative)

```bash
# In MSYS2 MinGW64 terminal
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-curl mingw-w64-x86_64-sqlite3 \
    mingw-w64-x86_64-yaml-cpp mingw-w64-x86_64-ffmpeg
```

## Build Configuration Options

### Standard CMake Options

```bash
# Build types
cmake -B build -DCMAKE_BUILD_TYPE=Debug      # Development build
cmake -B build -DCMAKE_BUILD_TYPE=Release    # Optimized build
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo  # Optimized with debug info
cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel # Size-optimized build

# Install prefix
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local

# Compiler selection
cmake -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

### Project-Specific Options

```bash
# Enable/disable tests
cmake -B build -DBUILD_TESTS=ON          # Enable unit tests
cmake -B build -DBUILD_TESTS=OFF         # Disable unit tests (default)

# Shared vs static libraries
cmake -B build -DBUILD_SHARED_LIBS=ON    # Use shared libraries
cmake -B build -DBUILD_SHARED_LIBS=OFF   # Use static libraries (default)

# Installation and packaging
cmake -B build -DENABLE_INSTALL=ON       # Enable install targets (default)
cmake -B build -DENABLE_PACKAGING=ON     # Enable CPack packaging (default)

# Dependency management
cmake -B build -DUSE_SYSTEM_DEPS=ON      # Prefer system dependencies (default)
cmake -B build -DUSE_SYSTEM_DEPS=OFF     # Force bundled dependencies
```

### Compiler-Specific Flags

```bash
# GCC optimizations
cmake -B build -DCMAKE_CXX_FLAGS="-march=native -mtune=native"

# Debug builds with sanitizers
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined"

# Clang with static analysis
cmake -B build -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_CXX_FLAGS="--analyze -Xanalyzer -analyzer-output=text"
```

## Development Builds

### Debug Configuration

```bash
# Full debug build with tests and tools
cmake -B build-debug \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON \
    -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror -g3 -O0"

cmake --build build-debug --parallel

# Run tests
cd build-debug && ctest --output-on-failure --parallel
```

### Memory Debugging

```bash
# Build with AddressSanitizer
cmake -B build-asan -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
cmake --build build-asan

# Run with memory checking
ASAN_OPTIONS=detect_leaks=1:check_initialization_order=1 \
    ./build-asan/sdrtrunk-transcriber --help

# Alternative: Valgrind
valgrind --leak-check=full --show-leak-kinds=all \
    ./build/sdrtrunk-transcriber --help
```

### Static Analysis

```bash
# Clang-tidy analysis
clang-tidy src/*.cpp -- -Iinclude -std=c++17

# Cppcheck analysis
cppcheck --enable=all --std=c++17 src/ include/

# PVS-Studio (commercial)
pvs-studio-analyzer trace -- cmake --build build
pvs-studio-analyzer analyze
plog-converter -a GA:1,2 -t tasklist -o project.tasks PVS-Studio.log
```

### Code Formatting

```bash
# Format all source files
find src include -name "*.cpp" -o -name "*.h" | \
    xargs clang-format -i --style=file

# Check formatting without changes
find src include -name "*.cpp" -o -name "*.h" | \
    xargs clang-format --dry-run --Werror --style=file
```

## Cross-Compilation

### ARM64 (AArch64)

```bash
# Install cross-compiler
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Configure for ARM64
cmake -B build-arm64 \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
    -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
    -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
    -DCMAKE_FIND_ROOT_PATH=/usr/aarch64-linux-gnu

cmake --build build-arm64
```

### Raspberry Pi

```bash
# Using Docker for cross-compilation
docker run --rm -v $(pwd):/workspace \
    -w /workspace balenalib/raspberry-pi-ubuntu:latest \
    bash -c "apt-get update && apt-get install -y build-essential cmake && \
             cmake -B build-rpi && cmake --build build-rpi"
```

### Windows from Linux

```bash
# Install MinGW-w64
sudo apt-get install mingw-w64

# Configure for Windows
cmake -B build-win64 \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++

cmake --build build-win64
```

## Troubleshooting

### Common Build Issues

#### CMake Configuration Errors

**Error**: `CMake 3.16 or higher is required`
```bash
# Solution: Update CMake
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | \
    sudo apt-key add -
echo 'deb https://apt.kitware.com/ubuntu/ focal main' | \
    sudo tee /etc/apt/sources.list.d/kitware.list
sudo apt-get update && sudo apt-get install cmake
```

**Error**: `Could not find yaml-cpp`
```bash
# Solution 1: Install system package
sudo apt-get install libyaml-cpp-dev

# Solution 2: Build from source
git clone https://github.com/jbeder/yaml-cpp.git external/yaml-cpp
# CMake will automatically use bundled version
```

**Error**: `Could not find PkgConfig`
```bash
# Solution: Install pkg-config
sudo apt-get install pkg-config
```

#### Compilation Errors

**Error**: `error: 'filesystem' is not a member of 'std'`
```bash
# Solution: Ensure C++17 and link filesystem library
cmake -B build -DCMAKE_CXX_STANDARD=17
# The CMakeLists.txt should handle this automatically
```

**Error**: `undefined reference to 'curl_easy_init'`
```bash
# Solution: Install libcurl development headers
sudo apt-get install libcurl4-openssl-dev
```

**Error**: `fatal error: 'sqlite3.h' file not found`
```bash
# Solution: Install SQLite development headers
sudo apt-get install libsqlite3-dev
```

#### Linking Errors

**Error**: `undefined reference to 'avformat_open_input'`
```bash
# Solution: Install FFmpeg development libraries
sudo apt-get install libavformat-dev libavcodec-dev libavutil-dev
```

**Error**: `cannot find -lyaml-cpp`
```bash
# Solution: Check if yaml-cpp is properly installed
pkg-config --libs yaml-cpp

# If not found, install or build from source
sudo apt-get install libyaml-cpp-dev
```

### Runtime Issues

#### Configuration File Errors

**Error**: `Config file not found: ./config.yaml`
```bash
# Solution: Copy sample configuration
cp sample-config.yaml config.yaml
vim config.yaml  # Edit as needed
```

**Error**: `YAML parsing error`
```bash
# Solution: Validate YAML syntax
yamllint config.yaml

# Or use Python to check
python3 -c "import yaml; yaml.safe_load(open('config.yaml'))"
```

#### Permission Issues

**Error**: `Permission denied accessing directory`
```bash
# Solution: Fix directory permissions
sudo chown -R $USER:$USER /path/to/recordings
chmod 755 /path/to/recordings
```

**Error**: `Cannot create database file`
```bash
# Solution: Ensure directory exists and is writable
mkdir -p $(dirname /path/to/database.db)
touch /path/to/database.db
```

#### FFmpeg Issues

**Error**: `FFmpeg not found in PATH`
```bash
# Solution: Install FFmpeg
sudo apt-get install ffmpeg

# Or add to PATH if installed elsewhere
export PATH=/path/to/ffmpeg/bin:$PATH
```

**Error**: `Unsupported audio format`
```bash
# Solution: Check FFmpeg codec support
ffmpeg -codecs | grep mp3

# Install additional codecs if needed
sudo apt-get install ubuntu-restricted-extras
```

### Performance Issues

#### High CPU Usage

1. **Check polling frequency**:
   ```yaml
   # In config.yaml, increase wait time
   LoopWaitSeconds: 5000  # 5 seconds instead of 200ms
   ```

2. **Disable debug output**:
   ```yaml
   DEBUG_MAIN: false
   DEBUG_FILE_PROCESSOR: false
   # ... disable all debug flags
   ```

3. **Use Release build**:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

#### High Memory Usage

1. **Monitor with system tools**:
   ```bash
   # Check memory usage
   top -p $(pgrep sdrtrunk-transcriber)
   
   # Check for memory leaks
   valgrind --tool=massif ./build/sdrtrunk-transcriber
   ```

2. **Optimize database operations**:
   - Ensure database file is on fast storage (SSD)
   - Consider periodic database vacuum operations
   - Monitor database size growth

#### Slow Transcription

1. **For OpenAI API mode**:
   - Check network connectivity and latency
   - Monitor API rate limits
   - Consider regional API endpoints

2. **For local mode**:
   - Ensure faster-whisper uses GPU if available
   - Check Python environment and dependencies
   - Monitor system resources during transcription

### Getting Additional Help

#### Debug Information Collection

```bash
# System information
uname -a
lsb_release -a

# Compiler information
gcc --version
cmake --version

# Dependency versions
pkg-config --modversion libcurl
pkg-config --modversion sqlite3
ffmpeg -version

# Build information
cmake -B build -LAH  # List all CMake variables
```

#### Enable Debug Logging

```yaml
# In config.yaml
DEBUG_MAIN: true
DEBUG_FILE_PROCESSOR: true
DEBUG_DATABASE_MANAGER: true
DEBUG_CURL_HELPER: true
DEBUG_TRANSCRIPTION_PROCESSOR: true
```

#### Community Support

- **GitHub Issues**: Report bugs and build issues
- **GitHub Discussions**: Ask questions and get help
- **Project Wiki**: Additional documentation and examples

## Performance Optimization

### Compiler Optimizations

```bash
# Native CPU optimizations
cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-march=native -mtune=native -flto"

# Profile-guided optimization (advanced)
# 1. Build with instrumentation
cmake -B build-pgo -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-fprofile-generate"
# 2. Run typical workload to generate profile data
# 3. Rebuild with profile data
cmake -B build-pgo -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-fprofile-use"
```

### Link-Time Optimization

```bash
# Enable LTO for smaller, faster binaries
cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

### Static Linking

```bash
# Create fully static binary (useful for containers)
cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++ -static"
```

## CI/CD Integration

### GitHub Actions

The project includes CI/CD workflows in `.github/workflows/`:

- `cmake-ubuntu.yml`: Ubuntu builds and tests
- `cmake-windows.yml`: Windows builds
- `ubuntu-codeql.yml`: Security analysis
- `snyk-security.yml`: Dependency vulnerability scanning

### Custom CI Integration

```bash
#!/bin/bash
# Example CI script
set -e

# Install dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake git pkg-config \
    ffmpeg libavcodec-dev libcurl4-openssl-dev \
    libsqlite3-dev libyaml-cpp-dev

# Clone external dependencies
git clone https://github.com/jbeder/yaml-cpp.git external/yaml-cpp
git clone https://github.com/CLIUtils/CLI11.git external/CLI11

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build --parallel $(nproc)

# Run tests
cd build && ctest --output-on-failure --parallel

# Basic smoke test
./sdrtrunk-transcriber --help

# Package for distribution
cpack -B packages
```

### Docker Builds

```dockerfile
# Multi-stage Dockerfile for optimized builds
FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential cmake git pkg-config \
    ffmpeg libavcodec-dev libcurl4-openssl-dev \
    libsqlite3-dev libyaml-cpp-dev

WORKDIR /src
COPY . .
RUN git clone https://github.com/jbeder/yaml-cpp.git external/yaml-cpp && \
    git clone https://github.com/CLIUtils/CLI11.git external/CLI11

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y ffmpeg libcurl4 libsqlite3-0 libyaml-cpp0.7
COPY --from=builder /src/build/sdrtrunk-transcriber /usr/local/bin/
ENTRYPOINT ["/usr/local/bin/sdrtrunk-transcriber"]
```

For additional build support and platform-specific issues, please refer to the GitHub Issues page or community discussions.