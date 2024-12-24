#pragma once

#include "base/log_msg.h"

namespace mylog {
namespace base {

/*
对 log_msg 的封装类，
log_msg 中的 logger_name 和 msg 类型是 fmt_string_view ，保存在栈上，
log_msg_buf 将 logger_name 和 msg 转存至 fmt_memory_buf
*/

class log_msg_buf : public log_msg {
public:
    log_msg_buf() = default;
    explicit log_msg_buf(const log_msg &init_msg);
    
    log_msg_buf(const log_msg_buf &other);
    log_msg_buf& operator=(const log_msg_buf &other);

    log_msg_buf(log_msg_buf &&other) noexcept;
    log_msg_buf& operator=(log_msg_buf &&other) noexcept;

private:
    void update_string_view();
    fmt_memory_buf buf_;
};

}   // namespace base
}   // namespace mylog