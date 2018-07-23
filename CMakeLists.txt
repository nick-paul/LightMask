cmake_minimum_required (VERSION 3.0)
project (demo)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(SDL2 REQUIRED)

add_executable(demo LightMask.cpp demo.cpp)
target_link_libraries(demo ${SDL2_LIBRARIES})
