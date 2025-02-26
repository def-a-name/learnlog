#pragma once

#include "sinks/basic_sink.h"
#include "base/null_mutex.h"
#include "sync_factory.h"

namespace learnlog {
namespace sinks {

// basic_sink 的派生类，
// 是模板参数为 Mutex 的模板类，在需要保证线程安全时传入 std::mutex，不需要时传入 base::null_mutex，
// 不提供具体操作，用于调试

template <typename Mutex>
class null_sink final : public basic_sink<Mutex> {
protected:
    void output_(const base::log_msg &) override {}
    void flush_() override {}
};

using null_sink_mt = null_sink<std::mutex>;
using null_sink_st = null_sink<base::null_mutex>;

}    // namespace sinks

// factory 函数，创建使用 basic_file_sink 的 logger 对象

template <typename Factory = learnlog::sync_factory>
logger_shr_ptr null_logger_mt(const std::string& logger_name) {
    return Factory::template create<sinks::null_sink_mt>(logger_name);
}

template <typename Factory = learnlog::sync_factory>
logger_shr_ptr null_logger_st(const std::string& logger_name) {
    return Factory::template create<sinks::null_sink_st>(logger_name);
}

}    // namespace learnlog