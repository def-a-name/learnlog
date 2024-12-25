#pragma once

#include "definitions.h"
#include "base/os.h"
#include "base/log_msg.h"
#include "sinks/formatters/filler.h"

#include <array>

namespace learnlog {
namespace sinks {

// 处理格式化字符的基类，格式化 log_msg 并填充空格使格式对齐
class flag_formatter {
public:
    flag_formatter() = default;
    explicit flag_formatter(const spaces_info& sp_info) : spaces_info_(sp_info) {}
    virtual ~flag_formatter() = default;
    // 派生类各自处理 log_msg 中不同的部分，格式化的结果填入 dest_buf
    virtual void format(const base::log_msg& msg,
                        const std::tm& time_tm,
                        fmt_memory_buf& dest_buf) = 0;

protected:
    spaces_info spaces_info_;   // 空格填充信息
};

// 设定自定义格式化字符，优先级高于预设格式字符；
// 使用时需要写一个该类的派生类 derived，实现 derived.format() 与 derived.clone()
// 再通过 pattern_formatter.add_custom_flag<derived>('<c>') 设置格式字符；
// 待处理完格式字符串生成 spaces_info 后，再调用 set_spaces_info() 设置空格填充信息
class custom_flag_formatter : public flag_formatter {
public:
    virtual void format(const base::log_msg&, const std::tm&, fmt_memory_buf&) = 0;
    // 要实现父类指针指向派生类对象
    virtual std::unique_ptr<custom_flag_formatter> clone() const = 0;

    void set_spaces_info(const spaces_info& sp_info) {
        flag_formatter::spaces_info_ = sp_info;
    }    
};

// 格式化 log_msg.logger_name
class name_formatter final : public flag_formatter {
public:
    explicit name_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}
    
    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        filler f(msg.logger_name.size(), spaces_info_, dest_buf);
        f.fill_msg(msg.logger_name);
    }
};

// 格式化 log_msg.level
class level_formatter final : public flag_formatter {
public:
    explicit level_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}
    
    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        const fmt_string_view& msg_level = level::level_name[static_cast<size_t>(msg.level)];
        filler f(msg_level.size(), spaces_info_, dest_buf);
        f.fill_msg(msg_level);
    }
};

// 日期时间相关

static const std::array<const char *, 7> weekday_name{
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"} };

static const std::array<const char *, 12> month_name{
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sept", "Oct", "Nov", "Dec"}};

// 格式化星期几
class a_formatter final : public flag_formatter {
public:
    explicit a_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}
    
    void format(const base::log_msg&, const std::tm& time_tm, fmt_memory_buf& dest_buf) override {
        fmt_string_view d_name{ weekday_name[static_cast<size_t>(time_tm.tm_wday)] };
        filler f(d_name.size(), spaces_info_, dest_buf);
        f.fill_msg(d_name);
    }
};

// 格式化月份名
class b_formatter final : public flag_formatter {
public:
    explicit b_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}
    
    void format(const base::log_msg&, const std::tm& time_tm, fmt_memory_buf& dest_buf) override {
        fmt_string_view m_name{ month_name[static_cast<size_t>(time_tm.tm_mon)] };
        filler f(m_name.size(), spaces_info_, dest_buf);
        f.fill_msg(m_name);
    }
};

// 将日期时间格式化为 (Sat Oct 26 17:28:50 2024)，相当于格式化字符串 "%a %b %d %H:%M:%S %y"
class c_formatter final : public flag_formatter {
public:
    explicit c_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg&, const std::tm& time_tm, fmt_memory_buf& dest_buf) override {
        const size_t text_len = 24;
        filler f(text_len, spaces_info_, dest_buf);

        // 日期
        base::fmt_base::append_string_view( weekday_name[static_cast<size_t>(time_tm.tm_wday)], 
                                           dest_buf);
        dest_buf.push_back(' ');
        base::fmt_base::append_string_view( month_name[static_cast<size_t>(time_tm.tm_mon)],
                                            dest_buf);
        dest_buf.push_back(' ');
        base::fmt_base::fill_uint(time_tm.tm_mday, 2, dest_buf);
        dest_buf.push_back(' ');

        // 时间
        base::fmt_base::fill_uint(time_tm.tm_hour, 2, dest_buf);
        dest_buf.push_back(':');
        base::fmt_base::fill_uint(time_tm.tm_min, 2, dest_buf);
        dest_buf.push_back(':');
        base::fmt_base::fill_uint(time_tm.tm_sec, 2, dest_buf);
        dest_buf.push_back(' ');
        base::fmt_base::fill_uint(time_tm.tm_year + 1900, 4, dest_buf);
    }
};

