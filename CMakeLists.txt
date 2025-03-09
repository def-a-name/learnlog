cmake_minimum_required(VERSION 3.11)

# ---------------------------------------------------------------------------------------
# Start learnlog project
# ---------------------------------------------------------------------------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/utils.cmake)

project(learnlog LANGUAGES CXX VERSION 1.0.0)
message(STATUS "Build learnlog")

# ---------------------------------------------------------------------------------------
# Set default build to Release
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

# ---------------------------------------------------------------------------------------
# Compiler config
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# example option
option(LEARNLOG_BUILD_EXAMPLE "Build examples" ON)

# test option
option(LEARNLOG_BUILD_TEST "Build tests" OFF)

# bench option
option(LEARNLOG_BUILD_BENCH "Build benchmarks" OFF)

# sanitizer option
option(LEARNLOG_SANITIZE_ADDRESS "Enable address sanitizer in tests" OFF)
option(LEARNLOG_SANITIZE_THREAD "Enable thread sanitizer in tests" OFF)
if (LEARNLOG_SANITIZE_ADDRESS AND LEARNLOG_SANITIZE_THREAD)
    message(FATAL_ERROR "Sanitizers are mutually exclusive")
endif ()

# warning option
option(LEARNLOG_BUILD_WARNING "Enable compiler warnings" OFF)

# thread local support option
option(LEARNLOG_USE_TLS "Use thread local storage" ON)

# fmt source option
option(LEARNLOG_USE_BUNDLED_FMT "Use bundled fmt library instead of external" ON)

# ---------------------------------------------------------------------------------------
# Package support
# ---------------------------------------------------------------------------------------
find_package(Threads REQUIRED)

if (LEARNLOG_USE_BUNDLED_FMT)
    message(STATUS "Bundled version of fmt will be used.")
else()
    find_package(fmt CONFIG)
    if(fmt_FOUND)
        message(STATUS "Package fmt is found.")
    else()
        message(STATUS "Latest version of fmt will be downloaded and used.")
        include(FetchContent)
        FetchContent_Declare(fmt
                GIT_REPOSITORY https://github.com/fmtlib/fmt
                # GIT_TAG 0c9fce2ffefecfdce794e1859584e25877b7b592 # v11.0.2
        )
        FetchContent_MakeAvailable(fmt)
    endif()
endif()


message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

# ---------------------------------------------------------------------------------------
# Static library
# ---------------------------------------------------------------------------------------
set(LEARNLOG_HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(LEARNLOG_SRCS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in 
               ${CMAKE_CURRENT_SOURCE_DIR}/include/version.h)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/base)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sinks)

file(GLOB LEARNLOG_ROOT_HEADERS "${LEARNLOG_HEADERS_DIR}/*.h")
file(GLOB LEARNLOG_ROOT_SRCS "${LEARNLOG_SRCS_DIR}/*.cpp")

add_library(learnlog STATIC ${LEARNLOG_ROOT_SRCS})
add_library(learnlog::learnlog ALIAS learnlog)
target_link_libraries(learnlog PUBLIC learnlog_base learnlog_sinks)
target_include_directories(learnlog PUBLIC 
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<BUILD_INTERFACE:${LEARNLOG_HEADERS_DIR}>)
spdlog_enable_warnings(learnlog)

# ---------------------------------------------------------------------------------------
# Compile options
# ---------------------------------------------------------------------------------------
# foreach(LEARNLOG_OPTION
#         LEARNLOG_USE_TLS
#         LEARNLOG_USE_BUNDLED_FMT)
#     if(${LEARNLOG_OPTION})
#         target_compile_definitions(learnlog PUBLIC ${LEARNLOG_OPTION})
#     endif()
# endforeach()

# ---------------------------------------------------------------------------------------
# Build binaries
# ---------------------------------------------------------------------------------------
if(LEARNLOG_BUILD_EXAMPLE)
    message(STATUS "Generating example(s)")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/example)
    spdlog_enable_warnings(example)
endif()

if(LEARNLOG_BUILD_TEST)
    message(STATUS "Generating tests")
    enable_testing()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
endif()

if(LEARNLOG_BUILD_BENCH)
    message(STATUS "Generating benchmarks")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bench)
endif()