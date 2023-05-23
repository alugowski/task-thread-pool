cmake_minimum_required(VERSION 3.12)
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

include(GNUInstallDirs)

target_include_directories(
        ${PROJECT_NAME}
        INTERFACE
        $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

set(HEADER_FILES task_thread_pool.hpp)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADER_FILES}")

# This library is compatible with C++11 and newer.
target_compile_features(task_thread_pool INTERFACE cxx_std_11)


###############################################

# Tests
option(TASK_THREAD_POOL_TEST "Enable task-thread-pool tests" OFF)
if(TASK_THREAD_POOL_TEST)
    add_subdirectory(tests)
endif()

# Benchmarks
option(TASK_THREAD_POOL_BENCH "Enable task-thread-pool benchmarks" OFF)
if(TASK_THREAD_POOL_BENCH)
    add_subdirectory(benchmark)
endif()

###############################################
# For cmake install:

include(CMakePackageConfigHelpers)

# Create version file
write_basic_package_version_file("${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

# Create a configuration file
configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/config.cmake.in"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/${PROJECT_NAME}
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR)

# Install config and version files
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/${PROJECT_NAME})

# Define the install targets
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

# Install the targets
install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/${PROJECT_NAME})