// 格式化年份（4位）
class y_formatter final : public flag_formatter {
public:
    explicit y_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg&, const std::tm& time_tm, fmt_memory_buf& dest_buf) override {
        filler f(4, spaces_info_, dest_buf);
        f.fill_msg(time_tm.tm_year + 1900);
    }
};

// 格式化月份（2位）
class m_formatter final : public flag_formatter {
public:
    explicit m_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg&, const std::tm& time_tm, fmt_memory_buf& dest_buf) override {
        filler f(2, spaces_info_, dest_buf);
        f.fill_msg(static_cast<unsigned>(time_tm.tm_mon + 1));
    }
};

// 格式化日（2位）
class d_formatter final : public flag_formatter {
public:
    explicit d_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg&, const std::tm& time_tm, fmt_memory_buf& dest_buf) override {
        filler f(2, spaces_info_, dest_buf);
        f.fill_msg(static_cast<unsigned>(time_tm.tm_mday));
    }
};

// 格式化小时（2位）
class H_formatter final : public flag_formatter {
public:
    explicit H_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg&, const std::tm& time_tm, fmt_memory_buf& dest_buf) override {
        filler f(2, spaces_info_, dest_buf);
        f.fill_msg(static_cast<unsigned>(time_tm.tm_hour));
    }
};

// 格式化分钟（2位）
class M_formatter final : public flag_formatter {
public:
    explicit M_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg&, const std::tm& time_tm, fmt_memory_buf& dest_buf) override {
        filler f(2, spaces_info_, dest_buf);
        f.fill_msg(static_cast<unsigned>(time_tm.tm_min));
    }
};

// 格式化秒（2位）
class S_formatter final : public flag_formatter {
public:
    explicit S_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg&, const std::tm& time_tm, fmt_memory_buf& dest_buf) override {
        filler f(2, spaces_info_, dest_buf);
        f.fill_msg(static_cast<unsigned>(time_tm.tm_sec));
    }
};

// 格式化至毫秒（3位）
class E_formatter final : public flag_formatter {
public:
    explicit E_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        auto milli_sec = base::fmt_base::precise_time<milliseconds>(msg.time);
        filler f(3, spaces_info_, dest_buf);
        f.fill_msg(static_cast<unsigned>(milli_sec.count()));
    }
};

// 格式化至微秒（6位）
class F_formatter final : public flag_formatter {
public:
    explicit F_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        auto micro_sec = base::fmt_base::precise_time<microseconds>(msg.time);
        filler f(6, spaces_info_, dest_buf);
        f.fill_msg(static_cast<unsigned>(micro_sec.count()));
    }
};

// 格式化至纳秒（9位）
class G_formatter final : public flag_formatter {
public:
    explicit G_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        auto nano_sec = base::fmt_base::precise_time<nanoseconds>(msg.time);
        filler f(9, spaces_info_, dest_buf);
        f.fill_msg(static_cast<unsigned>(nano_sec.count()));
    }
};

// 格式化时分秒（22:48:27），相当于格式化字符串 "%H:%M:%S"
class T_formatter final : public flag_formatter {
public:
    explicit T_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg&, const std::tm& time_tm, fmt_memory_buf& dest_buf) override {
        const size_t text_len = 8;
        filler f(text_len, spaces_info_, dest_buf);
        
        base::fmt_base::fill_uint(time_tm.tm_hour, 2, dest_buf);
        dest_buf.push_back(':');
        base::fmt_base::fill_uint(time_tm.tm_min, 2, dest_buf);
        dest_buf.push_back(':');
        base::fmt_base::fill_uint(time_tm.tm_sec, 2, dest_buf);
    }
};

