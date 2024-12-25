#pragma once

namespace learnlog {
namespace base {

// 与 std::mutex 结构相同，但不提供具体操作
struct null_mutex {
    void lock() const {}
    void unlock() const {}
};

}   // namespace base
}   // namespace learnlog