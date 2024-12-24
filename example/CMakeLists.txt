cmake_minimum_required(VERSION 3.11)
project(mylog_examples LANGUAGES CXX)

if(NOT TARGET mylog)
    # Stand-alone build
    find_package(mylog REQUIRED)
endif()

add_executable(example example.cpp)
target_link_libraries(example PRIVATE mylog)
