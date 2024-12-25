#pragma once

#include "sinks/sink.h"
#include "sinks/formatters/pattern_formatter.h"

#include <mutex>

namespace learnlog {
namespace sinks {

// sink 的派生类，
// 是模板参数为 Mutex 的模板类，在需要保证线程安全时传入 std::mutex，不需要时传入 base::null_mutex，
// basic_sink 覆写了父类的 log()、flush()、set_pattern()、set_formatter() 函数，并
// 分别交由自己的虚函数 output_()、flush_()、set_pattern_()、set_formatter_() 实现，
// 其中 output_()、flush_() 是纯虚函数，basic_sink 的子类中必须要实现

template <typename Mutex>
class basic_sink : public sink {
public:
    ~basic_sink() override = default;
    basic_sink() : formatter_{learnlog::make_unique<pattern_formatter>()} {}
    explicit basic_sink(formatter_uni_ptr formatter) : formatter_{std::move(formatter)} {}

    basic_sink(const basic_sink &) = delete;
    basic_sink& operator=(const basic_sink &) = delete;
    basic_sink(basic_sink &&) = delete;
    basic_sink& operator=(basic_sink &&) = delete;

    void log(const base::log_msg &msg) final override {
        std::lock_guard<Mutex> lock(mutex_);
        output_(msg);
    }

    void flush() final override {
        std::lock_guard<Mutex> lock(mutex_);
        flush_();
    }

    void set_pattern(const std::string &pattern) final override {
        std::lock_guard<Mutex> lock(mutex_);
        set_pattern_(pattern);
    }

    void set_formatter(formatter_uni_ptr formatter) final override {
        std::lock_guard<Mutex> lock(mutex_);
        set_formatter_(std::move(formatter));
    }

protected:
    virtual void output_(const base::log_msg &msg) = 0;
    virtual void flush_() = 0;

    // 以模板字符串 pattern 创建 pattern_formatter
    virtual void set_pattern_(const std::string &pattern) {
        formatter_ = learnlog::make_unique<pattern_formatter>(pattern);
    }

    virtual void set_formatter_(formatter_uni_ptr formatter) {
        formatter_ = std::move(formatter);
    }

    formatter_uni_ptr formatter_;
    Mutex mutex_;
};

}   // namespace sinks
}   // namespace learnlog