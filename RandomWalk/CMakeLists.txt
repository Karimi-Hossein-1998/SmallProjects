cmake_minimum_required(VERSION 3.16)
project(RandomWalk LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)

add_executable(walker walker.cpp)
target_include_directories(walker PRIVATE src)
target_link_libraries(walker PRIVATE PkgConfig::SDL3)

if(NOT MSVC)
    target_compile_options(walker PRIVATE -O3 -funroll-loops -ftree-vectorize)
endif()
