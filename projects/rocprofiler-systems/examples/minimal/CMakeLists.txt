# Copyright (c) Advanced Micro Devices, Inc.
# SPDX-License-Identifier:  MIT

cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(rocprofiler-systems-minimal LANGUAGES CXX)

# Support standalone builds
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake/standalone-helpers.cmake OPTIONAL)
endif()

add_executable(minimal-recursion ${CMAKE_CURRENT_SOURCE_DIR}/recursion.cpp)
# Keep recursive calls as real calls so the instrumenter sees every frame
# (noinline alone does not prevent tail-call / sibling-call optimization).
target_compile_options(minimal-recursion PRIVATE -fno-optimize-sibling-calls)

set(MINIMAL_EXAMPLES minimal-recursion)

# Minimal pthread example: exercises the pthread_create gotcha trace-args.
find_package(Threads REQUIRED)
add_executable(minimal-pthreads ${CMAKE_CURRENT_SOURCE_DIR}/pthreads.cpp)
target_link_libraries(minimal-pthreads PRIVATE Threads::Threads)
list(APPEND MINIMAL_EXAMPLES minimal-pthreads)

# Minimal NUMA example: exercises the numa_alloc gotcha trace-args. Only built
# when libnuma and its header are available.
find_library(NUMA_LIBRARY NAMES numa)
find_path(NUMA_INCLUDE_DIR NAMES numa.h)
if(NUMA_LIBRARY AND NUMA_INCLUDE_DIR)
    add_executable(minimal-numa-alloc ${CMAKE_CURRENT_SOURCE_DIR}/numa_alloc.cpp)
    target_include_directories(minimal-numa-alloc PRIVATE ${NUMA_INCLUDE_DIR})
    target_link_libraries(minimal-numa-alloc PRIVATE ${NUMA_LIBRARY})
    list(APPEND MINIMAL_EXAMPLES minimal-numa-alloc)
else()
    message(
        STATUS
        "[rocprofiler-systems] libnuma not found; skipping minimal-numa-alloc example"
    )
endif()

foreach(MINIMAL_EXAMPLE IN LISTS MINIMAL_EXAMPLES)
    if(ROCPROFSYS_INSTALL_EXAMPLES AND TARGET ${MINIMAL_EXAMPLE})
        install(
            TARGETS ${MINIMAL_EXAMPLE}
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/rocprofiler-systems/examples
            COMPONENT rocprofiler-systems-examples
        )
    endif()
endforeach()
