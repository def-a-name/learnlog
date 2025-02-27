#pragma once 

#include "definitions.h"
#include "base/os.h"

#include <exception>
#include <string>
#include <atomic>
#include <cassert>
#include <iterator>

// 处理异常，将异常的内容、产生位置（如果提供）组合成异常信息，
// 传给 handle_excpt() 处理
#define LEARNLOG_CATCH                                             \
    catch(const std::exception& e) {                            \
        learnlog::handle_excpt(e.what());                          \
    }                                                           \
    catch(...) {                                                \
        learnlog::handle_excpt("Unknown exception, rethrowing...");\
        throw;                                                  \
    }

namespace learnlog {

// 将异常的编号、时间、信息输出到stderr
// 通过 staic std::atomic 保证编号和时间的一致性、原子性
inline void handle_excpt(const std::string& msg) {
    using tp = sys_clock::time_point;
    static std::atomic<tp> last_report_time{ tp(seconds(0)) };
    static std::atomic<size_t> report_cnt{1};

    tp lr_time = last_report_time.load(std::memory_order_acquire);
    tp now = sys_clock::now();
    size_t cnt = report_cnt.fetch_add(1, std::memory_order_relaxed);
    assert( now - lr_time > std::chrono::nanoseconds(1) );
    // if(now - lr_time < std::chrono::seconds(1)){ return; }
    last_report_time.store(now, std::memory_order_release);

    char dt_buf[32];
    base::os::time_point_to_datetime_sec(dt_buf, sizeof(dt_buf), now);
    std::fprintf(stderr, "[*** EXCEPTION #%04zu ***] [%s] %s\n", cnt, dt_buf, msg.c_str());
}

// 自定义异常
class learnlog_excpt : public std::exception {
public:
    explicit learnlog_excpt(std::string msg) : msg_(std::move(msg)) {}
    
    learnlog_excpt(const std::string &msg, int last_errno) {
        fmt_memory_buf buf;
        fmt::format_system_error(buf, last_errno, msg.c_str());
        msg_ = fmt::to_string(buf);
    }

    learnlog_excpt(const std::string &msg, int last_errno, const source_loc& loc) {
        fmt_memory_buf buf;
        fmt::format_system_error(buf, last_errno, msg.c_str());
        fmt::format_to(std::back_inserter(buf), FMT_STRING("\n[location: {}:{} - {}]"),
                       loc.filename, loc.line, loc.funcname);
        msg_ = fmt::to_string(buf);
    }

    const char* what() const noexcept override{
        return msg_.c_str();
    }

private:
    std::string msg_;
};

[[noreturn]] inline void throw_learnlog_excpt(std::string msg) {
    throw learnlog_excpt(std::move(msg));
}

[[noreturn]] inline void throw_learnlog_excpt(const std::string& msg, int last_errno) {
    throw learnlog_excpt(msg, last_errno);
}

[[noreturn]] inline void throw_learnlog_excpt(const std::string& msg, int last_errno, const source_loc& loc) {
    throw learnlog_excpt(msg, last_errno, loc);
}

}  // namespace learnlog