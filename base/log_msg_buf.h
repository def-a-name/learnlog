#pragma once

#include "base/log_msg.h"

namespace learnlog {
namespace base {

/*
对 log_msg 的封装类，
log_msg 中的 logger_name 和 msg 类型是 fmt_string_view ，保存在栈上，
log_msg_buf 将 logger_name 和 msg 转存至 fmt_memory_buf
*/

class log_msg_buf : public log_msg {
public:
    log_msg_buf() = default;
    explicit log_msg_buf(const log_msg &init_msg)
        : log_msg{init_msg} {
        buf_.append(logger_name.begin(), logger_name.end());
        buf_.append(msg.begin(), msg.end());
        update_string_view();
    }
    
    log_msg_buf(const log_msg_buf &other)
        : log_msg{other} {
        buf_.append(logger_name.begin(), logger_name.end());
        buf_.append(msg.begin(), msg.end());
        update_string_view();
    }

    log_msg_buf& operator=(const log_msg_buf &other) {
        log_msg::operator=(other);
        buf_.clear();
        buf_.append(other.buf_.data(), other.buf_.data() + other.buf_.size());
        update_string_view();
        return *this;
    }

    log_msg_buf(log_msg_buf &&other) noexcept
        : log_msg{other},
          buf_{std::move(other.buf_)} {
        update_string_view();
    }

    log_msg_buf& operator=(log_msg_buf &&other) noexcept {
        log_msg::operator=(other);
        buf_ = std::move(other.buf_);
        update_string_view();
        return *this;
    }

private:
    void update_string_view() {
        log_msg::logger_name = fmt_string_view(buf_.data(), logger_name.size());
        log_msg::msg = fmt_string_view(buf_.data() + logger_name.size(), msg.size());
    }
    
    fmt_memory_buf buf_;
};

}   // namespace base
}   // namespace learnlog