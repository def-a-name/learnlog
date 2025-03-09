#pragma once

#include "base/thread_pool.h"
#include "concurrentqueue/blockingconcurrentqueue.h"
#include "async_logger.h"

namespace learnlog {
namespace base {

struct token_info {
    const moodycamel::ProducerToken p_token;
    std::atomic<bool> using_flag;

    explicit token_info(moodycamel::ConcurrentQueue<async_msg>& msg_q) : 
        p_token(msg_q), 
        using_flag(false) {}
};

#ifdef LEARNLOG_USE_TLS
    static thread_local token_info* t_info_ = nullptr;
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
            token_infos_.emplace_back(new token_info(msg_q_));
        }
        for (size_t i = 0; i < threads_num_; ++i) {
            threads_.emplace_back([this] {
                start_func_();
                producer_sema_.wait();
                
                size_t idx = consumer_cnt_.fetch_add(1, std::memory_order_relaxed);
                token_info* t_info = nullptr;
                {
                    std::lock_guard<std::mutex> lock(ti_mutex_);
                    t_info = token_infos_[idx];
                }
#ifdef LEARNLOG_USE_TLS
                t_info_ = t_info;
#else
                {
                    std::lock_guard<std::mutex> lock(cti_mutex_);
                    consumer_token_infos_[os::thread_id()] = t_info;
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
            token_infos_.emplace_back(new token_info(msg_q_));
        }
        for (size_t i = 0; i < threads_num_; ++i) {
            threads_.emplace_back([this] {
                start_func_();
                producer_sema_.wait();
                
                size_t idx = consumer_cnt_.fetch_add(1, std::memory_order_relaxed);
                token_info* t_info = nullptr;
                {
                    std::lock_guard<std::mutex> lock(ti_mutex_);
                    t_info = token_infos_[idx];
                }
#ifdef LEARNLOG_USE_TLS
                t_info_ = t_info;
#else
                {
                    std::lock_guard<std::mutex> lock(cti_mutex_);
                    consumer_token_infos_[os::thread_id()] = t_info;
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
            token_infos_.emplace_back(new token_info(msg_q_));
        }
        for (size_t i = 0; i < threads_num_; ++i) {
            threads_.emplace_back([this] {
                start_func_();
                producer_sema_.wait();
                
                size_t idx = consumer_cnt_.fetch_add(1, std::memory_order_relaxed);
                token_info* t_info = nullptr;
                {
                    std::lock_guard<std::mutex> lock(ti_mutex_);
                    t_info = token_infos_[idx];
                }
#ifdef LEARNLOG_USE_TLS
                t_info_ = t_info;
#else
                {
                    std::lock_guard<std::mutex> lock(cti_mutex_);
                    consumer_token_infos_[os::thread_id()] = t_info;
                }
#endif
                this->thread_pool::worker_loop_();
                stop_func_();
            });
        }
    }

    ~lockfree_concurrent_thread_pool() override {
        for (auto &t_info : token_infos_) {
            producer_sema_.signal();
            msg_q_.enqueue(t_info->p_token, async_msg(async_msg_type::terminate));
        }
        try {
            for(auto &t : threads_){
                t.join();
            }
            for (auto &t_info : token_infos_) {
                delete t_info;
                t_info = nullptr;
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
        token_info* t_info = nullptr;
#ifdef LEARNLOG_USE_TLS
        if (t_info_ == nullptr) {
            size_t cnt = producer_cnt_.fetch_add(1, std::memory_order_relaxed);
            {
                std::lock_guard<std::mutex> lock(ti_mutex_);
                t_info_ = token_infos_[cnt % threads_num_];
            }
            producer_sema_.signal();
        }
        t_info = t_info_;
#else
        size_t tid = os::thread_id();
        {
            std::lock_guard<std::mutex> lock(pti_mutex_);
            if (producer_token_infos_.find(tid) == producer_token_infos_.end()) {
                size_t cnt = producer_cnt_.fetch_add(1, std::memory_order_relaxed);
                {
                    std::lock_guard<std::mutex> lock(ti_mutex_);
                    t_info = token_infos_[cnt % threads_num_];
                }
                producer_token_infos_[tid] = t_info;
                producer_sema_.signal();
            }
            else {
                t_info = producer_token_infos_[tid];
            }
        }
#endif
        if (producer_cnt_.load(std::memory_order_relaxed) > threads_num_) {
            bool expected = false;
            while (!t_info->using_flag.compare_exchange_weak(expected, true,
                                                             std::memory_order_relaxed)) {
                expected = false;
            }
            while (!msg_q_.try_enqueue(t_info->p_token, std::move(amsg))) {
                if (msg_q_.enqueue(t_info->p_token, std::move(amsg))) {
                    break;
                }
            }
            t_info->using_flag.store(false, std::memory_order_relaxed);
        }
        else {
            while (!msg_q_.try_enqueue(t_info->p_token, std::move(amsg))) {
                if (msg_q_.enqueue(t_info->p_token, std::move(amsg))) {
                    break;
                }
            }
        }
    }

    void dequeue_async_msg_(async_msg& amsg) override {
        token_info* t_info = nullptr;
#ifdef LEARNLOG_USE_TLS
        t_info = t_info_;
#else
        {
            std::lock_guard<std::mutex> lock(cti_mutex_);
            t_info = consumer_token_infos_[os::thread_id()];
        }
#endif
        while (!msg_q_.try_dequeue_from_producer(t_info->p_token, amsg)) {
            continue;
        }
    }

    std::atomic<size_t> producer_cnt_{0};
    std::atomic<size_t> consumer_cnt_{0};
    moodycamel::LightweightSemaphore producer_sema_;

    std::mutex ti_mutex_;
    std::vector<token_info*> token_infos_;
#ifndef LEARNLOG_USE_TLS
    std::mutex pti_mutex_;
    std::unordered_map<size_t, token_info*> producer_token_infos_;
    std::mutex cti_mutex_;
    std::unordered_map<size_t, token_info*> consumer_token_infos_;
#endif

    moodycamel::ConcurrentQueue<async_msg> msg_q_;
};

}   // namespace base
}   // namespace learnlog