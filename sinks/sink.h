#pragma once

#include "definitions.h"
#include "base/log_msg.h"
#include "sinks/formatters/formatter.h"

#include <atomic>

namespace learnlog {
namespace sinks {

// 所有 sink 的基类，
// sink 是日志消息 log_msg 与实际输出的连接槽，负责接收 logger 处理得到的 log_msg，将其按照指定格式输出到指定位置，
// 格式化由 formatter 实现，每个 sink 只能指定唯一的 formatter（unique_ptr），
// sink 的各个派生类能够将 log_msg 输出到不同位置，每个派生类分别实现一种输出方式，
// 同一个 sink 可能被不同的线程访问，需要在派生类保证线程安全

class sink {
public:
    virtual ~sink() = default;
    virtual void log(const base::log_msg &msg) = 0;                     // 输出 1 条日志消息
    virtual void flush() = 0;                                           // 清空缓冲区，立即输出缓冲区内所有日志消息
    virtual void set_pattern(const std::string &pattern) = 0;           // 设置格式模板字符串
    virtual void set_formatter(formatter_uni_ptr sink_formatter) = 0;   // 指定 formatter

    void set_level(level::level_enum sink_level) {
        sink_level_.store(sink_level, std::memory_order_relaxed);
    }
    level::level_enum get_level() const {
        return static_cast<learnlog::level::level_enum>(sink_level_.load(std::memory_order_relaxed));
    }
    // log_msg 的等级高于 sink 等级才能被 sink 输出
    bool should_log(level::level_enum msg_level) const {
        return msg_level >= sink_level_.load(std::memory_order_relaxed);
    }

protected:
    std::atomic<int> sink_level_{level::level_enum::trace};      // sink 等级
};

}   // namespace sinks
}   // namespace learnlog