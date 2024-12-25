#include "base/log_msg_buf.h"
#include "base/os.h"

using namespace learnlog;
using namespace base;

log_msg_buf::log_msg_buf(const log_msg &init_msg)
    : log_msg{init_msg} {
        buf_.append(logger_name.begin(), logger_name.end());
        buf_.append(msg.begin(), msg.end());
        update_string_view();
}

log_msg_buf::log_msg_buf(const log_msg_buf &other)
    : log_msg{other} {
        buf_.append(logger_name.begin(), logger_name.end());
        buf_.append(msg.begin(), msg.end());
        update_string_view();
    }

log_msg_buf& log_msg_buf::operator=(const log_msg_buf &other) {
    log_msg::operator=(other);
    buf_.clear();
    buf_.append(other.buf_.data(), other.buf_.data() + other.buf_.size());
    update_string_view();
    return *this;
}

log_msg_buf::log_msg_buf(log_msg_buf &&other) noexcept
    : log_msg{other},
      buf_{std::move(other.buf_)} {
    update_string_view();
}

log_msg_buf& log_msg_buf::operator=(log_msg_buf &&other) noexcept {
    log_msg::operator=(other);
    buf_ = std::move(other.buf_);
    update_string_view();
    return *this;
}

void log_msg_buf::update_string_view() {
    log_msg::logger_name = fmt_string_view(buf_.data(), logger_name.size());
    log_msg::msg = fmt_string_view(buf_.data() + logger_name.size(), msg.size());
}
