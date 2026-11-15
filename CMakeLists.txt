cmake_minimum_required(VERSION 3.16)

project(abel VERSION 0.1.0 LANGUAGES CXX)

add_library(abel_tokenizer
    src/tokenizer.cpp
)

target_include_directories(abel_tokenizer PUBLIC src)
target_compile_features(abel_tokenizer PUBLIC cxx_std_17)

add_library(abel_parser
    src/parser.cpp
)

target_include_directories(abel_parser PUBLIC src)
target_compile_features(abel_parser PUBLIC cxx_std_17)
target_link_libraries(abel_parser PUBLIC abel_tokenizer)

add_executable(abel
    src/main.cpp
)

target_compile_features(abel PRIVATE cxx_std_17)
target_link_libraries(abel PRIVATE abel_tokenizer)

if (MSVC)
    target_compile_options(abel PRIVATE /W4)
else()
    target_compile_options(abel PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(abel_tokenizer PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(abel_parser PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)

if (BUILD_TESTING)
    add_executable(abel_tokenizer_tests
        tests/tokenizer_tests.cpp
    )
    target_compile_features(abel_tokenizer_tests PRIVATE cxx_std_17)
    target_link_libraries(abel_tokenizer_tests PRIVATE abel_tokenizer)
    if (MSVC)
        target_compile_options(abel_tokenizer_tests PRIVATE /W4)
    else()
        target_compile_options(abel_tokenizer_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME abel_tokenizer_tests COMMAND abel_tokenizer_tests)

    add_executable(abel_parser_tests
        tests/parser_tests.cpp
    )
    target_compile_features(abel_parser_tests PRIVATE cxx_std_17)
    target_link_libraries(abel_parser_tests PRIVATE abel_parser)
    if (MSVC)
        target_compile_options(abel_parser_tests PRIVATE /W4)
    else()
        target_compile_options(abel_parser_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME abel_parser_tests COMMAND abel_parser_tests)
endif()
