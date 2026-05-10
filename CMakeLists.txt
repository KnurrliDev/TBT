cmake_minimum_required(VERSION 4.3.2)

# Reproducible FetchContent timestamps
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
    cmake_policy(SET CMP0135 NEW)
endif()

project(TBT)

# --- C++ Standard ---
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)           # -std=c++23, not gnu++ -freflection



if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /openmp")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od ")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 ")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -O0 -g3 -ggdb -D_GLIBCXX_DEBUG -Wall -Wextra -Wpedantic" )
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -ffast-math -DNDEBUG -flto -fomit-frame-pointer")
endif()


# Export compile_commands.json for clangd, etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ===========================================================================
# Try to find glaze locally first
# ===========================================================================

find_package(glaze QUIET)

set(glaze_ENABLE_REFLECTION26 OFF)

if(NOT glaze_FOUND)
    message(STATUS "glaze not found locally - fetching from GitHub (main branch)")
    include(FetchContent)
    FetchContent_Declare(
        glaze
        GIT_REPOSITORY https://github.com/stephenberry/glaze.git
        GIT_TAG        v7.5.0           
        FIND_PACKAGE_ARGS NAMES glaze
    )
    FetchContent_MakeAvailable(glaze)
endif()

# ===========================================================================
# Interface library (header-only)
# ===========================================================================

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        glaze::glaze
)

# ===========================================================================
# Tests
# ===========================================================================

add_subdirectory(tests)