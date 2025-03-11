#pragma once

#include "base/thread_pool.h"
#include "concurrentqueue/blockingconcurrentqueue.h"
#include "async_logger.h"

namespace learnlog {
namespace base {

struct atomic_token {
    const moodycamel::ProducerToken p_token_;
    std::atomic<bool> enqueuing_;

    explicit atomic_token(moodycamel::ConcurrentQueue<async_msg>& msg_q)
        : p_token_(msg_q),
          enqueuing_(false) {}

    void enqueue_lock() {
        bool expected = false;
        while (!enqueuing_.compare_exchange_weak(expected, true,
                                             std::memory_order_relaxed)) {
            expected = false;
        }
    }

    void release() {
        enqueuing_.store(false, std::memory_order_relaxed);
    }
};

#ifdef LEARNLOG_USE_TLS
    static thread_local atomic_token* token_ = nullptr;
#endif

// 使用无锁队列 ConcurrentQueue 的线程池，处理 async_msg，
// 入队时先尝试 q.try_enqueue()，如果失败再尝试 q.enqueue()，以此循环
// 出队时循环尝试 q.try_dequeue_from_producer()，
// 如果在队列占用空间不变的条件下入队失败(try_enqueue)，会额外申请一块内存后再次尝试(enqueue)，
// 队列的总占用空间只增不减

class lockfree_concurrent_thread_pool final: public thread_pool {
public:
    lockfree_concurrent_thread_pool(size_t queue_size, size_t threads_num, 
                                    const std::function<void()>& on_thread_start,
                                    const std::function<void()>& on_thread_stop)
        : thread_pool(queue_size, 
                      lockfree_concurrent,
                      threads_num,
                      on_thread_start,
                      on_thread_stop),
          msg_q_(msg_q_size_) {
        for (size_t i = 0; i < threads_num_; ++i) {
            atomic_tokens_.emplace_back(new atomic_token(msg_q_));
        }
        for (size_t i = 0; i < threads_num_; ++i) {
            threads_.emplace_back([this] {
                start_func_();
                producer_sema_.wait();
                
                size_t idx = consumer_cnt_.fetch_add(1, std::memory_order_relaxed);
                atomic_token* token = nullptr;
                {
                    std::lock_guard<std::mutex> lock(t_mutex_);
                    token = atomic_tokens_[idx];
                }
#ifdef LEARNLOG_USE_TLS
                token_ = token;
#else
                {
                    std::lock_guard<std::mutex> lock(c_mutex_);
                    consumer_atomic_tokens_[os::thread_id()] = token;
                }
#endif
                this->thread_pool::worker_loop_();
                stop_func_();
            });
        }
    }
    
    lockfree_concurrent_thread_pool(size_t queue_size, size_t threads_num, 
                                    const std::function<void()>& on_thread_start)
        : thread_pool(queue_size,
                      lockfree_concurrent,
                      threads_num,
                      on_thread_start,
                      []{}),
          msg_q_(msg_q_size_) {
        for (size_t i = 0; i < threads_num_; ++i) {
            atomic_tokens_.emplace_back(new atomic_token(msg_q_));
        }
        for (size_t i = 0; i < threads_num_; ++i) {
            threads_.emplace_back([this] {
                start_func_();
                producer_sema_.wait();
                
                size_t idx = consumer_cnt_.fetch_add(1, std::memory_order_relaxed);
                atomic_token* token = nullptr;
                {
                    std::lock_guard<std::mutex> lock(t_mutex_);
                    token = atomic_tokens_[idx];
                }
#ifdef LEARNLOG_USE_TLS
                token_ = token;
#else
                {
                    std::lock_guard<std::mutex> lock(c_mutex_);
                    consumer_atomic_tokens_[os::thread_id()] = token;
                }
#endif
                this->thread_pool::worker_loop_();
                stop_func_();
            });
        }
    }

