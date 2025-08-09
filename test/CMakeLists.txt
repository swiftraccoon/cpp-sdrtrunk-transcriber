# Test configuration for SDRTrunk Transcriber

# Find or fetch Google Test with GMock
if(USE_SYSTEM_DEPS)
    find_package(GTest QUIET)
    find_package(GMock QUIET)
endif()

if(NOT GTest_FOUND AND NOT TARGET gtest)
    message(STATUS "Google Test not found, using FetchContent")
    include(FetchContent)
    
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    # Disable pthreads on Windows
    if(WIN32)
        set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
        add_compile_definitions(GTEST_HAS_PTHREAD=0)
    endif()
    
    # Enable GMock for mocking support
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(googletest)
    
    # Create aliases for consistent naming
    add_library(GTest::GTest ALIAS gtest)
    add_library(GTest::Main ALIAS gtest_main)
    add_library(GTest::GMock ALIAS gmock)
    add_library(GTest::GMockMain ALIAS gmock_main)
else()
    message(STATUS "Using system Google Test")
endif()

# Check for Google Benchmark (optional for performance tests)
find_package(benchmark QUIET)
if(benchmark_FOUND)
    message(STATUS "Google Benchmark found - performance tests enabled")
    set(BENCHMARK_AVAILABLE TRUE)
else()
    message(STATUS "Google Benchmark not found - performance tests will be basic timing only")
    set(BENCHMARK_AVAILABLE FALSE)
endif()

# Source files for testing (exclude main.cpp)
set(TEST_SOURCES
    ../src/curlHelper.cpp
    ../src/DatabaseManager.cpp
    ../src/fileProcessor.cpp
    ../src/ConfigSingleton.cpp
    ../src/transcriptionProcessor.cpp
    ../src/debugUtils.cpp
    ../src/fasterWhisper.cpp
)

# Test source files
set(TEST_FILES
    test.cpp
    SimplePerformanceTests.cpp
    IntegrationTests.cpp
    EdgeCaseTests.cpp
)

# Create main test executable
add_executable(runTests
    ${TEST_FILES}
    ${TEST_SOURCES}
)

# Create separate performance test executable (if benchmark available)
if(BENCHMARK_AVAILABLE)
    add_executable(perfTests
        PerformanceTests.cpp
        ${TEST_SOURCES}
    )
endif()

# Set target properties
set_target_properties(runTests PROPERTIES
    OUTPUT_NAME "sdrtrunk-transcriber-tests"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Include directories
target_include_directories(runTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}  # For test headers
)

