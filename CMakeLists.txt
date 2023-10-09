cmake_minimum_required(VERSION 3.10)
project(sdrTrunkTranscriber)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(CURL REQUIRED)

# For the main executable
add_executable(sdrTrunkTranscriber main.cpp curlHelper.cpp)
target_link_libraries(sdrTrunkTranscriber CURL::libcurl)

# For tests
enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(runTests test.cpp curlHelper.cpp)  # Include curlHelper.cpp here
target_link_libraries(runTests ${GTEST_LIBRARIES} pthread CURL::libcurl)  # Include CURL::libcurl here
