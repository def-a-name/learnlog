#include <catch2/catch_all.hpp>
#include "base/exception.h"
#include "definitions.h"

#include <vector>
#include <thread>
#include <string>

void throw_error(int cnt) {
    learnlog::source_loc s;
    try{
        std::string info = "err" + std::to_string(cnt);
        throw std::runtime_error(info);
    }
    LEARNLOG_CATCH
}

TEST_CASE("test_handle_exception_std_thread", "[exception]") {
    std::vector<std::thread> threads;
    int thread_num = 10;
    int i;
    for (i = 0; i < thread_num; ++i) {
        threads.emplace_back(throw_error, i);
    }
    i = 0;
    for (auto& t : threads) {
        t.join();
        i++;
    }
    REQUIRE(i == thread_num);
}