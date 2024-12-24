#include "base/log_msg.h"
#include "base/os.h"

using namespace mylog;
using namespace base;

log_msg::log_msg(sys_clock::time_point time_in,
                 source_loc loc_in,
                 level::level_enum level_in,
                 fmt_string_view msg_in,
                 fmt_string_view logger_name_in)
    : logger_name(logger_name_in),
      msg(msg_in),
      level(level_in),
      loc(loc_in),
      time(time_in),
      tid(os::thread_id()) {}

log_msg::log_msg(source_loc loc_in,
                 level::level_enum level_in,
                 fmt_string_view msg_in,
                 fmt_string_view logger_name_in)
    : log_msg(sys_clock::now(), loc_in, level_in, msg_in, logger_name_in) {}

log_msg::log_msg(level::level_enum level_in,
                 fmt_string_view msg_in,
                 fmt_string_view logger_name_in)
    : log_msg(sys_clock::now(), source_loc{}, level_in, msg_in, logger_name_in) {}