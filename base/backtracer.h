#pragma once

#include "base/log_msg_buf.h"
#include "base/circular_queue.h"

#include <atomic>
#include <mutex>
#include <functional>

namespace learnlog {
namespace base {

// 日志备份类,
// 将最后输出的 n 条 log_msg 保存到环形缓冲区中，便于出现问题时回溯，
// 拥有 mutable mutex，支持多线程操作

class backtracer{
public:
    backtracer() = default;
    backtracer(const backtracer& other) {
        std::lock_guard<std::mutex> lock(other.mutex_);
        enabled_ = other.enabled_;
        log_msgs_ = other.log_msgs_;
    }

    backtracer& operator=(const backtracer& other) {
        std::lock_guard<std::mutex> lock(other.mutex_);
        enabled_ = other.enabled_;
        log_msgs_ = other.log_msgs_;
        return *this;
    }

    backtracer(backtracer&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex_);
        enabled_ = std::move(other.enabled_);
        log_msgs_ = std::move(other.log_msgs_);
    }

    bool is_enabled() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return enabled_;
    }

    void enable_with_size(size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        enabled_ = true;
        log_msgs_ = circular_queue<log_msg_buf>(size);
    }

    void disable() {
        std::lock_guard<std::mutex> lock(mutex_);
        enabled_ = false;
        log_msgs_.clear();
    }

    void push_back(const log_msg& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        log_msgs_.push_back(log_msg_buf{msg});
    }

    void foreach_do(std::function<void(const log_msg&)> func) {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!log_msgs_.empty()) {
            log_msg_buf& msg_buf = log_msgs_.front();
            log_msgs_.pop_front();
            func(msg_buf);
        }
    }

private:
    mutable std::mutex mutex_;
    bool enabled_ = false;
    circular_queue<log_msg_buf> log_msgs_;
};

}   // namespace base
}   // namespace learnlog