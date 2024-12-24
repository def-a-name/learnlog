#pragma once

#include "base/null_mutex.h"
#include "sinks/basic_sink.h"

#include <mutex>
#include <ostream>

namespace mylog {
namespace sinks {

// basic_sink 的派生类，
// 是模板参数为 Mutex 的模板类，在需要保证线程安全时传入 std::mutex，不需要时传入 base::null_mutex，
// ostream_sink 覆写了父类的 output_()、flush_() 函数，将格式化后的 log_msg 写入 std::ostream

template <typename Mutex>
class ostream_sink final : public basic_sink<Mutex> {
public:
    explicit ostream_sink(std::ostream& os, bool force_flush = false)
        : ostream_(os),
          force_flush_(force_flush) {}
    
    ostream_sink(const ostream_sink &) = delete;
    ostream_sink& operator=(const ostream_sink &) = delete;

private:
    void output_(const base::log_msg& msg) override {
        fmt_memory_buf buf;
        basic_sink<Mutex>::formatter_->format(msg, buf);
        ostream_.write(buf.data(), static_cast<std::streamsize>(buf.size()));
        
        if (force_flush_) { ostream_.flush(); }
    }

    void flush_() override { ostream_.flush(); }

    std::ostream& ostream_;
    bool force_flush_;      // 是否在将 1 条 log_msg 写入 ostream 后立即清空缓冲区
};

using ostream_sink_mt = ostream_sink<std::mutex>;
using ostream_sink_st = ostream_sink<base::null_mutex>;

}    // namespace sinks
}    // namespace mylog