
project(TBT_Tests)

# Fetch Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.8.1
)
FetchContent_MakeAvailable(Catch2)

file(GLOB sources ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

add_executable( ${PROJECT_NAME} ${sources})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries( ${PROJECT_NAME} 
    TBT
    Catch2::Catch2WithMain
)