// 格式化与上一条日志的时间间隔，支持不同的时间单位
template <typename Metric>
class duration_formatter final : public flag_formatter {
public:
    explicit duration_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info), last_msg_time_(sys_clock::now()) {}
    
    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        auto delta = (std::max)(msg.time - last_msg_time_, sys_clock::duration::zero());
        auto duration = std::chrono::duration_cast<Metric>(delta);
        last_msg_time_ = msg.time;
        
        size_t duration_cnt = static_cast<size_t>(duration.count());
        size_t text_len = base::fmt_base::count_unsigned_digits(duration_cnt);
        filler f(text_len, spaces_info_, dest_buf);
        base::fmt_base::append_int(duration_cnt, dest_buf);
    }

private:
    sys_clock::time_point last_msg_time_;
};

// 进程线程相关

// 格式化进程id
class p_formatter final : public flag_formatter {
public:
    explicit p_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg&, const std::tm&, fmt_memory_buf& dest_buf) override {
        auto pid = base::os::pid();
        size_t p_len = base::fmt_base::count_unsigned_digits(pid);
        filler f(p_len, spaces_info_, dest_buf);
        base::fmt_base::append_int(pid, dest_buf);
    }
};

// 格式化线程id
class t_formatter final : public flag_formatter {
public:
    explicit t_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        size_t t_len = base::fmt_base::count_unsigned_digits(msg.tid);
        filler f(t_len, spaces_info_, dest_buf);
        base::fmt_base::append_int(msg.tid, dest_buf);
    }
};

// 消息内容相关

// 格式化 log_msg.msg
class v_formatter final : public flag_formatter {
public:
    explicit v_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        filler f(msg.msg.size(), spaces_info_, dest_buf);
        f.fill_msg(msg.msg);
    }
};

// 对于不受支持的格式化字符或非格式化字符，直接加入 dest_buf
class aggregate_formatter final : public flag_formatter {
public:
    aggregate_formatter() = default;

    void add_ch(char ch) { str_ += ch; }
    void format(const base::log_msg &, const std::tm &, fmt_memory_buf& dest_buf) override {
        base::fmt_base::append_string_view(str_, dest_buf);
    }

private:
    std::string str_;
};

// 标记需要上色的字符区间，格式化字符串如 "%^info%$"
class color_start_formatter final : public flag_formatter {
public:
    explicit color_start_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        msg.color_index_start = dest_buf.size();
    }
};

class color_end_formatter final : public flag_formatter {
public:
    explicit color_end_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        msg.color_index_end = dest_buf.size();
    }
};

// 消息位置相关

// 格式化 log_msg.loc，包括代码文件名、行号
class source_loc_formatter final : public flag_formatter {
public:
    explicit source_loc_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg &msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        if (msg.loc.empty()) {
            filler p(0, spaces_info_, dest_buf);
            return;
        }

        size_t text_len = std::char_traits<char>::length(msg.loc.filename) +
                            base::fmt_base::count_unsigned_digits(msg.loc.line) + 1;

        filler f(text_len, spaces_info_, dest_buf);
        base::fmt_base::append_string_view(msg.loc.filename, dest_buf);
        dest_buf.push_back(':');
        base::fmt_base::append_int(msg.loc.line, dest_buf);
    }
};

// 格式化 log_msg.loc 中的文件名
class source_filename_formatter final : public flag_formatter {
public:
    explicit source_filename_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        if (msg.loc.empty()) {
            filler f(0, spaces_info_, dest_buf);
            return;
        }
        size_t text_len = std::char_traits<char>::length(msg.loc.filename);
        filler f(text_len, spaces_info_, dest_buf);
        f.fill_msg(msg.loc.filename);
    }
};

// 格式化 log_msg.loc 中的行号
class source_linenum_formatter final : public flag_formatter {
public:
    explicit source_linenum_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        if (msg.loc.empty()) {
            filler f(0, spaces_info_, dest_buf);
            return;
        }
        size_t text_len = base::fmt_base::count_unsigned_digits(msg.loc.line);
        filler f(text_len, spaces_info_, dest_buf);
        f.fill_msg(msg.loc.line);
    }
};

