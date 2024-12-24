#pragma once

#include "base/mpmc_blocking_queue.h"
#include "base/async_msg.h"

#include <thread>
#include <vector>
#include <functional>

namespace mylog {

namespace base {

static const size_t default_queue_size = 8192;
static const size_t default_threads_num = 1;

class thread_pool {
public:
    using item_type = async_msg;
    
    thread_pool(size_t queue_size, size_t threads_num, 
                const std::function<void()>& on_thread_start,
                const std::function<void()>& on_thread_stop);
    thread_pool(size_t queue_size, size_t threads_num, 
                const std::function<void()>& on_thread_start);
    thread_pool(size_t queue_size = default_queue_size, 
                size_t threads_num = default_threads_num);
    ~thread_pool();

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    void enqueue_log(async_logger_shr_ptr&& logger,
                     const log_msg& msg,
                     async_overflow_method overflow_m);

    std::future<void> enqueue_flush(async_logger_shr_ptr&& logger, 
                                    async_overflow_method overflow_m);

    size_t override_count() { return msg_queue_.get_override_count(); }
    void reset_override_count() { msg_queue_.reset_override_count(); }

    size_t discard_count() { return msg_queue_.get_discard_count(); }
    void reset_discard_count() { msg_queue_.reset_discard_count(); }

    size_t message_queue_size() { return msg_queue_.size(); }

private:  
    void enqueue_async_msg_(async_msg&& amsg, async_overflow_method overflow_m);
    void worker_loop_();
    bool process_next_msg_();
    
    std::function<void()> start_func_;
    std::function<void()> stop_func_;
    std::vector<std::thread> threads_;
    mpmc_blocking_queue<item_type> msg_queue_;
};

}   // namespace base
}   // namespace mylog