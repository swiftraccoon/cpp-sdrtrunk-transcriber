# Dependencies.cmake - Modern dependency management

include(FetchContent)
include(FindPkgConfig)

# Set FetchContent options
set(FETCHCONTENT_QUIET FALSE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# CURL - Required for HTTP requests
if(WIN32)
    # On Windows, always use FetchContent for CURL if not using vcpkg
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR NOT CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
        message(STATUS "Using FetchContent for CURL on Windows")
        FetchContent_Declare(
            curl
            GIT_REPOSITORY https://github.com/curl/curl.git
            GIT_TAG curl-8_5_0
            GIT_SHALLOW TRUE
        )
        set(BUILD_CURL_EXE OFF CACHE BOOL "")
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
        set(CURL_ENABLE_SSL ON CACHE BOOL "")
        set(CURL_USE_SCHANNEL ON CACHE BOOL "")  # Use Windows SSL
        set(BUILD_TESTING OFF CACHE BOOL "")
        FetchContent_MakeAvailable(curl)
        if(NOT TARGET CURL::libcurl)
            add_library(CURL::libcurl ALIAS libcurl)
        endif()
        set(CURL_FOUND TRUE)
    else()
        find_package(CURL REQUIRED)
    endif()
else()
    find_package(CURL REQUIRED)
    if(NOT CURL_FOUND)
        message(FATAL_ERROR "CURL is required but not found. Please install libcurl-dev or equivalent.")
    endif()
endif()

# SQLite3 - Required for database operations
if(WIN32)
    # On Windows, use FetchContent for SQLite3 if not using vcpkg
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR NOT CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
        message(STATUS "Using FetchContent for SQLite3 on Windows")
        FetchContent_Declare(
            sqlite3
            URL https://www.sqlite.org/2024/sqlite-amalgamation-3450000.zip
            URL_HASH SHA256=bde30d13ebdf84926ddd5e8b6df145be03a577a48fd075a087a5dd815bcdf740
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(sqlite3)
        
        # Create SQLite3 library target
        add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
        target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})
        add_library(SQLite::SQLite3 ALIAS sqlite3)
        set(SQLite3_FOUND TRUE)
    else()
        find_package(unofficial-sqlite3 CONFIG QUIET)
        if(TARGET unofficial::sqlite3::sqlite3)
            add_library(SQLite::SQLite3 ALIAS unofficial::sqlite3::sqlite3)
            set(SQLite3_FOUND TRUE)
        else()
            find_package(SQLite3 REQUIRED)
        endif()
    endif()
else()
    find_package(SQLite3 REQUIRED)
    if(NOT SQLite3_FOUND AND NOT TARGET SQLite::SQLite3)
        message(FATAL_ERROR "SQLite3 is required but not found. Please install libsqlite3-dev or equivalent.")
    endif()
endif()


# libmpg123 - Required for accurate MP3 duration extraction
if(WIN32)
    # On Windows, use vcpkg or manual installation
    if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
        find_package(mpg123 CONFIG REQUIRED)
        # vcpkg creates MPG123::libmpg123 target but doesn't set the
        # MPG123_LIBRARIES/MPG123_INCLUDE_DIRS variables used by CMakeLists.txt
        set(MPG123_LIBRARIES MPG123::libmpg123)
        set(MPG123_INCLUDE_DIRS "")
    else()
        # Try to find mpg123 manually
        find_path(MPG123_INCLUDE_DIR mpg123.h)
        find_library(MPG123_LIBRARY NAMES mpg123)
        if(MPG123_INCLUDE_DIR AND MPG123_LIBRARY)
            add_library(mpg123 INTERFACE)
            target_include_directories(mpg123 INTERFACE ${MPG123_INCLUDE_DIR})
            target_link_libraries(mpg123 INTERFACE ${MPG123_LIBRARY})
            set(MPG123_LIBRARIES mpg123)
            set(MPG123_INCLUDE_DIRS ${MPG123_INCLUDE_DIR})
            set(MPG123_FOUND TRUE)
        else()
            message(FATAL_ERROR "mpg123 is required but not found. Please install mpg123 or use vcpkg.")
        endif()
    endif()
else()
    # On Linux/macOS, use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MPG123 REQUIRED libmpg123)
    if(NOT MPG123_FOUND)
        message(FATAL_ERROR "libmpg123 is required but not found. Please install libmpg123-dev or mpg123-devel.")
    endif()
endif()

# Threads (required for testing and potentially for the main app)
find_package(Threads REQUIRED)

# pybind11 - For Python integration (faster-whisper support)
option(USE_LOCAL_TRANSCRIPTION "Enable local transcription with Python/faster-whisper" ON)
if(USE_LOCAL_TRANSCRIPTION)
    # First try to find system pybind11
    find_package(pybind11 QUIET)
    
    if(NOT pybind11_FOUND)
        message(STATUS "pybind11 not found in system, fetching from GitHub...")
        FetchContent_Declare(
            pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11.git
            GIT_TAG v2.11.1  # Latest stable release
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(pybind11)
    else()
        message(STATUS "Using system pybind11")
    endif()
    
    # Find Python interpreter and development components
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    message(STATUS "Python3 found: ${Python3_EXECUTABLE}")
    message(STATUS "Python3 version: ${Python3_VERSION}")
endif()

# Print dependency information
message(STATUS "=== Dependency Summary ===")
message(STATUS "CURL found: ${CURL_FOUND}")
message(STATUS "SQLite3 found: ${SQLite3_FOUND}")
message(STATUS "In-house parsers: yamlParser, jsonParser, commandLineParser")
if(USE_LOCAL_TRANSCRIPTION)
    message(STATUS "pybind11 found: ${pybind11_FOUND}")
    message(STATUS "Python3 found: ${Python3_FOUND}")
endif()
message(STATUS "=========================")