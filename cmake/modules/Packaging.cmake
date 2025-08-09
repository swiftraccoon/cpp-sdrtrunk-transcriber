# Packaging.cmake - CPack configuration for creating packages

include(InstallRequiredSystemLibraries)

# Basic package information
set(CPACK_PACKAGE_NAME "sdrtrunk-transcriber")
set(CPACK_PACKAGE_VENDOR "SDRTrunk Community")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C++ application for transcribing SDRTrunk P25 recordings")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "sdrtrunk-transcriber")

# Package description
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Package maintainer
set(CPACK_PACKAGE_CONTACT "https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber")

# Source package configuration
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-source")
set(CPACK_SOURCE_IGNORE_FILES
    "/\\.git/"
    "/build/"
    "/build-.*/"
    "/\\.vscode/"
    "/\\.idea/"
    "\\.gitignore$"
    "\\.gitmodules$"
    "\\.clang-format$"
    "\\.clang-tidy$"
    ".*~$"
    "\\.swp$"
    "\\.orig$"
    "/CMakeFiles/"
    "CMakeCache\\.txt$"
    "Makefile$"
    "cmake_install\\.cmake$"
    "install_manifest\\.txt$"
    "/_CPack_Packages/"
    "\\.tar\\.gz$"
    "\\.deb$"
    "\\.rpm$"
)

# Components
set(CPACK_COMPONENTS_ALL Runtime)
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "SDRTrunk Transcriber Runtime")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Main executable and runtime files for SDRTrunk Transcriber")

# Platform-specific configurations
if(WIN32)
    # Windows (NSIS) configuration
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "SDRTrunk Transcriber")
    set(CPACK_NSIS_PACKAGE_NAME "SDRTrunk Transcriber")
    set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_NSIS_HELP_LINK "https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    
    # Create desktop shortcut
    set(CPACK_CREATE_DESKTOP_LINKS sdrtrunk-transcriber)
    
elseif(APPLE)
    # macOS configuration
    set(CPACK_GENERATOR "DragNDrop;TGZ")
    set(CPACK_DMG_FORMAT "UDZO")
    set(CPACK_DMG_VOLUME_NAME "SDRTrunk Transcriber ${PROJECT_VERSION}")
    
elseif(UNIX)
    # Linux configuration
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    
    # Debian package configuration
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_DEBIAN_PACKAGE_SECTION "misc")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcurl4, libsqlite3-0, libc6")
    set(CPACK_DEBIAN_PACKAGE_SUGGESTS "python3")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber")
    
    # RPM package configuration
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
    set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0")
    set(CPACK_RPM_PACKAGE_URL "https://github.com/swiftraccoon/cpp-sdrtrunk-transcriber")
    set(CPACK_RPM_PACKAGE_REQUIRES "libcurl, sqlite, glibc")
    set(CPACK_RPM_PACKAGE_SUGGESTS "python3")
    
    # Generate package file names with architecture
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
endif()

# Archive configuration
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

# Include CPack
include(CPack)

# Print packaging information
message(STATUS "=== Packaging Configuration ===")
message(STATUS "Package name: ${CPACK_PACKAGE_NAME}")
message(STATUS "Package version: ${CPACK_PACKAGE_VERSION}")
message(STATUS "Generators: ${CPACK_GENERATOR}")
message(STATUS "===============================")