// 格式化 log_msg.loc 中的函数名
class source_funcname_formatter final : public flag_formatter {
public:
    explicit source_funcname_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        if (msg.loc.empty()) {
            filler f(0, spaces_info_, dest_buf);
            return;
        }
        size_t text_len = std::char_traits<char>::length(msg.loc.funcname);
        filler f(text_len, spaces_info_, dest_buf);
        f.fill_msg(msg.loc.funcname);
    }
};

// 成套格式化调用
// 格式化字符串为 "[%y-%m-%d %H:%M:%S.%E] [%n] [%l] [%s:%#] %v"
class full_formatter final : public flag_formatter {
public:
    explicit full_formatter(const spaces_info& sp_info)
        : flag_formatter(sp_info) {}

    void format(const base::log_msg& msg, const std::tm& time_tm, fmt_memory_buf& dest_buf) override {
        auto duration = msg.time.time_since_epoch();
        seconds secs = std::chrono::duration_cast<seconds>(duration);

        // 填充 datetime_buf_ ，每秒最多填充 1 次
        if(secs != last_call_secs_ || datetime_buf_.size() == 0) {
            datetime_buf_.clear();
            datetime_buf_.push_back('[');
            base::fmt_base::fill_uint(time_tm.tm_year + 1900, 4, datetime_buf_);
            datetime_buf_.push_back('-');
            base::fmt_base::fill_uint(time_tm.tm_mon + 1, 2, datetime_buf_);
            datetime_buf_.push_back('-');
            base::fmt_base::fill_uint(time_tm.tm_mday, 2, datetime_buf_);
            datetime_buf_.push_back(' ');
            base::fmt_base::fill_uint(time_tm.tm_hour, 2, datetime_buf_);
            datetime_buf_.push_back(':');
            base::fmt_base::fill_uint(time_tm.tm_min, 2, datetime_buf_);
            datetime_buf_.push_back(':');
            base::fmt_base::fill_uint(time_tm.tm_sec, 2, datetime_buf_);
            datetime_buf_.push_back('.');

            last_call_secs_ = secs;
        }
        dest_buf.append(datetime_buf_.data(), datetime_buf_.data() + datetime_buf_.size());
        
        // 将 msg.time 精确到毫秒，加入 dest_buf
        milliseconds ms = base::fmt_base::precise_time<milliseconds>(msg.time);
        base::fmt_base::fill_uint(ms.count(), 3, dest_buf);
        dest_buf.push_back(']');
        dest_buf.push_back(' ');

        // 如果 msg.logger_name 非空，以 "[logger_name]" 的形式加入 dest_buf
        if (msg.logger_name.size() > 0) {
            dest_buf.push_back('[');
            base::fmt_base::append_string_view(msg.logger_name, dest_buf);
            dest_buf.push_back(']');
            dest_buf.push_back(' ');
        }

        // 将 msg.level 以 "[level_name]" 的形式加入 dest_buf 并上色
        dest_buf.push_back('[');
        msg.color_index_start = dest_buf.size();
        base::fmt_base::append_string_view(level::level_name[static_cast<size_t>(msg.level)], 
                                            dest_buf);
        msg.color_index_end = dest_buf.size();
        dest_buf.push_back(']');
        dest_buf.push_back(' ');

        // 如果 msg.loc 非空，以 "[filename:line]" 的形式加入 dest_buf
        if(!msg.loc.empty()) {
            dest_buf.push_back('[');
            base::fmt_base::append_string_view(msg.loc.filename, dest_buf);
            dest_buf.push_back(':');
            base::fmt_base::fill_uint(msg.loc.line, 5, dest_buf);
            dest_buf.push_back(']');
            dest_buf.push_back(' ');
        }

        // 将 msg.msg 加入 dest_buf
        base::fmt_base::append_string_view(msg.msg, dest_buf);
    }

private:
    seconds last_call_secs_{0};     // 记录上次调用的时间，精确到秒
    fmt_memory_buf datetime_buf_;   // 缓存 time_tm 精确到秒的部分
};

// 格式化字符 '%'，如 "100%%"
class char_formatter final : public flag_formatter {
public:
    explicit char_formatter(char ch)
        : ch_(ch) {}

    void format(const base::log_msg&, const std::tm&, fmt_memory_buf& dest_buf) override {
        dest_buf.push_back(ch_);
    }

private:
    char ch_;
};

}    // namespace sinks
}    // namespace learnlog