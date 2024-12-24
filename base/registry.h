#pragma once

#include "definitions.h"
#include "logger.h"
#include "sinks/formatters/pattern_formatter.h"
#include "base/periodic_function.h"
#include "base/thread_pool.h"

#include <unordered_map>
#include <functional>
#include <mutex>

namespace mylog {
class logger;

namespace base {
class thread_pool;

// 单例类，实现对 logger、flusher、thread_pool 三类资源的注册与管理，
// 通过 logger名称（logger_name）对 logger 进行增删改查，
// logger_name 是 logger 对象的唯一标识符，出现缺失、重复时会抛出异常

class registry {
public:
    registry(const registry&) = delete;
    registry& operator=(const registry&) = delete;

    void initialize_logger(logger_shr_ptr new_logger);
    void register_logger(logger_shr_ptr new_logger);
    void set_auto_register_logger(bool flag);
    logger_shr_ptr get_logger(const std::string& logger_name);
    void remove_logger(const std::string& logger_name); 
    logger_shr_ptr get_default_logger();
    void set_default_logger(logger_shr_ptr new_default_logger);
    void set_global_pattern(std::string pattern);
    void set_global_formatter(formatter_uni_ptr new_formatter);
    void set_pattern(const std::string& logger_name, std::string pattern);
    void set_log_level(const std::string& logger_name, level::level_enum log_level);
    void set_global_log_level(level::level_enum log_level);
    void set_flush_level(const std::string& logger_name, level::level_enum flush_level);
    void set_global_flush_level(level::level_enum flush_level);
    void exec_all(const std::function<void(logger_shr_ptr)>& func);
    void flush_all();
    void remove_all();
    
    template <typename Rep, typename Period>
    void flush_every(std::chrono::duration<Rep, Period> interval) {
        auto func = [this] { this->flush_all(); };
        std::lock_guard<std::mutex> lock(flusher_mutex_);
        global_flusher_ = mylog::make_unique<periodic_function>(func, interval);
    }

    void initialize_thread_pool(size_t msg_queue_size, size_t thread_num);
    void register_thread_pool(thread_pool_shr_ptr new_thread_pool);
    thread_pool_shr_ptr get_thread_pool();

    void close_registry();

    static registry& instance();

private:
    registry();
    ~registry();
    void register_logger_(logger_shr_ptr new_logger);

    std::mutex loggers_mutex_;
    formatter_uni_ptr global_formatter_;
    std::string global_pattern_ = "%+";
    level::level_enum global_log_level_ = level::info;
    level::level_enum global_flush_level_ = level::off;
    bool auto_register_logger_ = true;
    logger_shr_ptr default_logger_;
    std::unordered_map<std::string, logger_shr_ptr > loggers_;

    std::mutex flusher_mutex_;
    std::unique_ptr<periodic_function> global_flusher_;

    std::mutex thread_mutex_;
    thread_pool_shr_ptr thread_pool_;
};

}   // namespace base
}   // namespace mylog