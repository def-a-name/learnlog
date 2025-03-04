cmake_minimum_required(VERSION 3.11)
project(learnlog_examples LANGUAGES CXX)

if(NOT TARGET learnlog)
    find_package(learnlog REQUIRED)
endif()

add_executable(example example.cpp)
target_link_libraries(example PRIVATE learnlog)