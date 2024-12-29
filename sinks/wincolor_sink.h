#pragma once

#include "sinks/sink.h"
#include "win.h"
#include "base/static_mutex.h"
#include "base/wchar_support.h"

#include <mutex>

namespace learnlog {
namespace sinks {

// sink 的派生类，
// 是模板参数为 StaticMutex 的模板类，StaticMutex 是静态变量，专用于标准输入输出，
// wincolor_sink 将利用 windows api 接口，给 formatter_ 格式化得到的颜色区间上色，具体颜色
// 由 log_msg 的级别决定

template <typename StaticMutex>
class wincolor_sink : public sink {
public:
    using mutex_t = typename StaticMutex::mutex_t;

    wincolor_sink(void* handle) : mutex_(StaticMutex::mutex()), 
                                  handle_(handle),
                                  formatter_(learnlog::make_unique<pattern_formatter>()) {
        DWORD console_mode;
        bool in_console = ::GetConsoleMode(handle_, &console_mode) != 0;
        is_color_enabled_ = in_console;     // win 环境下，只要 sink 输出到控制台，都可以上色
        
        level_colors_[level::trace] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;        // white
        level_colors_[level::debug] = FOREGROUND_GREEN | FOREGROUND_BLUE;                         // cyan
        level_colors_[level::info] = FOREGROUND_GREEN;                                            // green
        level_colors_[level::warn] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;    // intense yellow
        level_colors_[level::error] = FOREGROUND_RED | FOREGROUND_INTENSITY;                        // intense red
        level_colors_[level::critical] = BACKGROUND_RED | 
                                   FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
                                   FOREGROUND_INTENSITY;                                   // intense white on red background
        level_colors_[level::off] = 0;
    }
    ~wincolor_sink() override = default;

    wincolor_sink(const wincolor_sink& other) = delete;
    wincolor_sink& operator=(const wincolor_sink& other) = delete;
    wincolor_sink(wincolor_sink&& other) = delete;
    wincolor_sink& operator=(wincolor_sink&& other) = delete;

    void set_level_color(level::level_enum level, WORD win_color) {
        std::lock_guard<mutex_t> lock(mutex_);
        level_colors_.at(static_cast<size_t>(level)) = win_color;
    }

    void log(const base::log_msg& msg) override {
        if (handle_ == nullptr || handle_ == INVALID_HANDLE_VALUE) return;

        std::lock_guard<mutex_t> lock(mutex_);
        msg.color_index_start = 0;
        msg.color_index_end = 0;
        fmt_memory_buf buf;
        formatter_->format(msg, buf);
        // 将 utf-8 转换为 wchar(utf-16)，避免乱码
        fmt_wmemory_buf wbuf;
        base::utf8buf_to_wcharbuf(buf, msg.color_index_start, msg.color_index_end, wbuf);

        WORD level_color = level_colors_.at(static_cast<size_t>(msg.level));
        size_t bytes_written = 0;
        DWORD bytes;
        if (is_color_enabled_ && msg.color_index_end > msg.color_index_start) {
            ::WriteConsoleW(handle_, wbuf.data(), msg.color_index_start, &bytes, nullptr);
            bytes_written += bytes;

            // 根据 log_msg.level 确定颜色，通过 win api 给标记文字区间上色，然后恢复默认颜色
            WORD origin_color = get_text_color_();
            level_color = level_color | (origin_color & 0xf0);  // 保留当前控制台的背景颜色
            set_text_color_(level_color);
            ::WriteConsoleW(handle_, wbuf.data() + msg.color_index_start, 
                            msg.color_index_end - msg.color_index_start, &bytes, nullptr);
            bytes_written += bytes;
            set_text_color_(origin_color);

            ::WriteConsoleW(handle_, wbuf.data() + msg.color_index_end, 
                            wbuf.size() - msg.color_index_end, &bytes, nullptr);
            bytes_written += bytes;
        }
        else {
            ::WriteConsoleW(handle_, wbuf.data(), wbuf.size(), &bytes, nullptr);
            bytes_written = bytes;
        }

        if (bytes_written != wbuf.size()) {
            source_loc loc{__FILE__, __LINE__, __func__};
            throw_learnlog_excpt("learnlog::wincolor_sink: WriteConsoleW() failed", ::GetLastError(), loc);
        }
    }

    void flush() override {
        // windows 控制台的输出缓冲区自动刷新
    }

    void set_pattern(const std::string& pattern) override {
        std::lock_guard<mutex_t> lock(mutex_);
        formatter_ = learnlog::make_unique<pattern_formatter>(pattern);
    }

    void set_formatter(formatter_uni_ptr formatter) override {
        std::lock_guard<mutex_t> lock(mutex_);
        formatter_ = std::move(formatter);
    }

private:
    mutex_t& mutex_;
    HANDLE handle_;
    formatter_uni_ptr formatter_;
    bool is_color_enabled_;
    std::array<WORD, LEARNLOG_LEVELS_NUM> level_colors_;

    // 获取当前控制台的字体颜色
    WORD get_text_color_() {
        CONSOLE_SCREEN_BUFFER_INFO con_buf_info;  
        // 如果获取不到 console buffer info，默认返回白色
        if(!::GetConsoleScreenBufferInfo(handle_, &con_buf_info)) {
            return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }
        return (con_buf_info.wAttributes & 0xff);
    }

    // 设置当前控制台的字体颜色
    BOOL set_text_color_(WORD color) {
        CONSOLE_SCREEN_BUFFER_INFO con_buf_info;  
        // 如果获取不到 console buffer info，不操作
        if(!::GetConsoleScreenBufferInfo(handle_, &con_buf_info)) {
            return false;
        }
        return ::SetConsoleTextAttribute(handle_, color);
    }
};

template <typename StaticMutex>
class stdout_wincolor_sink : public wincolor_sink<StaticMutex> {
public:
    stdout_wincolor_sink() : wincolor_sink<StaticMutex>(::GetStdHandle(STD_OUTPUT_HANDLE)) {}
};

template <typename StaticMutex>
class stderr_wincolor_sink : public wincolor_sink<StaticMutex> {
public:
    stderr_wincolor_sink() : wincolor_sink<StaticMutex>(::GetStdHandle(STD_ERROR_HANDLE)) {}
};

using stdout_wincolor_sink_mt = stdout_wincolor_sink<base::static_mutex>;
using stdout_wincolor_sink_st = stdout_wincolor_sink<base::static_nullmutex>;

using stderr_wincolor_sink_mt = stderr_wincolor_sink<base::static_mutex>;
using stderr_wincolor_sink_st = stderr_wincolor_sink<base::static_nullmutex>;

}   // namespace sinks
}   // namespace learnlog