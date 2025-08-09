#!/bin/bash
# Build script for SDRTrunk Transcriber

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Parse arguments
BUILD_TYPE="Release"
BUILD_TESTS="OFF"
CLEAN_BUILD="false"
INSTALL_DEPS="false"
PRESET=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --tests)
            BUILD_TESTS="ON"
            shift
            ;;
        --clean)
            CLEAN_BUILD="true"
            shift
            ;;
        --install-deps)
            INSTALL_DEPS="true"
            shift
            ;;
        --preset)
            PRESET="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --debug       Build in Debug mode"
            echo "  --tests       Build and run tests"
            echo "  --clean       Clean build directory first"
            echo "  --install-deps Install system dependencies"
            echo "  --preset NAME Use CMake preset"
            echo "  --help        Show this help"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Install system dependencies if requested
if [[ "$INSTALL_DEPS" == "true" ]]; then
    print_status "Installing system dependencies..."
    
    if command -v dnf &> /dev/null; then
        sudo dnf install -y cmake gcc-c++ libcurl-devel sqlite-devel yaml-cpp-devel cli11-devel nlohmann-json-devel gtest-devel
    elif command -v apt &> /dev/null; then
        sudo apt update
        sudo apt install -y build-essential cmake libcurl4-openssl-dev libsqlite3-dev libyaml-cpp-dev libcli11-dev nlohmann-json3-dev libgtest-dev
    elif command -v pacman &> /dev/null; then
        sudo pacman -S --needed cmake gcc curl sqlite yaml-cpp cli11 nlohmann-json gtest
    else
        print_warning "Package manager not detected. Please install dependencies manually."
        print_warning "Required: cmake, gcc-c++, libcurl-dev, sqlite-dev, yaml-cpp-dev, cli11-dev, nlohmann-json-dev, gtest-dev"
    fi
fi

# Clean build directory if requested
if [[ "$CLEAN_BUILD" == "true" ]]; then
    print_status "Cleaning build directory..."
    rm -rf build build-*
fi

# Configure and build
if [[ -n "$PRESET" ]]; then
    print_status "Using CMake preset: $PRESET"
    cmake --preset="$PRESET"
    cmake --build build --config "$BUILD_TYPE"
else
    print_status "Configuring CMake build..."
    cmake -B build \
          -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
          -DBUILD_TESTS="$BUILD_TESTS" \
          -DUSE_SYSTEM_DEPS=ON \
          -DENABLE_INSTALL=ON \
          -DENABLE_PACKAGING=ON
    
    print_status "Building project..."
    cmake --build build --config "$BUILD_TYPE"
fi

# Run tests if enabled
if [[ "$BUILD_TESTS" == "ON" ]]; then
    print_status "Running tests..."
    cd build
    ctest --output-on-failure
    cd ..
fi

# Show build results
print_status "Build completed successfully!"
echo ""
echo "Executable location: build/sdrtrunk-transcriber"
echo "Build type: $BUILD_TYPE"
echo "Tests enabled: $BUILD_TESTS"

# Show file info
if [[ -f "build/sdrtrunk-transcriber" ]]; then
    ls -lh build/sdrtrunk-transcriber*
fi

print_status "Run './build/sdrtrunk-transcriber --help' to see usage options"