# SPDX-License-Identifier: Apache-2.0
# Android JNI glue around the shared dawg core. The core stays the single
# source of algorithm logic (repo AGENTS.md); this directory only wraps it.
cmake_minimum_required(VERSION 3.22)
project(dawg_jni LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# app/src/main/cpp -> repo root is six levels up.
set(DAWG_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../core")
add_subdirectory("${DAWG_CORE_DIR}" "${CMAKE_BINARY_DIR}/dawg_core")

add_library(dawg_jni SHARED dawg_jni.cc)
target_compile_options(dawg_jni PRIVATE -Wall -Wextra -Werror)
target_link_libraries(dawg_jni PRIVATE dawg_core)
