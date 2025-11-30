cmake_minimum_required(VERSION 3.21)

# Reproducible FetchContent timestamps
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
    cmake_policy(SET CMP0135 NEW)
endif()

project(TBT)

# --- C++ Standard ---
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)           # -std=c++23, not gnu++

# Export compile_commands.json for clangd, etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ===========================================================================
# User-configurable options for compiler flags
# ===========================================================================

option(TBT_ENABLE_HARDENING      "Enable strict warnings and hardening flags" ON)
option(TBT_ENABLE_MARCH_NATIVE   "Enable -march=native (makes binaries non-portable!)" OFF)
option(TBT_ENABLE_LTO            "Enable Link-Time Optimization (if supported)" OFF)

# ===========================================================================
# Try to find glaze locally first
# ===========================================================================

find_package(glaze QUIET)

if(NOT glaze_FOUND)
    message(STATUS "glaze not found locally - fetching from GitHub (main branch)")
    include(FetchContent)
    FetchContent_Declare(
        glaze
        GIT_REPOSITORY https://github.com/stephenberry/glaze.git
        GIT_TAG        main           
        GIT_SHALLOW    TRUE
        FIND_PACKAGE_ARGS NAMES glaze
    )
    FetchContent_MakeAvailable(glaze)
endif()

# ===========================================================================
# Interface library (header-only)
# ===========================================================================

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        glaze::glaze
)

# ===========================================================================
# Portable compiler flags (grouped and toggleable)
# ===========================================================================

# Helper function to append flags only if compiler supports them
function(tbt_target_compile_options target visibility)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|Apple")
        # --- GCC / Clang common flags ---
        set(common_flags
            -Wall -Wextra -Wpedantic
#            -Wunused-but-set-variable
#            -Wshadow
#            -Wnon-virtual-dtor
#            -Wold-style-cast
#            -Wcast-align
#            -Wunused
#            -Woverloaded-virtual
#            -Wconversion
#            -Wsign-conversion
#            -Wnull-dereference
#            -Wdouble-promotion
#            -Wformat=2
#            -Wimplicit-fallthrough
        )

        if(TBT_ENABLE_HARDENING)
            target_compile_options(${target} ${visibility} ${common_flags})
        endif()

        if(TBT_ENABLE_MARCH_NATIVE AND CMAKE_BUILD_TYPE STREQUAL "Release")
            target_compile_options(${target} ${visibility} -march=native)
        endif()

        # LTO / IPO
        if(TBT_ENABLE_LTO)
            include(CheckIPOSupported)
            check_ipo_supported(RESULT ipo_supported LANGUAGES CXX)
            if(ipo_supported)
                set_target_properties(${target} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
            endif()
        endif()

    elseif(MSVC)
        # --- MSVC flags ---
        set(msvc_flags
            /W4                     # High warning level
            /permissive-            # Standards conformance
            /Zc:__cplusplus         # Correct __cplusplus macro
            /Zc:preprocessor       # New preprocessor (closer to Clang/GCC)
            /EHsc                   # Standard C++ exceptions
            /bigobj             
        )

        if(TBT_ENABLE_HARDENING)
            list(APPEND msvc_flags
                /wd4996              # Allow some common "insecure" CRT functions if you need them
                # /weXXXX           # Treat specific warnings as errors (add as needed)
            )
        endif()

        if(TBT_ENABLE_MARCH_NATIVE AND CMAKE_BUILD_TYPE STREQUAL "Release")
            list(APPEND msvc_flags /arch:native)   # MSVC equivalent (x86/x64 only)
        endif()

        target_compile_options(${target} ${visibility} ${msvc_flags})
    endif()
endfunction()

# Apply the flags to our interface library
tbt_target_compile_options(${PROJECT_NAME} INTERFACE)

# ===========================================================================
# Tests
# ===========================================================================

add_subdirectory(tests)