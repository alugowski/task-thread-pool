# FetchContent requires cmake >=3.11
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
enable_testing()

# Be pedantic for clean code
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -pedantic")
endif()

# Code Coverage
option(TASK_THREAD_POOL_TEST_COVERAGE "Code-coverage" OFF)
if(TASK_THREAD_POOL_TEST_COVERAGE)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options("--coverage")
        add_link_options("-lgcov" "--coverage")
    endif()
endif()

# install Catch2
Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.3.2
)

FetchContent_MakeAvailable(Catch2)

add_executable(functionality_tests basic_test.cpp)
target_link_libraries(functionality_tests PRIVATE Catch2::Catch2WithMain task-thread-pool::task-thread-pool)
target_compile_definitions(functionality_tests PUBLIC CATCH_CONFIG_FAST_COMPILE)

add_executable(stress_tests stress_test.cpp)
target_link_libraries(stress_tests PRIVATE Catch2::Catch2WithMain task-thread-pool::task-thread-pool)
target_compile_definitions(stress_tests PUBLIC CATCH_CONFIG_FAST_COMPILE)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(functionality_tests)
catch_discover_tests(stress_tests)
