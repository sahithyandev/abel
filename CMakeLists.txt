cmake_minimum_required(VERSION 3.16)

project(abel VERSION 0.1.0 LANGUAGES CXX)

add_executable(abel
    src/main.cpp
)

target_compile_features(abel PRIVATE cxx_std_17)

if (MSVC)
    target_compile_options(abel PRIVATE /W4)
else()
    target_compile_options(abel PRIVATE -Wall -Wextra -Wpedantic)
endif()
