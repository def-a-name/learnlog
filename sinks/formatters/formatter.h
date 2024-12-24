#pragma once

#include "definitions.h"
#include "base/log_msg.h"

namespace mylog {
namespace sinks {

// 格式化 log_msg 的基类，
// 接收以 log_msg 形式传入的日志消息，处理成要求的样式并写入缓冲区
class formatter {
public:
    virtual ~formatter() = default;
    virtual void format(const base::log_msg &msg, fmt_memory_buf &buf) = 0;
    virtual formatter_uni_ptr clone() const = 0;
};

}    // namespace sinks
}   // namespace mylog