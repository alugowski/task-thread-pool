cmake_minimum_required(VERSION 3.11)
# Read the version from task_thread_pool.hpp
set(VERSION_HEADER_FILE include/task_thread_pool.hpp)
file(STRINGS ${VERSION_HEADER_FILE} VERSION_MAJOR_STR REGEX "define .*_VERSION_MAJOR")
file(STRINGS ${VERSION_HEADER_FILE} VERSION_MINOR_STR REGEX "define .*_VERSION_MINOR")
file(STRINGS ${VERSION_HEADER_FILE} VERSION_PATCH_STR REGEX "define .*_VERSION_PATCH")
string(REGEX MATCH "[0-9]+" TP_MAJOR ${VERSION_MAJOR_STR})
string(REGEX MATCH "[0-9]+" TP_MINOR ${VERSION_MINOR_STR})
string(REGEX MATCH "[0-9]+" TP_PATCH ${VERSION_PATCH_STR})

project(task_thread_pool VERSION "${TP_MAJOR}.${TP_MINOR}.${TP_PATCH}" LANGUAGES CXX)

# Main
add_library(task_thread_pool INTERFACE)
# add alias so the project can be used with add_subdirectory
add_library(task-thread-pool::task-thread-pool ALIAS task_thread_pool)

target_include_directories(
        task_thread_pool INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# This library is compatible with C++11 and newer.
target_compile_features(task_thread_pool INTERFACE cxx_std_11)


###############################################

# Tests
option(TASK_THREAD_POOL_TEST "Enable tests" OFF)
if(TASK_THREAD_POOL_TEST)
    add_subdirectory(tests)
endif()

# Benchmarks
option(TASK_THREAD_POOL_BENCH "Enable benchmarks" OFF)
if(TASK_THREAD_POOL_BENCH)
    add_subdirectory(benchmark)
endif()
