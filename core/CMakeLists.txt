cmake_minimum_required(VERSION 3.22)
project(dawg_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DAWG_BUILD_TESTS "Build unit tests" ON)

set(DAWG_CORE_SOURCES
    src/error.cc
    src/util/mmap_file.cc
    src/util/utf8.cc
)

add_library(dawg_core STATIC ${DAWG_CORE_SOURCES})
target_include_directories(dawg_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(dawg_core PRIVATE
    -Wall -Wextra -Werror -fno-exceptions -fno-rtti)
