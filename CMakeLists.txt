cmake_minimum_required(VERSION 3.29.2)

include(FetchContent)

project(TBT)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_library( ${PROJECT_NAME} INTERFACE )
target_include_directories(${PROJECT_NAME} INTERFACE include)
target_link_libraries(${PROJECT_NAME} INTERFACE)

add_subdirectory(tests)