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
    src/pinyin/syllable_table.cc
    src/pinyin/syllabifier.cc
)

add_library(dawg_core STATIC ${DAWG_CORE_SOURCES})
target_include_directories(dawg_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(dawg_core PRIVATE
    -Wall -Wextra -Werror -fno-exceptions -fno-rtti)

if(DAWG_BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  include(GoogleTest)

  set(DAWG_TEST_SOURCES
      tests/syllabifier_test.cc
  )
  foreach(test_source ${DAWG_TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(${test_name} PRIVATE
        dawg_builder GTest::gtest_main)
    target_compile_options(${test_name} PRIVATE -Wall -Wextra)
    target_compile_definitions(${test_name} PRIVATE
        DAWG_TESTDATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/testdata")
    gtest_discover_tests(${test_name})
  endforeach()
endif()
