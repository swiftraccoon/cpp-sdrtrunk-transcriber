cmake_minimum_required(VERSION 3.10)
project(sdrTrunkTranscriber)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(CURL REQUIRED)

# Add yaml-cpp subdirectory 
add_subdirectory(yaml-cpp)

# For the main executable
add_executable(sdrTrunkTranscriber main.cpp curlHelper.cpp)
target_link_libraries(sdrTrunkTranscriber CURL::libcurl yaml-cpp)

# For tests
enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(runTests test.cpp curlHelper.cpp)  
target_link_libraries(runTests ${GTEST_LIBRARIES} pthread CURL::libcurl yaml-cpp)  
