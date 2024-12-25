#pragma once

#include <cassert>
#include <vector>

namespace learnlog {
namespace base {

// 通过std::vector实现的环形队列，不保证线程安全

template <typename T>
class circular_queue
{
public:
    using value_type = T;
    using size_t = std::size_t;

    // 默认构造函数，创建一个无效状态的队列
    circular_queue() = default;

    explicit circular_queue(size_t max_items)
        : max_size_(max_items + 1), // 保留一个元素判断是否队满
          vec_(max_size_) {}

    circular_queue(const circular_queue &) = default;
    circular_queue& operator=(const circular_queue &) = default;

    // 移动构造与移动赋值把原队列变为无效状态
    // 即所有成员变量设为0，move(vec_)
    circular_queue(circular_queue &&other) noexcept {
        move_queue(std::move(other));
    }
    circular_queue& operator=(circular_queue &&other) noexcept {
        move_queue(std::move(other));
        return *this;
    }

    size_t size() const { return (tail_ - head_ + max_size_) % max_size_; }

    size_t vec_size() const { return max_size_; }

    bool empty() const { return head_ == tail_; }

    bool full() const {
        if(max_size_ > 0) {
            return ((tail_ + 1) % max_size_) == head_;
        }
        return false;
    }

    const T& at(size_t i) const {
        assert(i < size());
        return vec_[(head_ + i) % max_size_];
    }

    // 插入元素，如果vec_已满，从头部覆盖
    void push_back(T &&item) {
        if(max_size_ > 0) {
            vec_[tail_] = std::move(item);
            tail_ = (tail_ + 1) % max_size_;
            if(tail_ == head_) {
                head_ = (head_ + 1) % max_size_;
                ++override_cnt_;
            }
        }
    }

    void pop_front() {
        if(max_size_ > 0) {
            head_ = (head_ + 1) % max_size_;
        }
    }

    void clear() {
        head_ = 0;
        tail_ = 0;
        override_cnt_ = 0;
        max_size_ = 0;
        vec_.clear();
    }

    T& front() {
        assert(!empty());
        return vec_[head_];
    }

    const T& front() const {
        assert(!empty());
        return vec_[head_];
    }

    size_t override_count() const {return override_cnt_;}
    void reset_override_count() {override_cnt_ = 0;}

private:
    void move_queue(circular_queue &&other) noexcept {
        max_size_ = other.max_size_;
        head_ = other.head_;
        tail_ = other.tail_;
        override_cnt_ = other.override_cnt_;
        vec_ = std::move(other.vec_);

        other.max_size_ = 0;
        other.head_ = other.tail_ = 0;
        other.override_cnt_ = 0;
    }

    size_t max_size_ = 0;   // max_size_ == 0 表示无效状态
    typename std::vector<T>::size_type head_ = 0;
    typename std::vector<T>::size_type tail_ = 0;
    size_t override_cnt_ = 0;
    std::vector<T> vec_;
};

}   // namespace base
}   // namespace learnlog