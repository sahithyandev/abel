add_executable(abel_wasm src/wasm.cpp)
target_compile_features(abel_wasm PRIVATE cxx_std_17)
target_link_libraries(abel_wasm PRIVATE abel_evaluator)
target_compile_options(abel_wasm PRIVATE -Wall -Wextra -Wpedantic -fexceptions)

set_target_properties(abel_wasm PROPERTIES
    OUTPUT_NAME "abel"
    SUFFIX ".js"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/packages/wasm/dist"
)

target_link_options(abel_wasm PRIVATE
    "--bind"
    "-fexceptions"
    "-sMODULARIZE=1"
    "-sEXPORT_ES6=1"
    "-sENVIRONMENT=web"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sFILESYSTEM=0"
)
