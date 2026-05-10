
project(TBT_Tests)

# Fetch Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.8.1
)
FetchContent_MakeAvailable(Catch2)

file(GLOB sources ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

file(GLOB_RECURSE TBT_HEADERS
    "${CMAKE_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_SOURCE_DIR}/include/*.h"
)

add_executable(${PROJECT_NAME}
    ${sources}
    ${TBT_HEADERS}          # ← headers are now part of the target
)

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries( ${PROJECT_NAME} 
    TBT
    Catch2::Catch2WithMain
)