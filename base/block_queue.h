#pragma once

#include "base/circular_queue.h"

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace learnlog {
namespace base {

/*
多生产者-多消费者阻塞队列，
提供 3 种入队方式：
    1. enqueue_if_have_room: 队列未满时直接入队，队列已满时丢弃；
    2. enqueue: 队列未满时直接入队，队列已满时阻塞，等待有元素出队后再入队；
    3. enqueue_nowait: 无论队列是否已满都入队，从环形队列头部覆盖；
提供 2 种出队方式：
    1. dequeue: 队列不空时出队，队列为空时阻塞，等待有元素入队后再出队；
    2. dequeue_for: 在 dequeue 的基础上，设置超时时间，超时后出队失败，返回 false；
*/

template <typename T>
class block_queue
{
public:  
    using item_type = T;
    explicit block_queue(size_t max_items) 
    : queue_(max_items) {}

#ifndef __MINGW32__
    void enqueue_if_have_room(T &&item) {
        bool pushed = false;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (!queue_.full()) {
                queue_.push_back(std::move(item));
                pushed = true;
            }
        }

        if (pushed) {
            push_cv_.notify_one();
        } else {
            ++discard_count_;
        }
    }
    
    void enqueue(T &&item) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            pop_cv_.wait(lock, [this] { return !this->queue_.full(); });
            queue_.push_back(std::move(item));
        }
        push_cv_.notify_one();
    }

    void enqueue_nowait(T &&item) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_.push_back(std::move(item));
        }
        push_cv_.notify_one();
    }

    void dequeue(T& popped_item) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            push_cv_.wait(lock, [this] { return !this->queue_.empty(); });
            popped_item = std::move(queue_.front());
            queue_.pop_front();
        }
        pop_cv_.notify_one();
    }

    bool dequeue_for(T& popped_item, std::chrono::milliseconds duration) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (!push_cv_.wait_for(lock, duration, [this] { return !this->queue_.empty(); })) {
                return false;
            }
            popped_item = std::move(queue_.front());
            queue_.pop_front();
        }
        pop_cv_.notify_one();
        return true;
    }

#else
    // 在 mingw 编译下，最后再释放 unique_lock，否则会导致死锁

    void enqueue_if_have_room(T &&item) {
        bool pushed = false;
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (!queue_.full()) {
            queue_.push_back(std::move(item));
            pushed = true;
        }

        if (pushed) {
            push_cv_.notify_one();
        } else {
            ++discard_count_;
        }
    }
    
    void enqueue(T &&item) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        pop_cv_.wait(lock, [this] { return !this->queue_.full(); });
        queue_.push_back(std::move(item));
        
        push_cv_.notify_one();
    }

    void enqueue_nowait(T &&item) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_.push_back(std::move(item));
        
        push_cv_.notify_one();
    }

    void dequeue(T& popped_item) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        push_cv_.wait(lock, [this] { return !this->queue_.empty(); });
        popped_item = std::move(queue_.front());
        queue_.pop_front();

        pop_cv_.notify_one();
    }

    bool dequeue_for(T& popped_item, std::chrono::milliseconds duration) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (!push_cv_.wait_for(lock, duration, [this] { return !this->queue_.empty(); })) {
            return false;
        }
        popped_item = std::move(queue_.front());
        queue_.pop_front();

        pop_cv_.notify_one();
        return true;
    }

#endif

    // 队列保存的元素个数
    size_t size() {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return queue_.size();
    }

    // 获取队列被覆盖的元素个数
    size_t get_override_count() {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return queue_.override_count();
    }

    // 重置队列被覆盖的元素个数
    void reset_override_count() {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.reset_override_count();
    }

    // 获取 enqueue_if_have_room() 中因队满而丢弃的元素个数
    size_t get_discard_count() const {
        return discard_count_.load(std::memory_order_relaxed);
    }

    // 重置 enqueue_if_have_room() 中因队满而丢弃的元素个数
    void reset_discard_count() { discard_count_.store(0, std::memory_order_relaxed); }

private:
    std::mutex queue_mutex_;
    std::condition_variable push_cv_;
    std::condition_variable pop_cv_;
    std::atomic<size_t> discard_count_{0};

    learnlog::base::circular_queue<T> queue_;
};

}   // namespace base
}   // namespace learnlog