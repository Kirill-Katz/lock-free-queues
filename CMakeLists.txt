cmake_minimum_required(VERSION 3.20)
project(SPMCQueue LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(benchmark REQUIRED)

add_library(spscqueue
    src/spsc_queue.cpp
)
target_include_directories(spscqueue PUBLIC include)

add_executable(spsc_bench
    benchmark/main.cpp
    benchmark/bm_spsc.cpp
)
target_link_libraries(spsc_bench PRIVATE spscqueue benchmark::benchmark)
