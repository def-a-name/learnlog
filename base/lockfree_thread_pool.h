#pragma once

#include "base/thread_pool.h"
#include "concurrentqueue/blockingconcurrentqueue.h"
#include "async_logger.h"

namespace learnlog {
namespace base {

using c_token_uni_ptr = std::unique_ptr<moodycamel::ConsumerToken>;

#ifdef LEARNLOG_USE_TLS
    static thread_local c_token_uni_ptr c_token_ = nullptr;
#endif

// 使用无锁阻塞队列 BlockingConcurrentQueue 的线程池，处理 async_msg，
// 循环尝试 q.try_enqueue() 入队，阻塞等待 q.wait_dequeue() 出队，
// 队列的总占用空间不变，初始化后不再额外申请内存

class lockfree_thread_pool final: public thread_pool {
public:
    lockfree_thread_pool(size_t queue_size, size_t threads_num, 
                         const std::function<void()>& on_thread_start,
                         const std::function<void()>& on_thread_stop)
        : thread_pool(queue_size, lockfree, threads_num, on_thread_start, on_thread_stop),
          msg_q_(msg_q_size_) {
        for (size_t i = 0; i < threads_num_; ++i) {
            threads_.emplace_back([this] {
                start_func_();
#ifdef LEARNLOG_USE_TLS
                c_token_ = learnlog::make_unique<moodycamel::ConsumerToken>(msg_q_);
#else
                size_t tid = os::thread_id();
                {
                    std::lock_guard<std::mutex> lock(hash_mutex_);
                    tokens_[tid] = learnlog::make_unique<moodycamel::ConsumerToken>(msg_q_);
                }
#endif
                this->thread_pool::worker_loop_();
                stop_func_();
            });
        }
    }
    
    lockfree_thread_pool(size_t queue_size, size_t threads_num, 
                           const std::function<void()>& on_thread_start)
        : thread_pool(queue_size, lockfree, threads_num, on_thread_start, []{}),
          msg_q_(msg_q_size_) {
        for (size_t i = 0; i < threads_num_; ++i) {
            threads_.emplace_back([this] {
                start_func_();
#ifdef LEARNLOG_USE_TLS
                c_token_ = learnlog::make_unique<moodycamel::ConsumerToken>(msg_q_);
#else
                size_t tid = os::thread_id();
                {
                    std::lock_guard<std::mutex> lock(hash_mutex_);
                    tokens_[tid] = learnlog::make_unique<moodycamel::ConsumerToken>(msg_q_);
                }
#endif
                this->thread_pool::worker_loop_();
                stop_func_();
            });
        }
    }

    lockfree_thread_pool(size_t queue_size = default_queue_size,
                     size_t threads_num = default_threads_num)
        : thread_pool(queue_size, lockfree, threads_num, []{}, []{}),
          msg_q_(msg_q_size_) {
        for (size_t i = 0; i < threads_num_; ++i) {
            threads_.emplace_back([this] {
                start_func_();
#ifdef LEARNLOG_USE_TLS
                c_token_ = learnlog::make_unique<moodycamel::ConsumerToken>(msg_q_);
#else
                size_t tid = os::thread_id();
                {
                    std::lock_guard<std::mutex> lock(hash_mutex_);
                    tokens_[tid] = learnlog::make_unique<moodycamel::ConsumerToken>(msg_q_);
                }
#endif
                this->thread_pool::worker_loop_();
                stop_func_();
            });
        }
    }

    ~lockfree_thread_pool() override {
        while (current_msg_count() != 0) {
            // os::sleep_for_ms(1);
        }
        for (size_t i = 0; i < threads_num_; i++) {
            enqueue_async_msg_(async_msg(async_msg_type::terminate));
        }
        try {
            for (auto &t : threads_){
                t.join();
            }
        }
        catch(const std::exception& e) {
            source_loc loc{__FILE__, __LINE__, __func__};
            throw_learnlog_excpt(e.what(), os::get_errno(), loc);
        }
    }

    size_t current_msg_count() { return msg_q_.size_approx(); }

private:
    void enqueue_async_msg_(async_msg&& amsg) override {
        while (!msg_q_.try_enqueue(std::move(amsg)))
            continue;
    }

    void dequeue_async_msg_(async_msg& amsg) override {
#ifdef LEARNLOG_USE_TLS
        msg_q_.wait_dequeue(*c_token_, amsg);
#else
        size_t tid = os::thread_id();
        moodycamel::ConsumerToken* token = nullptr;
        {
            std::lock_guard<std::mutex> lock(hash_mutex_);
            token = tokens_[tid].get();
        }
        msg_q_.wait_dequeue(*token, amsg);
#endif
    }

#ifndef LEARNLOG_USE_TLS
    std::mutex hash_mutex_;
    std::unordered_map<size_t, c_token_uni_ptr> tokens_;
#endif

    moodycamel::BlockingConcurrentQueue<async_msg> msg_q_;
};

}   // namespace base
}   // namespace learnlog