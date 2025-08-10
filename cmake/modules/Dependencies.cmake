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

# yaml-cpp - YAML parsing library
if(USE_SYSTEM_DEPS)
    find_package(yaml-cpp QUIET)
endif()

if(NOT TARGET yaml-cpp)
    message(STATUS "Using FetchContent for yaml-cpp")
    FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 0.8.0
    )
    
    # Configure yaml-cpp options
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "")
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "")
    set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "")
    set(YAML_CPP_INSTALL OFF CACHE BOOL "")
    
    FetchContent_MakeAvailable(yaml-cpp)
else()
    message(STATUS "Using system yaml-cpp")
endif()

# CLI11 - Command line parser
if(USE_SYSTEM_DEPS)
    find_package(CLI11 QUIET)
endif()

if(NOT TARGET CLI11::CLI11)
    message(STATUS "Using FetchContent for CLI11")
    FetchContent_Declare(
        CLI11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG v2.4.1
    )
    
    # Configure CLI11 options
    set(CLI11_BUILD_TESTS OFF CACHE BOOL "")
    set(CLI11_BUILD_EXAMPLES OFF CACHE BOOL "")
    set(CLI11_BUILD_DOCS OFF CACHE BOOL "")
    set(CLI11_INSTALL OFF CACHE BOOL "")
    
    FetchContent_MakeAvailable(CLI11)
else()
    message(STATUS "Using system CLI11")
endif()

# nlohmann/json - JSON library
if(USE_SYSTEM_DEPS)
    find_package(nlohmann_json QUIET)
endif()

if(NOT TARGET nlohmann_json::nlohmann_json)
    # Check if external/json exists first
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include/nlohmann/json.hpp")
        message(STATUS "Using bundled nlohmann/json from external/json")
        add_library(nlohmann_json_external INTERFACE)
        target_include_directories(nlohmann_json_external INTERFACE 
            "${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include"
        )
        add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json_external)
    else()
        message(STATUS "Using FetchContent for nlohmann/json")
        FetchContent_Declare(
            nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.11.3
        )
        FetchContent_MakeAvailable(nlohmann_json)
    endif()
else()
    message(STATUS "Using system nlohmann_json")
endif()

# Check if external subdirectories exist and prefer them when not using system deps
if(NOT USE_SYSTEM_DEPS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/yaml-cpp/CMakeLists.txt" AND NOT TARGET yaml-cpp)
        message(STATUS "Using bundled yaml-cpp from external/yaml-cpp")
        add_subdirectory(external/yaml-cpp)
    endif()
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/CLI11/CMakeLists.txt" AND NOT TARGET CLI11::CLI11)
        message(STATUS "Using bundled CLI11 from external/CLI11")
        add_subdirectory(external/CLI11)
    endif()
endif()

# Threads (required for testing and potentially for the main app)
find_package(Threads REQUIRED)

# Print dependency information
message(STATUS "=== Dependency Summary ===")
message(STATUS "CURL found: ${CURL_FOUND}")
message(STATUS "SQLite3 found: ${SQLite3_FOUND}")
if(TARGET yaml-cpp)
    message(STATUS "yaml-cpp: Available (target exists)")
else()
    message(STATUS "yaml-cpp: Not available")
endif()
if(TARGET CLI11::CLI11)
    message(STATUS "CLI11: Available (target exists)")
else()
    message(STATUS "CLI11: Not available")
endif()
if(TARGET nlohmann_json::nlohmann_json)
    message(STATUS "nlohmann_json: Available (target exists)")
else()
    message(STATUS "nlohmann_json: Not available")
endif()
message(STATUS "=========================")