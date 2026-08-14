# SPDX-License-Identifier: Apache-2.0
# HarmonyOS NAPI glue around the shared dawg core. The core stays the single
# source of algorithm logic (repo AGENTS.md); this directory only wraps it.
# NOTE: requires the HarmonyOS Native SDK component (DevEco SDK Manager);
# see platform/HarmonyOS/README.md.
cmake_minimum_required(VERSION 3.22)
project(dawg_napi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# entry/src/main/cpp -> repo root is six levels up.
set(DAWG_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../core")
add_subdirectory("${DAWG_CORE_DIR}" "${CMAKE_BINARY_DIR}/dawg_core")

add_library(dawg_napi SHARED dawg_napi.cc)
target_compile_options(dawg_napi PRIVATE -Wall -Wextra -Werror)
target_link_libraries(dawg_napi PRIVATE dawg_core libace_napi.z.so)
