#pragma once

#include "base/thread_pool.h"
#include "block_queue.h"
#include "async_logger.h"

namespace learnlog {
namespace base {

class lock_thread_pool final: public thread_pool {
public:
    lock_thread_pool(size_t queue_size, size_t threads_num, 
                           const std::function<void()>& on_thread_start,
                           const std::function<void()>& on_thread_stop)
        : thread_pool(queue_size, threads_num, on_thread_start, on_thread_stop),
          msg_q_(queue_size) {}
    
    lock_thread_pool(size_t queue_size, size_t threads_num, 
                           const std::function<void()>& on_thread_start)
        : thread_pool(queue_size, threads_num, on_thread_start, []{}),
          msg_q_(queue_size) {}

    lock_thread_pool(size_t queue_size = default_queue_size,
                     size_t threads_num = default_threads_num)
        : thread_pool(queue_size, threads_num, []{}, []{}),
          msg_q_(queue_size) {}

    ~lock_thread_pool() override {
        for (size_t i = 0; i < threads_.size(); i++) {
            enqueue_async_msg_(async_msg(async_msg_type::terminate));
        }
        try {
            for(auto &t : threads_){
                t.join();
            }
        }
        catch(const std::exception& e) {
            source_loc loc{__FILE__, __LINE__, __func__};
            throw_learnlog_excpt(e.what(), os::get_errno(), loc);
        }
    }

    size_t const override_count() { return msg_q_.get_override_count(); }
    void reset_override_count() { msg_q_.reset_override_count(); }
    size_t const discard_count() { return msg_q_.get_discard_count(); }
    void reset_discard_count() { msg_q_.reset_discard_count(); }

private:
    void enqueue_async_msg_(async_msg&& amsg) override {
        msg_q_.enqueue(std::move(amsg));
    }

    void dequeue_async_msg_(async_msg& amsg) override {
        msg_q_.dequeue(amsg);
    }

    block_queue<async_msg> msg_q_;
};

}   // namespace base
}   // namespace learnlog