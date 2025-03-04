cmake_minimum_required(VERSION 3.11)
project(learnlog_bench CXX)

if(NOT TARGET learnlog)
    find_package(learnlog REQUIRED)
endif()

find_package(Threads REQUIRED)

function(learnlog_prepare_bench bench_target bench_srcs learnlog_lib)
    add_executable(${bench_target} ${bench_srcs})
    target_link_libraries(${bench_target} PRIVATE ${learnlog_lib})
    spdlog_enable_warnings(${bench_target})
endfunction()

if(LEARNLOG_BUILD_BENCH)
    learnlog_prepare_bench(async_queue_bench "async_queue_bench.cpp" learnlog)
    learnlog_prepare_bench(async_thread_pool_bench "async_thread_pool_bench.cpp" learnlog)
endif()