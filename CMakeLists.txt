cmake_minimum_required(VERSION 4.3.2)

# Reproducible FetchContent timestamps
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
    cmake_policy(SET CMP0135 NEW)
endif()

project(TBT)

# --- C++ Standard ---
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)           # -std=c++23, not gnu++ 

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /openmp")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od ")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -freflection -fconstexpr-depth=1024 -fconstexpr-ops-limit=1000000 -fdiagnostics-color=always -fno-elide-type -ftemplate-backtrace-limit=0 -fdiagnostics-show-option -fmessage-length=0 -fmax-errors=0 -fdiagnostics-show-template-tree")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -O0 -g3 -ggdb -Wall -Wextra -Wpedantic" )
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -ffast-math -DNDEBUG -flto -fomit-frame-pointer")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ===========================================================================
# Interface library (header-only)
# ===========================================================================

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
)

# ===========================================================================
# Tests
# ===========================================================================

add_subdirectory(tests)