#pragma once

// learnlog 日志库的主要头文件，包含用户接口函数，用户可通过 learnlog::<function> 进行调用，
// 函数主要是对单例 registry 的封装，实现位于 learnlog.cpp，
// 由创建 logger 时设定的名称（logger_name）对 logger 进行增删改查

#include "definitions.h"
#include "sync_factory.h"
#include "async_factory.h"
#include "base/exception.h"
#include "version.h"

namespace learnlog {

// 根据全局设置对 new_logger 进行初始化，设置项包括
// 格式化器（formatter）、日志记录级别（log_level）、缓冲区刷新级别（flush_level），
// 初始化完成后默认注册 logger，该步骤可由 set_auto_register_logger(false) 取消
// example:
//  auto sink = std::make_shared<learnlog::sinks::basic_file_sink_mt>("filename");
//  auto logger = std::make_shared<learnlog::logger>("logger_name", sink);
//  learnlog::initialize_logger(logger);
void initialize_logger(logger_shr_ptr new_logger);

// 注册 new_logger，完成注册后 new_logger 可通过名称（logger_name）被访问
void register_logger(logger_shr_ptr new_logger);

// 设置 learnlog::initialize_logger() 后是否自动执行 learnlog::register_logger()
void set_auto_register_logger(bool flag);

// 通过名称（logger_name）获取 logger 对象的 shared_ptr，找不到时返回 nullptr
// example:
//  auto logger = learnlog::get_logger("logger_name");
logger_shr_ptr get_logger(const std::string& logger_name);

// 通过名称（logger_name）移除 logger 对象，对象不存在时无操作，不抛出异常
void remove_logger(const std::string& logger_name);

// 获取默认 logger 的 shared_ptr，如果不存在则返回 nullptr，
// 默认 logger 对象的名称（logger_name）是 ""，sink 是 learnlog::sinks::stdout_color_sink_mt
// example:
//  auto default_logger = learnlog::get_default_logger();
logger_shr_ptr get_default_logger();

// 设置默认 logger
// example:
//  auto sink = std::make_shared<learnlog::sinks::basic_file_sink_mt>("filename");
//  auto new_default_logger = std::make_shared<learnlog::logger>("logger_name", sink);
//  learnlog::set_default_logger(new_default_logger);
void set_default_logger(logger_shr_ptr new_default_logger);

// 设置用于格式化日志信息的模板，该模板将应用于所有已注册的 logger，
// 新注册的 logger 也默认使用该模板，
// 模板转义字符的说明位于 sinks/pattern_formatter.cpp
// example:
//  learnlog::set_global_pattern("[%c] [%^%l%$] %v");
void set_global_pattern(std::string pattern);

// 设置用于格式化日志信息的格式化器（formatter），该 formatter 将应用于所有已注册的 logger,
// 新注册的 logger 也默认使用该格式化器
void set_global_formatter(formatter_uni_ptr new_formatter);

// 设置指定 logger 的格式模板，模板转义字符的说明位于 sinks/pattern_formatter.cpp
// example:
//  learnlog::set_pattern("logger_name", "[%c] [%^%l%$] %v");
void set_pattern(const std::string& logger_name, std::string pattern);

// 设置全局日志记录级别（log_level），该级别将应用于所有已注册的 logger,
// 新注册的 logger 也默认使用该级别，
// log_level 决定了日志被输出的最低级别，高于该级别的日志才会被输出，
// 日志级别的定义位于 definitions.h
// example:
//  learnlog::set_global_log_level(learnlog::level::debug);
void set_global_log_level(level::level_enum log_level);

// 设置指定 logger 的日志记录级别（log_level），日志级别的定义位于 definitions.h
// example:
//  learnlog::set_log_level("logger_name", learnlog::level::debug);
void set_log_level(const std::string& logger_name, level::level_enum log_level);

// 设置全局缓冲区刷新级别（flush_level），该级别将应用于所有已注册的 logger,
// 新注册的 logger 也默认使用该级别，
// flush_level 控制缓冲区的强制刷新，高于该级别的日志在输出后立即刷新所在缓冲区，
// 日志级别的定义位于 definitions.h
// example:
//  learnlog::set_global_flush_level(learnlog::level::error);
void set_global_flush_level(level::level_enum flush_level);

// 设置指定 logger 的缓冲区刷新级别（flush_level），日志级别的定义位于 definitions.h
// example:
//  learnlog::set_flush_level("logger_name", learnlog::level::error);
void set_flush_level(const std::string& logger_name, level::level_enum flush_level);

// 对所有已注册的 logger 执行函数 func
// example:
//  int logger_cnt = 0;
//  auto func = [&logger_cnt](learnlog::logger_shr_ptr) { logger_cnt++; };
//  learnlog::exec_all(func);
void exec_all(const std::function<void(logger_shr_ptr)>& func);

// 对所有已注册的 logger 刷新输出缓冲区
void flush_all();

// 移除所有已注册的 logger
void remove_all();

// 初始化一个新的线程池，
// msg_queue_size 是日志消息缓冲区的大小，thread_num 是后台处理日志的线程数量，
// 完成后自动调用 register_thread_pool() 注册
template <typename Threadpool>
void initialize_thread_pool(size_t msg_queue_size, size_t thread_num) {
    base::registry::instance().initialize_thread_pool<Threadpool>(msg_queue_size, 
                                                                  thread_num);
}

// 注册线程池
void register_thread_pool(thread_pool_shr_ptr new_thread_pool);

// 获取线程池的 shared_ptr
thread_pool_shr_ptr get_thread_pool();

// 关闭 learnlog，释放单例 registry 中的所有资源，一般不需要手动调用
void close();

// 每隔一段时间 interval，对所有已注册的 logger 刷新输出缓冲区
// example:
//  learnlog::flush_every(std::chrono::seconds(1));
template <typename Rep, typename Period>
void flush_every(std::chrono::duration<Rep, Period> interval) {
    base::registry::instance().flush_every(interval);
}

// 通过 factory 创建 logger 对象，创建时需要指定 logger 使用的 sink、
// logger 的名称以及 sink 的创建参数
// example:
//  learnlog::create<learnlog::sinks::basic_file_sink_mt>("logger_name", "filename");
// 等同于:
//  learnlog::basic_file_logger_mt("logger_name", "filename");
template <typename Sink, typename... SinkArgs>
logger_shr_ptr create(std::string logger_name, SinkArgs &&...sink_args) {
    return sync_factory::create<Sink>(std::move(logger_name), 
                                         std::forward<SinkArgs>(sink_args)...);
}

// 通过 async_factory_lock 创建线程池为 lock_thread_pool 的 async_logger 对象，
// 创建时需要指定 logger 使用的 sink、logger 的名称以及 sink 的创建参数
// example:
//  learnlog::create_async_lock<learnlog::sinks::basic_file_sink_mt>("logger_name",
//                                                                   "filename");
// 等同于:
//  learnlog::basic_file_logger_mt<learnlog::async_factory>("logger_name", 
//                                                          "filename");
template <typename Sink, typename... SinkArgs>
async_logger_shr_ptr create_async_lock(std::string logger_name, SinkArgs &&...sink_args) {
    return async_factory_lock::create<Sink>(std::move(logger_name), 
                                            std::forward<SinkArgs>(sink_args)...);
}

// 通过 async_factory 创建线程池为 lockfree_thread_pool 的 async_logger 对象，
// 创建时需要指定 logger 使用的 sink、logger 的名称以及 sink 的创建参数
// example:
//  learnlog::create_async_lockfree<learnlog::sinks::basic_file_sink_mt>("logger_name", 
//                                                                       "filename");
// 等同于:
//  learnlog::basic_file_logger_mt<learnlog::async_factory_lockfree>("logger_name", 
//                                                                   "filename");
template <typename Sink, typename... SinkArgs>
async_logger_shr_ptr create_async_lockfree(std::string logger_name, 
                                           SinkArgs &&...sink_args) {
    return async_factory_lockfree::create<Sink>(std::move(logger_name), 
                                                std::forward<SinkArgs>(sink_args)...);
}

// 通过 async_factory 创建线程池为 lockfree_concurrent_thread_pool 的 async_logger 对象，
// 创建时需要指定 logger 使用的 sink、logger 的名称以及 sink 的创建参数
// example:
//  learnlog::create_async_lockfree_concurrent<learnlog::sinks::basic_file_sink_mt>("logger_name", 
//                                                                                  "filename");
// 等同于:
//  learnlog::basic_file_logger_mt<learnlog::async_factory_lockfree_concurrent>("logger_name", 
//                                                                              "filename");
template <typename Sink, typename... SinkArgs>
async_logger_shr_ptr create_async_lockfree_concurrent(std::string logger_name, 
                                                      SinkArgs &&...sink_args) {
    return 
        async_factory_lockfree_concurrent::create<Sink>(std::move(logger_name), 
                                                        std::forward<SinkArgs>(sink_args)...);
}

/* 以下是调用默认 logger 记录日志的接口，支持多种输入 */

template <typename... Args>
void log(source_loc loc, 
         level::level_enum level, 
         fmt_format_string<Args...> fmt_fstr, 
         Args &&...args) {
    learnlog::get_default_logger()->log(loc, level, fmt_fstr, std::forward<Args>(args)...);
}

template <typename... Args>
void log(level::level_enum level, fmt_format_string<Args...> fmt_fstr, Args &&...args) {
    learnlog::get_default_logger()->log(level, fmt_fstr, std::forward<Args>(args)...);
}

template <typename T,
              typename std::enable_if<!is_convertible_to_basic_format_string<const T &>::value,
                                      int>::type = 0>
void log(source_loc loc, level::level_enum lvl, const T &msg) {
    learnlog::get_default_logger()->log(loc, lvl, msg);
}

template <typename T>
void log(level::level_enum level, const T& msg) {
    learnlog::get_default_logger()->log(level, msg);
}

template <typename... Args>
void trace(fmt_format_string<Args...> fmt_fstr, Args &&...args) {
    learnlog::get_default_logger()->trace(fmt_fstr, std::forward<Args>(args)...);
}

template <typename... Args>
void debug(fmt_format_string<Args...> fmt_fstr, Args &&...args) {
    learnlog::get_default_logger()->debug(fmt_fstr, std::forward<Args>(args)...);
}

template <typename... Args>
void info(fmt_format_string<Args...> fmt_fstr, Args &&...args) {
    learnlog::get_default_logger()->info(fmt_fstr, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(fmt_format_string<Args...> fmt_fstr, Args &&...args) {
    learnlog::get_default_logger()->warn(fmt_fstr, std::forward<Args>(args)...);
}

template <typename... Args>
void error(fmt_format_string<Args...> fmt_fstr, Args &&...args) {
    learnlog::get_default_logger()->error(fmt_fstr, std::forward<Args>(args)...);
}

template <typename... Args>
void critical(fmt_format_string<Args...> fmt_fstr, Args &&...args) {
    learnlog::get_default_logger()->critical(fmt_fstr, std::forward<Args>(args)...);
}

template <typename T>
void trace(const T& msg) {
    learnlog::get_default_logger()->trace(msg);
}

template <typename T>
void debug(const T& msg) {
    learnlog::get_default_logger()->debug(msg);
}

template <typename T>
void info(const T& msg) {
    learnlog::get_default_logger()->info(msg);
}

template <typename T>
void warn(const T& msg) {
    learnlog::get_default_logger()->warn(msg);
}

template <typename T>
void error(const T& msg) {
    learnlog::get_default_logger()->error(msg);
}

template <typename T>
void critical(const T& msg) {
    learnlog::get_default_logger()->critical(msg);
}

}   // namespace learnlog