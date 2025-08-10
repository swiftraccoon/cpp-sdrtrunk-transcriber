# CompilerFlags.cmake - Modern compiler flags and warnings setup

# Enable colors in diagnostics
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fdiagnostics-color=always)
endif()

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC specific flags
    set(COMPILER_WARNINGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wformat=2
        -Wformat-security
        -Wunused
        -Wno-unused-parameter
        -Wcast-align
        -Wpointer-arith
        -Wredundant-decls
        -Wundef
        -Wvla
        -Wcast-qual
        -Wconversion
        -Wsign-conversion
        -Wlogical-op
        -Wmissing-declarations
        -Wdouble-promotion
        -Wformat-nonliteral
        -Wformat-y2k
        -Winit-self
        -Wmissing-include-dirs
        -Wstrict-overflow=2
        -Wswitch-default
        -Wswitch-enum
        -Wuninitialized
        -Wstrict-aliasing=2
        -Wfloat-equal
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang specific flags
    set(COMPILER_WARNINGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wformat=2
        -Wunused
        -Wno-unused-parameter
        -Wcast-align
        -Wpointer-arith
        -Wredundant-decls
        -Wundef
        -Wvla
        -Wcast-qual
        -Wconversion
        -Wsign-conversion
        -Wmissing-declarations
        -Wdouble-promotion
        -Wformat-nonliteral
        -Winit-self
        -Wmissing-prototypes
        -Wstrict-overflow=2
        -Wswitch-default
        -Wswitch-enum
        -Wuninitialized
        -Wstrict-aliasing=2
        -Wfloat-equal
        -Wnull-dereference
        -Wduplicate-enum
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC specific flags
    set(COMPILER_WARNINGS
        /W3
        /w14242 # 'identfier': conversion from 'type1' to 'type1', possible loss of data
        /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /w14263 # 'function': member function does not override any base class virtual member function
        /w14265 # 'classname': class has virtual functions, but destructor is not virtual
        /w14287 # 'operator': unsigned/negative constant mismatch
        /w14289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
        /w14296 # 'operator': expression is always 'boolean_value'
        /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
        /w14545 # expression before comma evaluates to a function which is missing an argument list
        /w14546 # function call before comma missing argument list
        /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
        /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
        /w14555 # expression has no effect; expected expression with side-effect
        /w14619 # pragma warning: there is no warning number 'number'
        /w14640 # Enable warning on thread un-safe static member initialization
        /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
        /w14905 # wide string literal cast to 'LPSTR'
        /w14906 # string literal cast to 'LPWSTR'
        /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
        /permissive- # standards conformance mode for MSVC compiler.
    )
    
    # Additional MSVC definitions
    add_compile_definitions(
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    )
endif()

# Apply warning flags to all targets
add_compile_options(${COMPILER_WARNINGS})

# Optimization flags for different build types
if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
else()
    # MSVC has its own optimization flags
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /Zi /DNDEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "/O1 /DNDEBUG")
endif()

# Link-time optimization for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT error)
    if(ipo_supported)
        message(STATUS "IPO/LTO enabled for Release build")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "IPO/LTO not supported: <${error}>")
    endif()
endif()

# Security hardening flags (Linux/Unix)
if(UNIX AND NOT APPLE)
    add_compile_options(
        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2
    )
    add_link_options(
        -Wl,-z,relro
        -Wl,-z,now
        -Wl,-z,noexecstack
    )
endif()

# macOS specific flags
if(APPLE)
    add_compile_options(-fstack-protector-strong)
endif()