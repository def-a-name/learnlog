#pragma once

#include "definitions.h"
#include "base/exception.h"
#include "base/log_msg.h"
#include "base/backtracer.h"

#include <vector>
#include <atomic>

namespace mylog {

// 所有 logger 的基类，
// 用户的调用指令最先经由 logger 处理，logger 根据用户传入的各式参数生成消息结构体 log_msg，
// 如果消息等级达到或高于允许被记录的等级，log_msg 会被交给 sink 处理，
// 如果开启 backtrace，最后输出的 n 条 log_msg 会被备份，出现问题时可以回溯，
// logger 为主要函数 log() 重载了多种调用方法，丰富了用户接口，
// log() 函数主要的调用逻辑是 log() -> log_() -> do_log_() -> sink_log_()

class logger {
public:
    explicit logger(std::string name) :
        name_(std::move(name)),
        sinks_() {} 

    logger(std::string name, sink_shr_ptr single_sink) :
        name_(std::move(name)),
        sinks_{std::move(single_sink)} {}

    template <typename It>
    logger(std::string name, It begin, It end) :
        name_(std::move(name)),
        sinks_(begin, end) {}

    logger(std::string name, sinks_init_list sinks) :
        logger(std::move(name), sinks.begin(), sinks.end()) {}

    virtual ~logger() = default;

    logger(const logger& other) = default;
    logger(logger&& other) = default;
    void swap(logger& other) noexcept;

    // 模板函数 log() 的完整参数实现，msg 以 fmt_format_string 类型及可变参数传入
    template <typename... Args>
    void log(source_loc loc, 
             level::level_enum level, 
             fmt_format_string<Args...> fmt_fstr, 
             Args &&...args) {
        log_(loc, level, fmt_string_view{fmt_fstr}, std::forward<Args>(args)...);
    }

    // 模板函数 log() ，缺省 source_loc
    template <typename... Args>
    void log(level::level_enum level, fmt_format_string<Args...> fmt_fstr, Args &&...args) {
        log_(source_loc{}, level, fmt_string_view{fmt_fstr}, std::forward<Args>(args)...);
    }

    // 模板函数 log() ，缺省 source_loc，将 msg 隐式转换为 fmt_format_string
    template <typename T>
    void log(level::level_enum level, const T& msg) {
        log(source_loc{}, level, msg);
    }

    // 模板函数 log() ，如果 msg 不能隐式转换为 fmt_format_string，
    // 则通过 "{}, msg" 构造为 fmt_format_string
    template <typename T,
              typename std::enable_if<!is_convertible_to_basic_format_string<const T &>::value,
                                      int>::type = 0>
    void log(source_loc loc, level::level_enum lvl, const T &msg) {
        log(loc, lvl, "{}", msg);
    }

    // 非模板函数 log() 的完整参数实现，msg 直接以 fmt_string_view 类型传入
    // 跳过中间函数套用，实现 log_() 的功能
    void log(sys_clock::time_point time, 
             source_loc loc, level::level_enum level, fmt_string_view msg) {
        bool log_enabled = should_log_(level);
        bool backtrace_enabled = false;
        if ( tracer_ != nullptr && tracer_->is_enabled() ) {
            backtrace_enabled = true;
        }
        if (!log_enabled && !backtrace_enabled) {
            return;
        }
        
        base::log_msg log_msg(time, loc, level, msg, name_);
        do_log_(log_msg, log_enabled, backtrace_enabled);
    }

    // 非模板函数 log() ，缺省 log time ，msg 直接以 fmt_string_view 类型传入
    // 跳过中间函数套用，实现 log_() 的功能
    void log(source_loc loc, level::level_enum level, fmt_string_view msg) {
        bool log_enabled = should_log_(level);
        bool backtrace_enabled = false;
        if ( tracer_ != nullptr && tracer_->is_enabled() ) {
            backtrace_enabled = true;
        }
        if (!log_enabled && !backtrace_enabled) {
            return;
        }
        
        base::log_msg log_msg(loc, level, msg, name_);
        do_log_(log_msg, log_enabled, backtrace_enabled);
    }

    // 非模板函数 log() ，缺省 time 与 loc ，msg 直接以 fmt_string_view 类型传入
    void log(level::level_enum level, fmt_string_view msg) {
        log(source_loc{}, level, msg);
    }


