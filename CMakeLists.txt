cmake_minimum_required(VERSION 3.20)
project(SPMCQueue LANGUAGES CXX)

include(CheckCXXCompilerFlag)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(benchmark REQUIRED)
find_package(absl REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

add_library(spscqueue src/spsc_buffer.cpp)
target_include_directories(spscqueue PUBLIC include)

add_executable(spmc_bench
    src/main.cpp
    src/spmc_burst_bench.cpp
)
target_compile_options(spmc_bench PRIVATE -g -fno-omit-frame-pointer -O3 -march=native)
target_link_libraries(spmc_bench PRIVATE spscqueue benchmark::benchmark)
target_link_libraries(spmc_bench PRIVATE absl::flat_hash_map)

function(add_spmc_bench_sanitizer_target target_name)
    cmake_parse_arguments(ARG "" "" "SANITIZERS" ${ARGN})

    add_library(${target_name}_queue src/spsc_buffer.cpp)
    target_include_directories(${target_name}_queue PUBLIC include)

    target_compile_options(${target_name}_queue PRIVATE
        -g
        -fno-omit-frame-pointer
        -O1
        -fPIE
        ${ARG_SANITIZERS}
    )

    add_executable(${target_name}
        src/main.cpp
        src/spmc_burst_bench.cpp
    )

    target_compile_options(${target_name} PRIVATE
        -g
        -fno-omit-frame-pointer
        -O1
        -fPIE
        ${ARG_SANITIZERS}
    )

    target_link_options(${target_name} PRIVATE
        -pie
        ${ARG_SANITIZERS}
    )

    target_link_libraries(${target_name} PRIVATE
        ${target_name}_queue
        benchmark::benchmark
        absl::flat_hash_map
        pthread
    )
endfunction()

add_spmc_bench_sanitizer_target(spmc_bench_asan_ubsan_lsan
    SANITIZERS -fsanitize=address,undefined,leak
)

add_spmc_bench_sanitizer_target(spmc_bench_tsan
    SANITIZERS -fsanitize=thread
)
