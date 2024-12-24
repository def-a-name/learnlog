#pragma once

#include "sinks/sink.h"
#include "sinks/formatters/pattern_formatter.h"
#include "base/static_mutex.h"

#include <mutex>

namespace mylog {
namespace sinks {

// sink 的派生类，
// 是模板参数为 StaticMutex 的模板类，StaticMutex 是静态变量，专用于标准输入输出，
// 如果 log_msg 输出到支持改变颜色的终端，ansicolor_sink 将利用 ANSI 颜色转义序列，
// 给 formatter_ 格式化得到的颜色区间上色，具体颜色由 log_msg 的级别决定；
// 如果终端不支持颜色显示，则正常输出

template <typename StaticMutex>
class ansicolor_sink : public sink {
public:
    using mutex_t = typename StaticMutex::mutex_t;
    
    explicit ansicolor_sink(FILE* file) : mutex_(StaticMutex::mutex()), 
                                          file_(file),
                                          formatter_(mylog::make_unique<pattern_formatter>()) {
        level_colors_.at(level::trace) = white;
        level_colors_.at(level::debug) = cyan;
        level_colors_.at(level::info) = green;
        level_colors_.at(level::warn) = yellow_bold;
        level_colors_.at(level::error) = red_bold;
        level_colors_.at(level::critical) = bold_on_red;
        level_colors_.at(level::off) = reset;

        is_color_enabled_ = base::os::in_terminal(file_) && base::os::is_color_terminal();
    }
    ~ansicolor_sink() override = default;

    ansicolor_sink(const ansicolor_sink& other) = delete;
    ansicolor_sink& operator=(const ansicolor_sink& other) = delete;
    ansicolor_sink(ansicolor_sink&& other) = delete;
    ansicolor_sink& operator=(ansicolor_sink&& other) = delete;

    void set_level_color(level::level_enum level, const std::string& ansi_color) {
        std::lock_guard<mutex_t> lock(mutex_);
        level_colors_.at(static_cast<size_t>(level)) = ansi_color;
    }

    void log(const base::log_msg& msg) override {
        std::lock_guard<mutex_t> lock(mutex_);
        msg.color_index_start = 0;
        msg.color_index_end = 0;
        fmt_memory_buf buf;
        formatter_->format(msg, buf);

        std::string level_color = level_colors_.at(static_cast<size_t>(msg.level));
        size_t bytes_written = 0;
        size_t bytes;
        if (is_color_enabled_ && msg.color_index_end > msg.color_index_start) {
            bytes = ::fwrite(buf.data(), sizeof(char), msg.color_index_start, file_);
            bytes_written += bytes;

            // 根据 log_msg.level 确定颜色，通过添加 ANSI 颜色转义字符串给标记文字区间上色，然后恢复默认颜色
            ::fwrite(level_color.c_str(), sizeof(char), level_color.size(), file_);
            bytes = ::fwrite(buf.data() + msg.color_index_start, sizeof(char), 
                                msg.color_index_end - msg.color_index_start, file_);
            bytes_written += bytes;
            ::fwrite(reset.c_str(), sizeof(char), reset.size(), file_);

            bytes = ::fwrite(buf.data() + msg.color_index_end, sizeof(char), 
                                buf.size() - msg.color_index_end, file_);
            bytes_written += bytes;
        }
        else {
            bytes_written = ::fwrite(buf.data(), sizeof(char), buf.size(), file_);
        }

        if (bytes_written != buf.size()) {
            source_loc loc{__FILE__, __LINE__, __func__};
            throw_mylog_excpt("mylog::ansicolor_sink: fwrite() failed", errno, loc);
        }
    }

    void flush() override {
        std::lock_guard<mutex_t> lock(mutex_);
        ::fflush(file_);
    }

    void set_pattern(const std::string& pattern) override {
        std::lock_guard<mutex_t> lock(mutex_);
        formatter_ = mylog::make_unique<pattern_formatter>(pattern);
    }

    void set_formatter(formatter_uni_ptr formatter) override {
        std::lock_guard<mutex_t> lock(mutex_);
        formatter_ = std::move(formatter);
    }

private:
    mutex_t& mutex_;
    FILE* file_;
    formatter_uni_ptr formatter_;
    bool is_color_enabled_;
    std::array<std::string, MYLOG_LEVELS_NUM> level_colors_;

    // 字体样式
    const std::string reset = "\033[m";
    const std::string bold = "\033[1m";
    const std::string dark = "\033[2m";
    const std::string underline = "\033[4m";
    const std::string blink = "\033[5m";
    const std::string reverse = "\033[7m";
    const std::string concealed = "\033[8m";
    const std::string clear_line = "\033[K";

    // 字体颜色
    const std::string black = "\033[30m";
    const std::string red = "\033[31m";
    const std::string green = "\033[32m";
    const std::string yellow = "\033[33m";
    const std::string blue = "\033[34m";
    const std::string magenta = "\033[35m";
    const std::string cyan = "\033[36m";
    const std::string white = "\033[37m";

    /// 背景颜色
    const std::string on_black = "\033[40m";
    const std::string on_red = "\033[41m";
    const std::string on_green = "\033[42m";
    const std::string on_yellow = "\033[43m";
    const std::string on_blue = "\033[44m";
    const std::string on_magenta = "\033[45m";
    const std::string on_cyan = "\033[46m";
    const std::string on_white = "\033[47m";

    /// 加粗颜色字体
    const std::string yellow_bold = "\033[33m\033[1m";
    const std::string red_bold = "\033[31m\033[1m";
    const std::string bold_on_red = "\033[1m\033[41m";
};

template <typename StaticMutex>
class stdout_ansicolor_sink : public ansicolor_sink<StaticMutex> {
public:
    stdout_ansicolor_sink() : ansicolor_sink<StaticMutex>(stdout) {}
};

template <typename StaticMutex>
class stderr_ansicolor_sink : public ansicolor_sink<StaticMutex> {
public:
    stderr_ansicolor_sink() : ansicolor_sink<StaticMutex>(stderr) {}
};

using stdout_ansicolor_sink_mt = stdout_ansicolor_sink<base::static_mutex>;
using stdout_ansicolor_sink_st = stdout_ansicolor_sink<base::static_nullmutex>;

using stderr_ansicolor_sink_mt = stderr_ansicolor_sink<base::static_mutex>;
using stderr_ansicolor_sink_st = stderr_ansicolor_sink<base::static_nullmutex>;

}    // namespace sinks
}    // namespace mylog