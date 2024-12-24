cmake_minimum_required(VERSION 3.11)
project(mylog_bench CXX)

if(NOT TARGET mylog)
    # Stand-alone build
    find_package(mylog REQUIRED)
endif()

find_package(Threads REQUIRED)
find_package(benchmark CONFIG)
if(NOT benchmark_FOUND)
    message(STATUS "Using CMake Version ${CMAKE_VERSION}")
    # User can fetch googlebenchmark
    message(STATUS "Downloading GoogleBenchmark")
    include(FetchContent)

    # disable tests
    set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "")
    # Do not build and run googlebenchmark tests
    FetchContent_Declare(googlebenchmark GIT_REPOSITORY https://github.com/google/benchmark.git GIT_TAG v1.6.0)
    FetchContent_MakeAvailable(googlebenchmark)
endif()
