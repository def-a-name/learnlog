cmake_minimum_required(VERSION 3.11)

# ---------------------------------------------------------------------------------------
# Start mylog project
# ---------------------------------------------------------------------------------------
include(cmake/utils.cmake)

project(mylog LANGUAGES CXX)
message(STATUS "Build mylog")

# ---------------------------------------------------------------------------------------
# Set default build to Debug
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose Release or Debug")
endif()

# ---------------------------------------------------------------------------------------
# Compiler config
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# warning options
option(MYLOG_BUILD_WARNINGS "Enable compiler warnings" OFF)

# example options
option(MYLOG_BUILD_EXAMPLES "Build examples" ON)

# test options
option(MYLOG_BUILD_TESTS "Build tests" OFF)

# sanitizer options
option(MYLOG_SANITIZE_ADDRESS "Enable address sanitizer in tests" ON)

# bench options
option(MYLOG_BUILD_BENCH "Build benchmarks (Requires https://github.com/google/benchmark.git to be installed)" OFF)

# ---------------------------------------------------------------------------------------
# Package support
# ---------------------------------------------------------------------------------------
find_package(Threads REQUIRED)

find_package(fmt CONFIG)
if(fmt_FOUND)
    message(STATUS "Packaged version of fmt will be used.")
else()
    message(STATUS "Bundled version of fmt will be downloaded and used.")
    include(FetchContent)
    FetchContent_Declare(fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt
            GIT_TAG 0c9fce2ffefecfdce794e1859584e25877b7b592 # v11.0.2
    )
    FetchContent_MakeAvailable(fmt)
endif()

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

# ---------------------------------------------------------------------------------------
# Static library
# ---------------------------------------------------------------------------------------
set(MYLOG_HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(MYLOG_SRCS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_subdirectory(helloworld)
add_subdirectory(base)
add_subdirectory(sinks)

file(GLOB MYLOG_ROOT_HEADERS "${MYLOG_HEADERS_DIR}/*.h")
file(GLOB MYLOG_ROOT_SRCS "${MYLOG_SRCS_DIR}/*.cpp")

add_library(mylog STATIC ${MYLOG_ROOT_SRCS})
add_library(mylog::mylog ALIAS mylog)
target_include_directories(mylog PUBLIC 
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                            $<BUILD_INTERFACE:${MYLOG_HEADERS_DIR}>)
target_link_libraries(mylog mylog_helloworld mylog_base mylog_sinks)
spdlog_enable_warnings(mylog)

# ---------------------------------------------------------------------------------------
# Build binaries
# ---------------------------------------------------------------------------------------
if(MYLOG_BUILD_EXAMPLES)
    message(STATUS "Generating example(s)")
    add_subdirectory(example)
    spdlog_enable_warnings(example)
endif()

if(MYLOG_BUILD_TESTS)
    message(STATUS "Generating tests")
    enable_testing()
    add_subdirectory(tests)
endif()

if(MYLOG_BUILD_BENCH)
    message(STATUS "Generating benchmarks")
    add_subdirectory(bench)
endif()