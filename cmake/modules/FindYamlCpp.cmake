# FindYamlCpp.cmake - Find yaml-cpp library
#
# This module defines:
#  YAML_CPP_FOUND - True if yaml-cpp is found
#  YAML_CPP_INCLUDE_DIRS - Include directories for yaml-cpp
#  YAML_CPP_LIBRARIES - Libraries to link against
#  YAML_CPP_VERSION - Version of yaml-cpp found
#  yaml-cpp - Imported target (if found)

include(FindPackageHandleStandardArgs)

# Use pkg-config if available
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_YAML_CPP QUIET yaml-cpp)
endif()

# Find the include directory
find_path(YAML_CPP_INCLUDE_DIR
    NAMES yaml-cpp/yaml.h
    PATHS
        ${PC_YAML_CPP_INCLUDEDIR}
        ${PC_YAML_CPP_INCLUDE_DIRS}
        /usr/include
        /usr/local/include
        /opt/local/include
        ${CMAKE_PREFIX_PATH}/include
    PATH_SUFFIXES
        yaml-cpp
)

# Find the library
find_library(YAML_CPP_LIBRARY
    NAMES 
        yaml-cpp
        libyaml-cpp
    PATHS
        ${PC_YAML_CPP_LIBDIR}
        ${PC_YAML_CPP_LIBRARY_DIRS}
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        ${CMAKE_PREFIX_PATH}/lib
        ${CMAKE_PREFIX_PATH}/lib64
)

# Try to determine version
if(YAML_CPP_INCLUDE_DIR AND EXISTS "${YAML_CPP_INCLUDE_DIR}/yaml-cpp/dll.h")
    file(STRINGS "${YAML_CPP_INCLUDE_DIR}/yaml-cpp/dll.h" 
         YAML_CPP_VERSION_MAJOR_LINE 
         REGEX "^#define[ \t]+YAML_CPP_VERSION_MAJOR[ \t]+[0-9]+")
    file(STRINGS "${YAML_CPP_INCLUDE_DIR}/yaml-cpp/dll.h" 
         YAML_CPP_VERSION_MINOR_LINE 
         REGEX "^#define[ \t]+YAML_CPP_VERSION_MINOR[ \t]+[0-9]+")
    file(STRINGS "${YAML_CPP_INCLUDE_DIR}/yaml-cpp/dll.h" 
         YAML_CPP_VERSION_PATCH_LINE 
         REGEX "^#define[ \t]+YAML_CPP_VERSION_PATCH[ \t]+[0-9]+")
    
    string(REGEX REPLACE "^#define[ \t]+YAML_CPP_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1"
           YAML_CPP_VERSION_MAJOR "${YAML_CPP_VERSION_MAJOR_LINE}")
    string(REGEX REPLACE "^#define[ \t]+YAML_CPP_VERSION_MINOR[ \t]+([0-9]+).*" "\\1"
           YAML_CPP_VERSION_MINOR "${YAML_CPP_VERSION_MINOR_LINE}")
    string(REGEX REPLACE "^#define[ \t]+YAML_CPP_VERSION_PATCH[ \t]+([0-9]+).*" "\\1"
           YAML_CPP_VERSION_PATCH "${YAML_CPP_VERSION_PATCH_LINE}")
    
    set(YAML_CPP_VERSION "${YAML_CPP_VERSION_MAJOR}.${YAML_CPP_VERSION_MINOR}.${YAML_CPP_VERSION_PATCH}")
elseif(PC_YAML_CPP_VERSION)
    set(YAML_CPP_VERSION ${PC_YAML_CPP_VERSION})
endif()

# Handle standard arguments
find_package_handle_standard_args(YamlCpp
    REQUIRED_VARS YAML_CPP_LIBRARY YAML_CPP_INCLUDE_DIR
    VERSION_VAR YAML_CPP_VERSION
)

if(YAML_CPP_FOUND)
    set(YAML_CPP_LIBRARIES ${YAML_CPP_LIBRARY})
    set(YAML_CPP_INCLUDE_DIRS ${YAML_CPP_INCLUDE_DIR})
    
    # Create imported target
    if(NOT TARGET yaml-cpp)
        add_library(yaml-cpp UNKNOWN IMPORTED)
        set_target_properties(yaml-cpp PROPERTIES
            IMPORTED_LOCATION "${YAML_CPP_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${YAML_CPP_INCLUDE_DIR}"
        )
        
        # Check if we need to link against additional libraries
        include(CheckCXXSourceCompiles)
        set(CMAKE_REQUIRED_INCLUDES ${YAML_CPP_INCLUDE_DIR})
        set(CMAKE_REQUIRED_LIBRARIES ${YAML_CPP_LIBRARY})
        
        check_cxx_source_compiles("
            #include <yaml-cpp/yaml.h>
            int main() {
                YAML::Node node;
                return 0;
            }" YAML_CPP_COMPILES)
        
        if(NOT YAML_CPP_COMPILES)
            # Try linking with pthread
            set(CMAKE_REQUIRED_LIBRARIES ${YAML_CPP_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
            check_cxx_source_compiles("
                #include <yaml-cpp/yaml.h>
                int main() {
                    YAML::Node node;
                    return 0;
                }" YAML_CPP_COMPILES_WITH_PTHREAD)
            
            if(YAML_CPP_COMPILES_WITH_PTHREAD)
                set_target_properties(yaml-cpp PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}"
                )
            endif()
        endif()
    endif()
endif()

# Clean up
mark_as_advanced(YAML_CPP_INCLUDE_DIR YAML_CPP_LIBRARY)