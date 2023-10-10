cmake_minimum_required(VERSION 3.10)
project(sdrTrunkTranscriber)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)

add_subdirectory(yaml-cpp)

# For the main executable
add_executable(sdrTrunkTranscriber main.cpp curlHelper.cpp DatabaseManager.cpp fileProcessor.cpp)
target_include_directories(sdrTrunkTranscriber PUBLIC
    ${CURL_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
)

target_link_libraries(sdrTrunkTranscriber
    CURL::libcurl
    yaml-cpp
    SQLite::SQLite3
)

# For tests
enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(runTests test.cpp curlHelper.cpp DatabaseManager.cpp fileProcessor.cpp)
target_link_libraries(runTests ${GTEST_LIBRARIES} pthread CURL::libcurl yaml-cpp SQLite::SQLite3)