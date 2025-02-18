#pragma once

#include "definitions.h"
#include "base/os.h"

namespace learnlog {
namespace base {

// 保存日志信息的底层结构体

struct log_msg {
    log_msg() = default;
    log_msg(sys_clock::time_point time_in,  // 登记时间
            source_loc loc_in,              // 发生位置
            level::level_enum level_in,     // 日志等级
            fmt_string_view msg_in,         // 日志内容
            fmt_string_view logger_name_in) // 处理该日志的logger名称
        : logger_name(logger_name_in),
          msg(msg_in),
          level(level_in),
          loc(loc_in),
          time(time_in),
          tid(os::thread_id()) {}
    
    // 使用当前系统时间
    log_msg(source_loc loc_in, level::level_enum level_in, fmt_string_view msg_in, fmt_string_view logger_name_in)
        : log_msg(sys_clock::now(), loc_in, level_in, msg_in, logger_name_in) {}
    
    // 使用当前系统时间，发生位置留空
    log_msg(level::level_enum level_in, fmt_string_view msg_in, fmt_string_view logger_name_in)
        : log_msg(sys_clock::now(), source_loc{}, level_in, msg_in, logger_name_in) {}

    log_msg(const log_msg &other) = default;
    log_msg &operator=(const log_msg &other) = default;

    fmt_string_view logger_name;
    fmt_string_view msg;
    level::level_enum level{level::off};
    source_loc loc;
    sys_clock::time_point time;
    size_t tid{0};

    // 格式化后，上色的字符索引区间
    mutable size_t color_index_start{0};
    mutable size_t color_index_end{0};
};

}   // namespace base
}   // namespace learnlog