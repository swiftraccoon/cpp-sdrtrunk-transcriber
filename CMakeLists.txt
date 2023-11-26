# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Project name and language
project(sdrTrunkTranscriber)

# Set C++ standard and make it mandatory
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find required packages
# CURL for making HTTP requests
find_package(CURL REQUIRED)
# SQLite3 for database operations
find_package(SQLite3 REQUIRED)
# PkgConfig for managing package information
find_package(PkgConfig REQUIRED)


# Add yaml-cpp subdirectory to build YAML parser library
add_subdirectory(external/yaml-cpp)

# Add CLI11 as a subdirectory
add_subdirectory(external/CLI11)

# Main Executable Configuration
# Define the executable and its source files
add_executable(sdrTrunkTranscriber src/main.cpp src/curlHelper.cpp src/DatabaseManager.cpp src/fileProcessor.cpp src/ConfigSingleton.cpp src/transcriptionProcessor.cpp src/debugUtils.cpp src/fasterWhisper.cpp)

# Specify include directories for the main executable
target_include_directories(sdrTrunkTranscriber PUBLIC
    ${CURL_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/yaml-cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/CLI11
)

# Link required libraries to the main executable
target_link_libraries(sdrTrunkTranscriber
    CURL::libcurl  # CURL for HTTP requests
    yaml-cpp       # YAML parser
    SQLite::SQLite3 # SQLite3 for database operations
    CLI11::CLI11
)

# Test Configuration
# Enable testing functionality
# enable_testing()

# # Find Google Test framework
# find_package(GTest REQUIRED)

# # Include Google Test headers
# include_directories(${GTEST_INCLUDE_DIRS})

# #
# include_directories(${PROJECT_SOURCE_DIR}/external/json/single_include)

# # Define the test executable and its source files
# add_executable(runTests test.cpp curlHelper.cpp DatabaseManager.cpp fileProcessor.cpp ConfigSingleton.cpp transcriptionProcessor.cpp debugUtils.cpp)

# Link required libraries to the test executable
# target_link_libraries(runTests 
#     ${GTEST_LIBRARIES}  # Google Test framework
#     pthread             # POSIX threads
#     CURL::libcurl       # CURL for HTTP requests
#     yaml-cpp            # YAML parser
#     SQLite::SQLite3     # SQLite3 for database operations
#     CLI11::CLI11
# )