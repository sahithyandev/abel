cmake_minimum_required(VERSION 3.16)

project(abel VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

option(ABEL_BUILD_CLI "Build the native Abel CLI" ON)
option(ABEL_BUILD_WASM "Build the Abel WebAssembly runtime" OFF)

add_subdirectory(apps/cli)

if (ABEL_BUILD_WASM)
    add_subdirectory(apps/wasm)
endif()
