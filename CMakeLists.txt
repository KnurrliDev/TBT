cmake_minimum_required(VERSION 3.29.2)

include(FetchContent)

project(TBT)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

enable_language(CXX)
enable_language(C)

add_compile_options(
  /EHsc
  /Zc:preprocessor
  /diagnostics:caret
  /bigobj
)

add_library( ${PROJECT_NAME} INTERFACE )
target_include_directories(${PROJECT_NAME} INTERFACE include)
target_link_libraries(${PROJECT_NAME} INTERFACE)

add_subdirectory(tests)