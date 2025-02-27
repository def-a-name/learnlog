cmake_minimum_required(VERSION 3.11)
project(learnlog_bench CXX)

if(NOT TARGET learnlog)
    # Stand-alone build
    find_package(learnlog REQUIRED)
endif()

find_package(Threads REQUIRED)

add_executable(async_queue_bench async_queue_bench.cpp)
spdlog_enable_warnings(async_queue_bench)
target_link_libraries(async_queue_bench PRIVATE learnlog)
if(LEARNLOG_SANITIZE_ADDRESS)
    spdlog_enable_address_sanitizer(example)
endif()
if(LEARNLOG_SANITIZE_THREAD)
    spdlog_enable_thread_sanitizer(example)
endif()