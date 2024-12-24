#pragma once

#ifdef _WIN32
    #include "sinks/wincolor_sink.h"
#else
    #include "sinks/ansicolor_sink.h"
#endif

#include "sync_factory.h"

namespace mylog {
namespace sinks {

#ifdef _WIN32
    using stdout_color_sink_mt = stdout_wincolor_sink_mt;
    using stdout_color_sink_st = stdout_wincolor_sink_st;
    using stderr_color_sink_mt = stderr_wincolor_sink_mt;
    using stderr_color_sink_st = stderr_wincolor_sink_st;
#else
    using stdout_color_sink_mt = stdout_ansicolor_sink_mt;
    using stdout_color_sink_st = stdout_ansicolor_sink_st;
    using stderr_color_sink_mt = stderr_ansicolor_sink_mt;
    using stderr_color_sink_st = stderr_ansicolor_sink_st;
#endif

}   // namespace sinks

// factory 函数，创建使用 std_color_sink 的 logger 对象

template <typename Factory = mylog::sync_factory>
logger_shr_ptr stdout_color_logger_mt(const std::string& logger_name) {
    return Factory::template create<sinks::stdout_color_sink_mt>(logger_name);
}

template <typename Factory = mylog::sync_factory>
logger_shr_ptr stdout_color_logger_st(const std::string& logger_name) {
    return Factory::template create<sinks::stdout_color_sink_st>(logger_name);
}

template <typename Factory = mylog::sync_factory>
logger_shr_ptr stderr_color_logger_mt(const std::string& logger_name) {
    return Factory::template create<sinks::stderr_color_sink_mt>(logger_name);
}

template <typename Factory = mylog::sync_factory>
logger_shr_ptr stderr_color_logger_st(const std::string& logger_name) {
    return Factory::template create<sinks::stderr_color_sink_st>(logger_name);
}

}   // namespace mylog