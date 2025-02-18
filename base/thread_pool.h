#pragma once

#include "base/async_msg.h"

#include <thread>
#include <vector>
#include <functional>

namespace learnlog {
namespace base {

static const size_t default_queue_size = 32768;
static const size_t default_threads_num = 1;

class thread_pool {
public:
    thread_pool(size_t queue_size, size_t threads_num, 
                const std::function<void()>& on_thread_start,
                const std::function<void()>& on_thread_stop);
    virtual ~thread_pool() = default;
    
    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    void enqueue_log(async_logger_shr_ptr&& logger, const log_msg& msg);
    std::future<void> enqueue_flush(async_logger_shr_ptr&& logger);

    size_t const message_queue_size() { return msg_q_size_; }

protected:
    void worker_loop_();
    bool process_async_msg_();
    virtual void enqueue_async_msg_(async_msg&& amsg) = 0;
    virtual void dequeue_async_msg_(async_msg& amsg) = 0;

    size_t msg_q_size_;
    std::function<void()> start_func_;
    std::function<void()> stop_func_;
    std::vector<std::thread> threads_;
};

}   // namespace base
}   // namespace learnlog