#include <catch2/catch_all.hpp>
#include "base/periodic_function.h"

#include <ctime>
#include <iostream>
#include <mutex>
#include <condition_variable>

class tester {
public:
    tester() = default;
    
    void produce_timestamp() {
        std::string send = std::to_string(std::time(nullptr));
        {
            std::lock_guard<std::mutex> lock(mutex_);
            oss_.write(send.c_str(), send.size());
            cv_.notify_one();
        }
        std::cout << "send timestamp: " << send << std::endl;
    }

    std::string consume_timestamp() {
        std::string recv;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !this->oss_.str().empty(); });
            if (!this->oss_.str().empty()) {
                recv = oss_.str();
                oss_.str("");
            } 
        }
        std::cout << "recv timestamp: " << recv << std::endl;
        return recv;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::ostringstream oss_;
};


TEST_CASE("test_periodic_function", "[periodic_function]") {
    std::time_t start_time = std::time(nullptr);
    tester t;
    size_t interval_sec = 1;
    {
        mylog::base::periodic_function p_func(std::bind(&tester::produce_timestamp, &t),
                                      std::chrono::seconds(interval_sec));
        size_t last_ts = 0;
        while (std::time(nullptr) - start_time < 5) {
            size_t ts = static_cast<size_t>(std::stoi(t.consume_timestamp()));
            if (last_ts > 0) {
                REQUIRE(ts - last_ts == interval_sec);
            }
            last_ts = ts;
        }
    }
}