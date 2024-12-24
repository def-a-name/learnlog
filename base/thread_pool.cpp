#include "base/thread_pool.h"
#include "base/exception.h"
#include "async_logger.h"

using namespace mylog;
using namespace base;

thread_pool::thread_pool(size_t queue_size, size_t threads_num, 
                         const std::function<void()>& on_thread_start,
                         const std::function<void()>& on_thread_stop) :
    msg_queue_(queue_size),
    start_func_(on_thread_start),
    stop_func_(on_thread_stop)
{
    if(threads_num <= 0 || threads_num > 1000) {
        throw_mylog_excpt(
            "mylog::thread_pool(): invalid threads_num param "
            "(valid range is 1-1000)");
    }
    
    for (size_t i = 0; i < threads_num; ++i) {
        threads_.emplace_back([this, on_thread_start, on_thread_stop] {
            start_func_();
            this->thread_pool::worker_loop_();
            stop_func_();
        });
    }
}

thread_pool::thread_pool(size_t queue_size, size_t threads_num, 
                         const std::function<void()>& on_thread_start) :
    thread_pool(queue_size, threads_num, on_thread_start, [] {}) {}

thread_pool::thread_pool(size_t queue_size, size_t threads_num) :
    thread_pool(queue_size, threads_num, [] {}, [] {}) {}

thread_pool::~thread_pool() {
    try {
        for (int i = 0; i < threads_.size(); i++) {
            enqueue_async_msg_(async_msg(async_msg_type::terminate), 
                               async_overflow_method::block_wait);
        }
        for(auto &t : threads_){
            t.join();
        }
    }
    catch(const std::exception& e) {
        source_loc loc{__FILE__, __LINE__, __func__};
        throw_mylog_excpt(e.what(), os::get_errno(), loc);
    }
}

void thread_pool::enqueue_log(async_logger_shr_ptr&& logger,
                              const log_msg& msg,
                              async_overflow_method overflow_m) {
    async_msg amsg(std::move(logger), async_msg_type::log, msg);
    enqueue_async_msg_(std::move(amsg), overflow_m);
}

std::future<void> thread_pool::enqueue_flush(async_logger_shr_ptr&& logger, 
                                             async_overflow_method overflow_m) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    async_msg amsg(std::move(logger), async_msg_type::flush, std::move(promise));
    enqueue_async_msg_(std::move(amsg), overflow_m);
    
    return future;
}

void thread_pool::enqueue_async_msg_(async_msg&& amsg, async_overflow_method overflow_m) {
    switch (overflow_m) {
        case async_overflow_method::discard_new: {
            msg_queue_.enqueue_if_have_room(std::move(amsg));
            break;
        }
        case async_overflow_method::block_wait: {
            msg_queue_.enqueue(std::move(amsg));
            break;
        }
        case async_overflow_method::override_old: {
            msg_queue_.enqueue_nowait(std::move(amsg));
            break;
        }
        default: {
            assert(false);
        }
    }
}

void thread_pool::worker_loop_() {
    while( process_next_msg_() ) {}
}

bool thread_pool::process_next_msg_() {
    async_msg msg_popped;
    msg_queue_.dequeue(msg_popped);

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