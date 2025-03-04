#pragma once

#include "base/thread_pool.h"
#include "concurrentqueue/blockingconcurrentqueue.h"
#include "async_logger.h"

namespace learnlog {
namespace base {

#ifdef LEARNLOG_USE_TLS
    static thread_local size_t p_token_idx_ = SIZE_MAX;
#endif

class lockfree_concurrent_thread_pool final: public thread_pool {
public:
    using p_token_uni_ptr = std::unique_ptr<moodycamel::ProducerToken>;

    lockfree_concurrent_thread_pool(size_t queue_size, size_t threads_num, 
                                    const std::function<void()>& on_thread_start,
                                    const std::function<void()>& on_thread_stop)
        : thread_pool(queue_size, 
                      lockfree_concurrent,
                      threads_num,
                      on_thread_start,
                      on_thread_stop),
          tokens_using_(threads_num_),
          msg_q_(msg_q_size_) {
        for (size_t i = 0; i < threads_num_; ++i) {
            tokens_.emplace_back(learnlog::make_unique<moodycamel::ProducerToken>(msg_q_));
        }
        for (size_t i = 0; i < threads_num_; ++i) {
            threads_.emplace_back([this] {
                start_func_();
                tokens_sema_.wait();
                size_t idx = consumer_cnt_.fetch_add(1, std::memory_order_relaxed);
#ifdef LEARNLOG_USE_TLS
                p_token_idx_ = idx;
#else
                {
                    std::lock_guard<std::mutex> lock(consumer_mutex_);
                    consumer_p_token_idx_[os::thread_id()] = idx;
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
          tokens_using_(threads_num_),
          msg_q_(msg_q_size_) {
        for (size_t i = 0; i < threads_num_; ++i) {
            tokens_.emplace_back(learnlog::make_unique<moodycamel::ProducerToken>(msg_q_));
        }
        for (size_t i = 0; i < threads_num_; ++i) {
            threads_.emplace_back([this] {
                start_func_();
                tokens_sema_.wait();
                size_t idx = consumer_cnt_.fetch_add(1, std::memory_order_relaxed);
#ifdef LEARNLOG_USE_TLS
                p_token_idx_ = idx;
#else
                {
                    std::lock_guard<std::mutex> lock(consumer_mutex_);
                    consumer_p_token_idx_[os::thread_id()] = idx;
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
          tokens_using_(threads_num_),
          msg_q_(msg_q_size_) {
        for (size_t i = 0; i < threads_num_; ++i) {
            tokens_.emplace_back(learnlog::make_unique<moodycamel::ProducerToken>(msg_q_));
        }
        for (size_t i = 0; i < threads_num_; ++i) {
            threads_.emplace_back([this] {
                start_func_();
                tokens_sema_.wait();
                size_t idx = consumer_cnt_.fetch_add(1, std::memory_order_relaxed);
#ifdef LEARNLOG_USE_TLS
                p_token_idx_ = idx;
#else
                {
                    std::lock_guard<std::mutex> lock(consumer_mutex_);
                    consumer_p_token_idx_[os::thread_id()] = idx;
                }
#endif
                this->thread_pool::worker_loop_();
                stop_func_();
            });
        }
    }

    ~lockfree_concurrent_thread_pool() override {
        for (auto &token : tokens_) {
            tokens_sema_.signal();
            msg_q_.enqueue(*token, async_msg(async_msg_type::terminate));
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

    size_t current_msg_count() { return msg_q_.size_approx(); }

private:
    void enqueue_async_msg_(async_msg&& amsg) override {
#ifdef LEARNLOG_USE_TLS
        if (p_token_idx_ == SIZE_MAX) {
            size_t cnt = producer_cnt_.fetch_add(1, std::memory_order_relaxed);
            p_token_idx_ = cnt % threads_num_;
            tokens_sema_.signal();
        }

        auto p_token = tokens_[p_token_idx_].get();
        if (producer_cnt_.load(std::memory_order_relaxed) > threads_num_) {
            bool expected = false;
            while (!tokens_using_[p_token_idx_].compare_exchange_weak(expected, true,
                                                                      std::memory_order_relaxed)) {
                expected = false;
            }
            while (!msg_q_.try_enqueue(*p_token, std::move(amsg))) {
                if (msg_q_.enqueue(*p_token, std::move(amsg))) {
                    break;
                }
            }
            tokens_using_[p_token_idx_].store(false, std::memory_order_relaxed);
        }
        else {
            while (!msg_q_.try_enqueue(*p_token, std::move(amsg))) {
                if (msg_q_.enqueue(*p_token, std::move(amsg))) {
                    break;
                }
            }
        }
#else
        size_t tid = os::thread_id();
        size_t idx = 0;
        {
            std::lock_guard<std::mutex> lock(producer_mutex_);
            if (producer_p_token_idx_.find(tid) == producer_p_token_idx_.end()) {
                size_t cnt = producer_cnt_.fetch_add(1, std::memory_order_relaxed);
                producer_p_token_idx_[tid] = cnt % threads_num_;
                tokens_sema_.signal();
            }
            idx = producer_p_token_idx_[tid];
        }
        
        auto p_token = tokens_[idx].get();
        if (producer_cnt_.load(std::memory_order_relaxed) > threads_num_) {
            bool expected = false;
            while (!tokens_using_[idx].compare_exchange_weak(expected, true,
                                                             std::memory_order_relaxed)) {
                expected = false;
            }
            while (!msg_q_.try_enqueue(*p_token, std::move(amsg))) {
                if (msg_q_.enqueue(*p_token, std::move(amsg))) {
                    break;
                }
            }
            tokens_using_[idx].store(false, std::memory_order_relaxed);
        }
        else {
            while (!msg_q_.try_enqueue(*p_token, std::move(amsg))) {
                if (msg_q_.enqueue(*p_token, std::move(amsg))) {
                    break;
                }
            }
        }
#endif
    }

    void dequeue_async_msg_(async_msg& amsg) override {
#ifdef LEARNLOG_USE_TLS
        while (!msg_q_.try_dequeue_from_producer(*tokens_[p_token_idx_], amsg)) {
            continue;
        }
#else
        size_t idx = 0;
        {
            std::lock_guard<std::mutex> lock(consumer_mutex_);
            idx = consumer_p_token_idx_[os::thread_id()];
        }
        while (!msg_q_.try_dequeue_from_producer(*tokens_[idx], amsg)) {
            continue;
        }
#endif
    }
    
    std::atomic<size_t> producer_cnt_{0};
    std::atomic<size_t> consumer_cnt_{0};
    moodycamel::LightweightSemaphore tokens_sema_;
    std::vector<std::atomic<bool> > tokens_using_;
    std::vector<p_token_uni_ptr> tokens_;
#ifndef LEARNLOG_USE_TLS
    std::mutex producer_mutex_;
    std::unordered_map<size_t, size_t> producer_p_token_idx_;
    std::mutex consumer_mutex_;
    std::unordered_map<size_t, size_t> consumer_p_token_idx_;
#endif

    moodycamel::ConcurrentQueue<async_msg> msg_q_;
};

}   // namespace base
}   // namespace learnlog