    lockfree_concurrent_thread_pool(size_t queue_size = default_queue_size,
                                    size_t threads_num = default_threads_num)
        : thread_pool(queue_size,
                      lockfree_concurrent,
                      threads_num,
                      []{},
                      []{}),
          msg_q_(msg_q_size_) {
        for (size_t i = 0; i < threads_num_; ++i) {
            atomic_tokens_.emplace_back(new atomic_token(msg_q_));
        }
        for (size_t i = 0; i < threads_num_; ++i) {
            threads_.emplace_back([this] {
                start_func_();
                producer_sema_.wait();
                
                size_t idx = consumer_cnt_.fetch_add(1, std::memory_order_relaxed);
                atomic_token* token = nullptr;
                {
                    std::lock_guard<std::mutex> lock(t_mutex_);
                    token = atomic_tokens_[idx];
                }
#ifdef LEARNLOG_USE_TLS
                token_ = token;
#else
                {
                    std::lock_guard<std::mutex> lock(c_mutex_);
                    consumer_atomic_tokens_[os::thread_id()] = token;
                }
#endif
                this->thread_pool::worker_loop_();
                stop_func_();
            });
        }
    }

    ~lockfree_concurrent_thread_pool() override {
        for (auto &token : atomic_tokens_) {
            producer_sema_.signal();
            msg_q_.enqueue(token->p_token_,
                           async_msg(async_msg_type::terminate));
        }
        try {
            for(auto &t : threads_){
                t.join();
            }
            for (auto &token : atomic_tokens_) {
                delete token;
                token = nullptr;
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
        atomic_token* token = nullptr;
#ifdef LEARNLOG_USE_TLS
        if (token_ == nullptr) {
            size_t cnt = producer_cnt_.fetch_add(1, std::memory_order_relaxed);
            {
                std::lock_guard<std::mutex> lock(t_mutex_);
                token_ = atomic_tokens_[cnt % threads_num_];
            }
            producer_sema_.signal();
        }
        token = token_;
#else
        size_t tid = os::thread_id();
        {
            std::lock_guard<std::mutex> lock(p_mutex_);
            if (producer_atomic_tokens_.find(tid) == producer_atomic_tokens_.end()) {
                size_t cnt = producer_cnt_.fetch_add(1, std::memory_order_relaxed);
                {
                    std::lock_guard<std::mutex> lock(t_mutex_);
                    token = atomic_tokens_[cnt % threads_num_];
                }
                producer_atomic_tokens_[tid] = token;
                producer_sema_.signal();
            }
            else {
                token = producer_atomic_tokens_[tid];
            }
        }
#endif
        token->enqueue_lock();
        while (!msg_q_.try_enqueue(token->p_token_, std::move(amsg))) {
            if (msg_q_.enqueue(token->p_token_, std::move(amsg))) {
                break;
            }
        }
        token->release();
    }

    void dequeue_async_msg_(async_msg& amsg) override {
        atomic_token* token = nullptr;
#ifdef LEARNLOG_USE_TLS
        token = token_;
#else
        {
            std::lock_guard<std::mutex> lock(c_mutex_);
            token = consumer_atomic_tokens_[os::thread_id()];
        }
#endif
        while (!msg_q_.try_dequeue_from_producer(token->p_token_, amsg)) {
            continue;
        }
    }

    std::atomic<size_t> producer_cnt_{0};
    std::atomic<size_t> consumer_cnt_{0};
    moodycamel::LightweightSemaphore producer_sema_;

    std::mutex t_mutex_;
    std::vector<atomic_token*> atomic_tokens_;
#ifndef LEARNLOG_USE_TLS
    std::mutex p_mutex_;
    std::unordered_map<size_t, atomic_token*> producer_atomic_tokens_;
    std::mutex c_mutex_;
    std::unordered_map<size_t, atomic_token*> consumer_atomic_tokens_;
#endif

    moodycamel::ConcurrentQueue<async_msg> msg_q_;
};

}   // namespace base
}   // namespace learnlog