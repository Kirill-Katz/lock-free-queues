cmake_minimum_required(VERSION 3.20)
project(SPMCQueue LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(benchmark REQUIRED)
find_package(absl REQUIRED)

add_library(spscqueue src/spsc_buffer.cpp)
target_include_directories(spscqueue PUBLIC include)

add_executable(spmc_bench
    src/main.cpp
)

target_compile_options(spmc_bench PRIVATE -g -fno-omit-frame-pointer -O3 -march=native)
target_link_libraries(spmc_bench PRIVATE spscqueue benchmark::benchmark)
target_link_libraries(spmc_bench PRIVATE absl::flat_hash_map)


add_executable(moody_bench
    benchmark/main.cpp
    benchmark/bm_moody.cpp
)
target_include_directories(moody_bench PUBLIC include)
target_link_libraries(moody_bench PRIVATE spscqueue benchmark::benchmark)