# Link libraries for main test executable
target_link_libraries(runTests PRIVATE
    GTest::GTest
    GTest::Main
    CURL::libcurl
    SQLite::SQLite3
    yaml-cpp
    CLI11::CLI11
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# Link libraries for performance tests (if available)
if(BENCHMARK_AVAILABLE)
    target_link_libraries(perfTests PRIVATE
        GTest::GTest
        GTest::Main
        GTest::GMock
        benchmark::benchmark
        CURL::libcurl
        SQLite::SQLite3
        yaml-cpp
        CLI11::CLI11
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    
    # Set properties for performance test executable
    set_target_properties(perfTests PROPERTIES
        OUTPUT_NAME "sdrtrunk-transcriber-perf-tests"
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
    
    target_include_directories(perfTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# Compiler-specific options for tests
target_compile_features(runTests PRIVATE cxx_std_17)

# Add compile definitions
target_compile_definitions(runTests PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    $<$<NOT:$<PLATFORM_ID:Windows>>:GTEST_HAS_PTHREAD=1>
    $<$<BOOL:${BENCHMARK_AVAILABLE}>:BENCHMARK_AVAILABLE>
)

if(BENCHMARK_AVAILABLE)
    target_compile_definitions(perfTests PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        $<$<NOT:$<PLATFORM_ID:Windows>>:GTEST_HAS_PTHREAD=1>
        BENCHMARK_AVAILABLE
    )
endif()

# Enable compiler warnings for tests, but be less strict
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(runTests PRIVATE
        -Wall -Wextra
        # Disable warnings that are noisy with external dependencies
        -Wno-unused-parameter
        -Wno-sign-conversion
        -Wno-conversion
        -Wno-switch-default
        -Wno-switch-enum
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(runTests PRIVATE /W3)
endif()

# Discover and register tests with CTest
include(GoogleTest)
gtest_discover_tests(runTests
    PROPERTIES
        LABELS "unit;integration;edge-case"
        TIMEOUT 120
        DISCOVERY_TIMEOUT 30
)

if(BENCHMARK_AVAILABLE)
    gtest_discover_tests(perfTests
        PROPERTIES
            LABELS "performance;benchmark"
            TIMEOUT 300
    )
endif()

# Add custom test targets
add_custom_target(test-verbose
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose --output-on-failure
    DEPENDS runTests
    COMMENT "Running tests with verbose output"
)

add_custom_target(test-unit
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose --output-on-failure -L "unit"
    DEPENDS runTests
    COMMENT "Running unit tests only"
)

add_custom_target(test-integration
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose --output-on-failure -L "integration"
    DEPENDS runTests
    COMMENT "Running integration tests only"
)

add_custom_target(test-edge-cases
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose --output-on-failure -L "edge-case"
    DEPENDS runTests
    COMMENT "Running edge case tests only"
)

if(BENCHMARK_AVAILABLE)
    add_custom_target(test-performance
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose --output-on-failure -L "performance"
        DEPENDS perfTests
        COMMENT "Running performance tests"
    )
endif()

add_custom_target(test-all
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose --output-on-failure
    DEPENDS runTests $<$<BOOL:${BENCHMARK_AVAILABLE}>:perfTests>
    COMMENT "Running all tests"
)

# Add memory check target (if available)
find_program(VALGRIND_EXECUTABLE valgrind)
if(VALGRIND_EXECUTABLE AND NOT WIN32)
    add_custom_target(test-memcheck
        COMMAND ${VALGRIND_EXECUTABLE}
            --tool=memcheck
            --leak-check=full
            --show-leak-kinds=all
            --track-origins=yes
            --verbose
            --error-exitcode=1
            $<TARGET_FILE:runTests>
        DEPENDS runTests
        COMMENT "Running tests with Valgrind memory checking"
    )
endif()

# Code coverage support
option(ENABLE_COVERAGE "Enable code coverage" OFF)
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(runTests PRIVATE --coverage)
    target_link_options(runTests PRIVATE --coverage)
    
    find_program(GCOV_EXECUTABLE gcov)
    find_program(LCOV_EXECUTABLE lcov)
    find_program(GENHTML_EXECUTABLE genhtml)
    
    if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E make_directory coverage
            COMMAND ${LCOV_EXECUTABLE} --capture --directory . --output-file coverage/coverage.info
            COMMAND ${LCOV_EXECUTABLE} --remove coverage/coverage.info '/usr/*' '*/external/*' '*/test/*' --output-file coverage/coverage.info
            COMMAND ${GENHTML_EXECUTABLE} coverage/coverage.info --output-directory coverage/html
            DEPENDS runTests
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage report"
        )
    endif()
endif()

# Test data setup - copy sample files for testing
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_data)

# Copy or create sample test files
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/test_data/sample_glossary.json
     "{\n  \"10-4\": \"acknowledged\",\n  \"officer\": \"police officer\",\n  \"unit\": \"patrol unit\"\n}")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/test_data/sample_config.yaml
     "OPENAI_API_KEY: test-key\nDATABASE_PATH: :memory:\nDirectoryToMonitor: /tmp\n")

# Print test configuration
message(STATUS "=== Test Configuration ===")
if(TARGET GTest::GTest)
    message(STATUS "Google Test: Available")
else()
    message(STATUS "Google Test: Not available")
endif()
if(GMOCK_AVAILABLE)
    message(STATUS "Google Mock: Available")
else()
    message(STATUS "Google Mock: Not available")
endif()
if(BENCHMARK_AVAILABLE)
    message(STATUS "Google Benchmark: Available")
else()
    message(STATUS "Google Benchmark: Not available")
endif()
if(VALGRIND_EXECUTABLE)
    message(STATUS "Valgrind: Found at ${VALGRIND_EXECUTABLE}")
else()
    message(STATUS "Valgrind: Not found")
endif()
if(ENABLE_COVERAGE)
    message(STATUS "Code coverage: Enabled")
else()
    message(STATUS "Code coverage: Disabled")
endif()
message(STATUS "Test files: ${CMAKE_CURRENT_BINARY_DIR}/test_data")
message(STATUS "============================")