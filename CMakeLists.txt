# Modern CMake build system for SDRTrunk Transcriber
cmake_minimum_required(VERSION 3.16)

# Project declaration with version and description
project(sdrTrunkTranscriber
    VERSION 1.0.0
    DESCRIPTION "C++ application for transcribing SDRTrunk P25 recordings"
    LANGUAGES CXX
)

# Set C++ standard to C++17 and make it required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Build options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_INSTALL "Enable install targets" ON)
option(ENABLE_PACKAGING "Enable CPack packaging" ON)
option(USE_SYSTEM_DEPS "Prefer system dependencies over bundled ones" ON)

# Include compiler flags and warnings setup
include(CompilerFlags)

# Set default build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Find or fetch dependencies
include(Dependencies)

# Source files
set(SOURCES
    src/main.cpp
    src/curlHelper.cpp
    src/DatabaseManager.cpp
    src/fileProcessor.cpp
    src/ConfigSingleton.cpp
    src/transcriptionProcessor.cpp
    src/debugUtils.cpp
    src/fasterWhisper.cpp
    src/security.cpp
)

# Create main executable
add_executable(sdrTrunkTranscriber ${SOURCES})

# Set target properties
set_target_properties(sdrTrunkTranscriber PROPERTIES
    OUTPUT_NAME "sdrtrunk-transcriber"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Include directories
target_include_directories(sdrTrunkTranscriber PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(sdrTrunkTranscriber PRIVATE
    CURL::libcurl
    SQLite::SQLite3
    yaml-cpp
    CLI11::CLI11
    nlohmann_json::nlohmann_json
)

# Compiler-specific options
target_compile_features(sdrTrunkTranscriber PRIVATE cxx_std_17)

# Add compile definitions for build type
target_compile_definitions(sdrTrunkTranscriber PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Testing configuration
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Installation configuration
if(ENABLE_INSTALL)
    include(GNUInstallDirs)
    
    install(TARGETS sdrTrunkTranscriber
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
    )
    
    # Install sample configuration
    install(FILES sample-config.yaml
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/sdrtrunk-transcriber
        COMPONENT Runtime
    )
    
    # Install scripts
    install(DIRECTORY scripts/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/sdrtrunk-transcriber/scripts
        COMPONENT Runtime
        FILES_MATCHING PATTERN "*.py" PATTERN "*.sh"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                   GROUP_READ GROUP_EXECUTE
                   WORLD_READ WORLD_EXECUTE
    )
endif()

# Packaging configuration
if(ENABLE_PACKAGING)
    include(Packaging)
endif()