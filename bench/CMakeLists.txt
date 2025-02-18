cmake_minimum_required(VERSION 3.11)
project(learnlog_bench CXX)

if(NOT TARGET learnlog)
    # Stand-alone build
    find_package(learnlog REQUIRED)
endif()

find_package(Threads REQUIRED)
# find_package(benchmark CONFIG)
# if(NOT benchmark_FOUND)
#     message(STATUS "Using CMake Version ${CMAKE_VERSION}")
#     # User can fetch googlebenchmark
#     message(STATUS "Downloading GoogleBenchmark")
#     include(FetchContent)

#     # disable tests
#     set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "")
#     # Do not build and run googlebenchmark tests
#     FetchContent_Declare(googlebenchmark GIT_REPOSITORY https://github.com/google/benchmark.git GIT_TAG v1.6.0)
#     FetchContent_MakeAvailable(googlebenchmark)
# endif()

add_executable(async_queue_bench async_queue_bench.cpp)
spdlog_enable_warnings(async_queue_bench)
target_link_libraries(async_queue_bench PRIVATE learnlog)