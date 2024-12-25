#pragma once

#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace learnlog {
namespace base {

// 在生命周期内，新增一个线程，该线程每隔一段时间 interval 执行函数 func，
// 在构造时建立线程，在析构时跳出循环并阻塞线程（func 最后一次执行完毕后返回）
class periodic_function {
public:
    template <typename Rep, typename Period>
    periodic_function(const std::function<void()>& func, 
                      std::chrono::duration<Rep, Period> interval) {
        active_ = (interval > std::chrono::duration<Rep, Period>::zero());
        if (!active_) return;

        thread_ = std::thread([this, func, interval] {
            for (;;) {
                std::unique_lock<std::mutex> lock(mutex_);
                if (cv_.wait_for(lock, interval, [this] {return !this->active_;} )) {
                    return;
                }
                func();
            }
        });
    }

    periodic_function(const periodic_function&) = delete;
    periodic_function& operator=(const periodic_function&) = delete;

    ~periodic_function() {
        if (thread_.joinable()) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                active_ = false;
            }
            cv_.notify_one();
            thread_.join();
        }
    }

private:
    bool active_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

}   // namespace base
}   // namespace learnlog