#include "base/thread_pool.h"
#include "base/exception.h"
#include "async_logger.h"

using namespace learnlog;
using namespace base;

thread_pool::thread_pool(size_t queue_size, size_t threads_num, 
                         const std::function<void()>& on_thread_start,
                         const std::function<void()>& on_thread_stop) :
    msg_q_size_(queue_size),
    start_func_(on_thread_start),
    stop_func_(on_thread_stop)
{
    if(threads_num <= 0 || threads_num > 1024) {
        std::string err_str = 
            fmt::format("learnlog::thread_pool(): invalid threads_num {:d} "
                        "(valid range is 1-1024)", threads_num);
        throw_learnlog_excpt(err_str);
    }
    
    for (size_t i = 0; i < threads_num; ++i) {
        threads_.emplace_back([this] {
            start_func_();
            this->thread_pool::worker_loop_();
            stop_func_();
        });
    }
}

void thread_pool::enqueue_log(async_logger_shr_ptr&& logger,
                              const log_msg& msg) {
    async_msg amsg(std::move(logger), async_msg_type::log, msg);
    enqueue_async_msg_(std::move(amsg));
}

std::future<void> thread_pool::enqueue_flush(async_logger_shr_ptr&& logger) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    async_msg amsg(std::move(logger), async_msg_type::flush, std::move(promise));
    enqueue_async_msg_(std::move(amsg));
    
    return future;
}

bool thread_pool::process_async_msg_() {
    async_msg msg_popped;
    dequeue_async_msg_(msg_popped);

    switch (msg_popped.msg_type) {
        case async_msg_type::log: {
            msg_popped.logger->do_sink_log_(msg_popped);
            return true;
        }
        case async_msg_type::flush: {
            msg_popped.logger->do_flush_sink_();
            msg_popped.flush_promise.set_value();
            return true;
        }
        case async_msg_type::terminate: {
            return false;
        }
        default: {
            assert(false);
        }
    }

    return true;
}

void thread_pool::worker_loop_() {
    while( process_async_msg_() ) {}
}