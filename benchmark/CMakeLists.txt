# FetchContent requires cmake >=3.11
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -DNDEBUG")
endif ()

include(FetchContent)

# Add Google Benchmark
set(BENCHMARK_ENABLE_TESTING NO)

include(FetchContent)

FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG origin/main
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(googlebenchmark)


add_executable(task_thread_pool_bench task_thread_pool_bench.cpp)
target_link_libraries(task_thread_pool_bench benchmark::benchmark task-thread-pool::task-thread-pool)
