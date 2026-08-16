cmake_minimum_required(VERSION 3.16)
project(turbokv VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -pthread)
endif()

include_directories(include)

# Core engine library
add_library(turbokv_core
    src/engine.cpp
    src/wal.cpp
    src/server.cpp
)

# Server executable
add_executable(turbokv_server src/main.cpp)
target_link_libraries(turbokv_server PRIVATE turbokv_core)

# CLI benchmark tool
add_executable(turbokv_bench benchmarks/bench.cpp)
target_link_libraries(turbokv_bench PRIVATE turbokv_core)

# Tests
enable_testing()
add_executable(turbokv_tests tests/test_engine.cpp)
target_link_libraries(turbokv_tests PRIVATE turbokv_core)
add_test(NAME CoreEngineTests COMMAND turbokv_tests)
