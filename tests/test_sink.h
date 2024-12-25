#pragma once

#include "sinks/basic_sink.h"
#include "base/null_mutex.h"

namespace learnlog {
namespace sinks {

template <typename Mutex>
class test_sink : public basic_sink<Mutex> {
public:
    explicit test_sink() = default;

    size_t msg_count() {
        std::lock_guard<Mutex> lock(basic_sink<Mutex>::mutex_);
        return msg_cnt_;
    }

    size_t flush_count() {
        std::lock_guard<Mutex> lock(basic_sink<Mutex>::mutex_);
        return flush_cnt_;
    }

    void set_sink_delay_ms(size_t sink_delay_ms) {
        std::lock_guard<Mutex> lock(basic_sink<Mutex>::mutex_);
        delay_ms_ = sink_delay_ms;
    }

    std::vector<std::string> msgs() {
        std::lock_guard<Mutex> lock(basic_sink<Mutex>::mutex_);
        return log_msgs_;
    }

private:
    void output_(const base::log_msg &msg) override {
        fmt_memory_buf buf;
        basic_sink<Mutex>::formatter_->format(msg, buf);
        log_msgs_.emplace_back(buf.data(), buf.size());
        msg_cnt_++;
        base::os::sleep_for_ms(delay_ms_);
    }

    void flush_() override {
        flush_cnt_++;
    }

    size_t msg_cnt_{0};
    size_t flush_cnt_{0};
    size_t delay_ms_{0};
    std::vector<std::string> log_msgs_;
};

using test_sink_mt = test_sink<std::mutex>;
using test_sink_st = test_sink<base::null_mutex>;

}   // namespace sinks
}   // namespace learnlog