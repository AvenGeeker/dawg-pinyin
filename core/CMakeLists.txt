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
    src/pinyin/lattice.cc
    src/pinyin/decoder.cc
    src/dawg/dawg_reader.cc
    src/lm/char_lm.cc
    src/engine.cc
    src/capi.cc
)

add_library(dawg_core STATIC ${DAWG_CORE_SOURCES})
target_include_directories(dawg_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(dawg_core PRIVATE
    -Wall -Wextra -Werror -fno-exceptions -fno-rtti)

# Offline builder library; host tools and tests link it, the runtime decoder
# does not.
add_library(dawg_builder STATIC src/dawg/dawg_builder.cc)
target_include_directories(dawg_builder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(dawg_builder PUBLIC dawg_core)
target_compile_options(dawg_builder PRIVATE
    -Wall -Wextra -Werror -fno-exceptions -fno-rtti)

# Host-only offline tools; excluded from cross builds (NDK / HarmonyOS).
if(NOT CMAKE_CROSSCOMPILING)
  add_executable(dawg_build_dict tools/build_dict.cc)
  target_include_directories(dawg_build_dict PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(dawg_build_dict PRIVATE dawg_builder)
  target_compile_options(dawg_build_dict PRIVATE -Wall -Wextra -Werror)

  add_executable(dawg_import_rime tools/import_rime.cc)
  target_include_directories(dawg_import_rime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(dawg_import_rime PRIVATE dawg_core)
  target_compile_options(dawg_import_rime PRIVATE -Wall -Wextra -Werror)

  add_executable(dawg_bench tools/bench.cc)
  target_include_directories(dawg_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(dawg_bench PRIVATE dawg_core)
  target_compile_options(dawg_bench PRIVATE -Wall -Wextra -Werror)
endif()

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
      tests/dawg_test.cc
      tests/char_lm_test.cc
      tests/decoder_test.cc
      tests/engine_test.cc
      tests/capi_test.cc
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
