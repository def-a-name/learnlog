#pragma once

#include "definitions.h"
#include "base/os.h"
#include "base/exception.h"
#include "base/static_mutex.h"
#include "base/log_msg.h"
#include "sinks/sink.h"
#include "sinks/formatters/pattern_formatter.h"
#include "sync_factory.h"

#ifdef _WIN32
    #include "win.h"
    #include "base/wchar_support.h"
    
    #include <fileapi.h>
    #include <io.h>
    #include <stdio.h>
#endif  // _WIN32

namespace mylog {
namespace sinks {

// sink 的派生类，
// 是模板参数为 StaticMutex 的模板类，StaticMutex 是静态变量，专用于标准输入输出，
// std_sink 覆写了父类的 log()、flush()、set_pattern()、set_formatter() 函数，
// 根据构造时传入的 FILE* 决定将 log_msg 输出到 stdout 还是 stderr

template <typename StaticMutex>
class std_sink : public sink {
public:
    using mutex_t = typename StaticMutex::mutex_t;

    explicit std_sink(FILE* file) : mutex_(StaticMutex::mutex()), 
                                    file_(file), 
                                    formatter_(mylog::make_unique<pattern_formatter>()) {
#ifdef _WIN32
        handle_ = reinterpret_cast<HANDLE>(::_get_osfhandle(::_fileno(file_)));
        // 如果 FILE* 既不是 stdout 也不是 stderr，且 handle_ 为无效句柄，才抛出异常，终止程序
        if (file != stdout && file != stderr && handle_ == INVALID_HANDLE_VALUE) {
            source_loc loc{__FILE__, __LINE__, __func__};
            throw_mylog_excpt("mylog::std_sink: invalid FILE*, get HANDLE failed", 
                                base::os::get_errno(), loc);
        }
#endif  // _WIN32
    }

    ~std_sink() override = default;

    std_sink(const std_sink& other) = delete;
    std_sink& operator=(const std_sink& other) = delete;
    std_sink(std_sink&& other) = delete;
    std_sink& operator=(std_sink&& other) = delete;

    void log(const base::log_msg& msg) override {
#ifdef _WIN32
        if (handle_ == INVALID_HANDLE_VALUE) return;
        
        std::lock_guard<mutex_t> lock(mutex_);
        fmt_memory_buf buf;
        formatter_->format(msg, buf);
        // 将 utf-8 转换为 wchar(utf-16)，避免乱码
        fmt_wmemory_buf wbuf;
        base::utf8buf_to_wcharbuf(buf, msg.color_index_start, msg.color_index_end, wbuf);

        DWORD wbuf_size = static_cast<DWORD>(wbuf.size());
        DWORD bytes_written = 0;
        ::WriteConsoleW(handle_, wbuf.data(), wbuf_size, &bytes_written, nullptr);

        if (bytes_written != wbuf_size) {
            source_loc loc{__FILE__, __LINE__, __func__};
            throw_mylog_excpt("mylog::std_sink: WriteConsoleW() failed", 
                                base::os::get_errno(), loc);
        }
#else
        std::lock_guard<mutex_t> lock(mutex_);
        fmt_memory_buf buf;
        formatter_->format(msg, buf);
        size_t bytes_written = ::fwrite(buf.data(), sizeof(char), buf.size(), file_);

        if (bytes_written != buf.size()) {
            source_loc loc{__FILE__, __LINE__, __func__};
            throw_mylog_excpt("mylog::std_sink: fwrite() failed", 
                                base::os::get_errno(), loc);
        }
#endif
        ::fflush(file_);    // 每条日志输出后立即清空缓冲区
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
#ifdef _WIN32
    HANDLE handle_;
#endif  // _WIN32
};

template <typename StaticMutex>
class stdout_sink : public std_sink<StaticMutex> {
public:
    stdout_sink() : std_sink<StaticMutex>(stdout) {}
};

template <typename StaticMutex>
class stderr_sink : public std_sink<StaticMutex> {
public:
    stderr_sink() : std_sink<StaticMutex>(stderr) {}
};

using stdout_sink_mt = stdout_sink<base::static_mutex>;
using stdout_sink_st = stdout_sink<base::static_nullmutex>;

using stderr_sink_mt = stderr_sink<base::static_mutex>;
using stderr_sink_st = stderr_sink<base::static_nullmutex>;

}   // namespace sinks

// factory 函数，创建使用 std_sink 的 logger 对象

template <typename Factory = mylog::sync_factory>
logger_shr_ptr stdout_logger_mt(const std::string& logger_name) {
    return Factory::template create<sinks::stdout_sink_mt>(logger_name);
}

template <typename Factory = mylog::sync_factory>
logger_shr_ptr stdout_logger_st(const std::string& logger_name) {
    return Factory::template create<sinks::stdout_sink_st>(logger_name);
}

template <typename Factory = mylog::sync_factory>
logger_shr_ptr stderr_logger_mt(const std::string& logger_name) {
    return Factory::template create<sinks::stderr_sink_mt>(logger_name);
}

template <typename Factory = mylog::sync_factory>
logger_shr_ptr stderr_logger_st(const std::string& logger_name) {
    return Factory::template create<sinks::stderr_sink_st>(logger_name);
}

}   // namespace mylog