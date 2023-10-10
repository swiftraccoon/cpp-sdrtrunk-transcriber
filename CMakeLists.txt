cmake_minimum_required(VERSION 3.10)
project(sdrTrunkTranscriber)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(PkgConfig)
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)

add_subdirectory(yaml-cpp)

# For the main executable
add_executable(sdrTrunkTranscriber main.cpp curlHelper.cpp DatabaseManager.cpp fileProcessor.cpp)
target_include_directories(sdrTrunkTranscriber PUBLIC
    /usr/include/ffmpeg/libavcodec
    /usr/include/ffmpeg/libavformat
    /usr/include/ffmpeg/libavutil
    ${CURL_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
    ${AVFORMAT_INCLUDE_DIRS}
    ${AVCODEC_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
)


target_link_libraries(sdrTrunkTranscriber
    CURL::libcurl
    yaml-cpp
    SQLite::SQLite3
    ${AVFORMAT_LIBRARIES}
    ${AVCODEC_LIBRARIES}
    ${AVUTIL_LIBRARIES}
)

# For tests
enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(runTests test.cpp curlHelper.cpp DatabaseManager.cpp fileProcessor.cpp)
target_link_libraries(runTests ${GTEST_LIBRARIES} pthread CURL::libcurl yaml-cpp SQLite::SQLite3)