    template <typename... Args>
    void trace(fmt_format_string<Args...> fmt_fstr, Args &&...args) {
        log(level::level_enum::trace, fmt_fstr, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(fmt_format_string<Args...> fmt_fstr, Args &&...args) {
        log(level::level_enum::debug, fmt_fstr, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(fmt_format_string<Args...> fmt_fstr, Args &&...args) {
        log(level::level_enum::info, fmt_fstr, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(fmt_format_string<Args...> fmt_fstr, Args &&...args) {
        log(level::level_enum::warn, fmt_fstr, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(fmt_format_string<Args...> fmt_fstr, Args &&...args) {
        log(level::level_enum::error, fmt_fstr, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void critical(fmt_format_string<Args...> fmt_fstr, Args &&...args) {
        log(level::level_enum::critical, fmt_fstr, std::forward<Args>(args)...);
    }

    template <typename T>
    void trace(const T &msg) { log(level::level_enum::trace, msg); }

    template <typename T>
    void debug(const T &msg) { log(level::level_enum::debug, msg); }

    template <typename T>
    void info(const T &msg) { log(level::level_enum::info, msg); }

    template <typename T>
    void warn(const T &msg) { log(level::level_enum::warn, msg); }

    template <typename T>
    void error(const T &msg) { log(level::level_enum::error, msg); }

    template <typename T>
    void critical(const T &msg) { log(level::level_enum::critical, msg); }


    void set_log_level(level::level_enum log_level) {
        log_level_ = log_level;
    }

    level::level_enum get_log_level() const {
        return log_level_;
    }

    void set_flush_level(level::level_enum flush_level) {
        flush_level_ = flush_level;
    }
    
    level::level_enum get_flush_level() const {
        return flush_level_;
    }

    void flush() { flush_sink_(); }

    void set_pattern(std::string pattern);

    std::string& get_pattern() { return pattern_; }

    void set_formatter(formatter_uni_ptr formatter);

    void enable_backtrace_n(size_t msg_num) {
        if (tracer_ == nullptr) {
            tracer_ = std::make_shared<base::backtracer>();
        }
        tracer_->enable_with_size(msg_num);
    }
    void disable_backtrace() { tracer_->disable(); }
    void dump_backtrace();

    const std::vector<sink_shr_ptr>& sinks() const { return sinks_; }
    std::vector<sink_shr_ptr>& sinks() { return sinks_; }
    const std::string& name() const { return name_;}

    // 借助 make_shared 复制出新的 logger 对象，并返回共享指针
    virtual logger_shr_ptr clone(std::string new_name);

protected:
    std::string name_;                              
    std::vector<sink_shr_ptr> sinks_;               
    level::level_enum log_level_ = level::info;     
    level::level_enum flush_level_ = level::off;    
    std::string pattern_ = "%+";                    
    std::shared_ptr<base::backtracer> tracer_;      

    bool should_log_(level::level_enum msg_level) const {
        return msg_level >= log_level_;
    }

    bool should_flush_(level::level_enum msg_level) const {
        return (msg_level >= flush_level_) && (msg_level != level::off);
    }

    template <typename... Args>
    void log_(source_loc loc, level::level_enum level, fmt_string_view fmt_strv, Args &&...args) {
        bool log_enabled = should_log_(level);
        bool backtrace_enabled = false;
        if ( tracer_ != nullptr && tracer_->is_enabled() ) {
            backtrace_enabled = true;
        }
        if (!log_enabled && !backtrace_enabled) {
            return;
        }
        try {
            fmt_memory_buf buf;
            fmt::vformat_to(fmt::appender(buf), fmt_strv, fmt::make_format_args(args...));
        
            base::log_msg log_msg(loc, level, fmt_string_view(buf.data(), buf.size()), name_);
            do_log_(log_msg, log_enabled, backtrace_enabled);
        }
        MYLOG_CATCH
    }

    void do_log_(const base::log_msg& msg, bool log_enabled, bool backtrace_enabled);
    virtual void sink_log_(const base::log_msg& msg);
    virtual void flush_sink_();
};

void swap(logger& a, logger& b);

}   // namespace mylog