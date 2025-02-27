cmake_minimum_required(VERSION 3.11)
project(learnlog_examples LANGUAGES CXX)

if(NOT TARGET learnlog)
    # Stand-alone build
    find_package(learnlog REQUIRED)
endif()

add_executable(example example.cpp)
target_link_libraries(example PRIVATE learnlog)
if(LEARNLOG_SANITIZE_ADDRESS)
    spdlog_enable_address_sanitizer(example)
endif()
if(LEARNLOG_SANITIZE_THREAD)
    spdlog_enable_thread_sanitizer(example)
endif()