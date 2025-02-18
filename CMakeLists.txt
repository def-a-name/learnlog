cmake_minimum_required(VERSION 3.11)

# ---------------------------------------------------------------------------------------
# Start learnlog project
# ---------------------------------------------------------------------------------------
include(cmake/utils.cmake)

project(learnlog LANGUAGES CXX)
message(STATUS "Build learnlog")

# ---------------------------------------------------------------------------------------
# Set default build to Release
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug")
endif()

# ---------------------------------------------------------------------------------------
# Compiler config
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# warning options
option(LEARNLOG_BUILD_WARNINGS "Enable compiler warnings" OFF)

# example options
option(LEARNLOG_BUILD_EXAMPLES "Build examples" ON)

# test options
option(LEARNLOG_BUILD_TESTS "Build tests" OFF)

# sanitizer options
option(LEARNLOG_SANITIZE_ADDRESS "Enable address sanitizer in tests" ON)

# bench options
option(LEARNLOG_BUILD_BENCH "Build benchmarks" OFF)

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
set(LEARNLOG_HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(LEARNLOG_SRCS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_subdirectory(base)
add_subdirectory(sinks)

file(GLOB LEARNLOG_ROOT_HEADERS "${LEARNLOG_HEADERS_DIR}/*.h")
file(GLOB LEARNLOG_ROOT_SRCS "${LEARNLOG_SRCS_DIR}/*.cpp")

add_library(learnlog STATIC ${LEARNLOG_ROOT_SRCS})
add_library(learnlog::learnlog ALIAS learnlog)
target_include_directories(learnlog PUBLIC 
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                            $<BUILD_INTERFACE:${LEARNLOG_HEADERS_DIR}>)
target_link_libraries(learnlog learnlog_base learnlog_sinks)
spdlog_enable_warnings(learnlog)

# ---------------------------------------------------------------------------------------
# Build binaries
# ---------------------------------------------------------------------------------------
if(LEARNLOG_BUILD_EXAMPLES)
    message(STATUS "Generating example(s)")
    add_subdirectory(example)
    spdlog_enable_warnings(example)
endif()

if(LEARNLOG_BUILD_TESTS)
    message(STATUS "Generating tests")
    enable_testing()
    add_subdirectory(tests)
endif()

if(LEARNLOG_BUILD_BENCH)
    message(STATUS "Generating benchmarks")
    add_subdirectory(bench)
endif()