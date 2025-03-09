#include "learnlog.h"

void use_default_logger();
class custom_formatter;
void create_loggers();
void create_sinks();
void async_mode();

int main(int, char* [])
{ 
    // 使用默认 logger 记录日志
    use_default_logger();

    // 新建 logger 并记录日志
    create_loggers();

    // 建立 sink 设定日志记录位置
    create_sinks();

    // 异步模式
    async_mode();

    return 0;
}

void use_default_logger() {
    learnlog::info("{:20s}learnlog v{}.{}.{}\n", "",
                                                 LEARNLOG_VERSION_MAJOR,
                                                 LEARNLOG_VERSION_MINOR,
                                                 LEARNLOG_VERSION_PATCH);
    learnlog::warn("hello world");
    learnlog::error("{1} {0}, {2}", "world", "hello", 2025);
    learnlog::log(learnlog::level::critical, "你好世界");
    
    // 日志等级
    learnlog::debug("This message is not displayed because default log level is 'info'");
    learnlog::set_global_log_level(learnlog::level::trace);
    learnlog::debug("This message can be displayed now");
    learnlog::trace("This message is also displayed");

    // 日志格式
    learnlog::set_global_pattern("[%T] [%l] %^%v%$");
    learnlog::info("Reference of escape characters can be found in "
                      "sinks/formatters/pattern_formatter.cpp");
    
    learnlog::trace("Custom formatter is supported.");
    std::unique_ptr<learnlog::sinks::pattern_formatter> formatter = 
        learnlog::make_unique<learnlog::sinks::pattern_formatter>("[%T] [%l] %^%#%$");
    formatter->add_custom_flag<custom_formatter>('#');
    learnlog::set_global_formatter(std::move(formatter));
    learnlog::debug("前六个字符会被输出");
    
    learnlog::set_global_pattern("%+");
}

class custom_formatter : public learnlog::sinks::custom_flag_formatter {
public:
    void format(const learnlog::base::log_msg& msg, 
                const std::tm&,
                learnlog::fmt_memory_buf& dest_buf) override {
        learnlog::sinks::filler f(6, spaces_info_, dest_buf);
        f.fill_msg(learnlog::fmt_string_view{msg.msg.begin(), 6});
    }
    std::unique_ptr<custom_flag_formatter> clone() const override {
        return learnlog::make_unique<custom_formatter>();
    }
};

#include "sinks/std_color_sinks.h"
void create_loggers() {
    auto sink = std::make_shared<learnlog::sinks::stdout_color_sink_st>();
    auto logger_1 = std::make_shared<learnlog::logger>("logger1", sink);
    logger_1->info("Method 1");
    learnlog::initialize_logger(logger_1);

    auto logger_2 = learnlog::create<learnlog::sinks::stdout_color_sink_st>("logger2");
    logger_2->info("Method 2");

    auto logger_3 = learnlog::stdout_color_logger_st("logger3");
    logger_3->info("Method 3");

    // 在单例中统一管理 logger
    learnlog::set_global_pattern("\"%n\" %v");
    auto func = [](learnlog::logger_shr_ptr l) {
        l->info("is registered");
    };
    learnlog::exec_all(func);
    
    learnlog::set_log_level("default_logger", learnlog::level::warn);
    learnlog::set_pattern("default_logger", "%^<%n>%$: %v");
    auto logger_4 = learnlog::get_logger("default_logger");
    learnlog::remove_logger("default_logger");
    logger_4->enable_backtrace_n(3);
    logger_4->info("This message is not displayed");
    logger_4->warn("is unregistered");
    
    learnlog::exec_all([](learnlog::logger_shr_ptr l) {
        l->info("is still registered");
    });

    logger_4->dump_backtrace();

    learnlog::flush_every(learnlog::seconds(1));
}

#include "sinks/std_sinks.h"
#include "sinks/ostream_sink.h"
#include "sinks/basic_file_sink.h"
#include "sinks/rolling_file_sink.h"
#include <sstream>
void create_sinks() {
    // 单线程使用
    std::vector<learnlog::sink_shr_ptr> sinks_st;
    std::ostringstream oss;
    sinks_st.emplace_back(std::make_shared<learnlog::sinks::ostream_sink_st>(oss));
    sinks_st.emplace_back(std::make_shared<learnlog::sinks::stdout_sink_st>());
    sinks_st.emplace_back(std::make_shared<learnlog::sinks::stderr_sink_st>());
    sinks_st.emplace_back(std::make_shared<learnlog::sinks::stdout_color_sink_st>());
    sinks_st.emplace_back(std::make_shared<learnlog::sinks::stderr_color_sink_st>());
#ifdef _WIN32
    learnlog::filename_t fname_1 = L"example_tmp/basic_file.log";
    learnlog::filename_t fname_2 = L"example_tmp/滚动文件.log";
#else
    learnlog::filename_t fname_1 = "example_tmp/basic_file.log";
    learnlog::filename_t fname_2 = "example_tmp/滚动文件.log";
#endif
    sinks_st.emplace_back(std::make_shared<learnlog::sinks::basic_file_sink_st>(fname_1, true));
    sinks_st.emplace_back(std::make_shared<learnlog::sinks::rolling_file_sink_st>(fname_2, 1024, 3));
    auto logger_st = std::make_shared<learnlog::logger>("logger_st", sinks_st.begin(), sinks_st.end());
    logger_st->info("These sinks are avaliable in single-thread");

    // 多线程使用
    std::ostringstream oss_mt;
    auto sink_1 = std::make_shared<learnlog::sinks::ostream_sink_mt>(oss_mt);
    auto sink_2 = std::make_shared<learnlog::sinks::stdout_sink_mt>();
    auto sink_3 = std::make_shared<learnlog::sinks::stderr_sink_mt>();
    auto sink_4 = std::make_shared<learnlog::sinks::stdout_color_sink_mt>();
    sink_4->set_pattern("[%T] [tid: %^%t%$] %v");
    auto sink_5 = std::make_shared<learnlog::sinks::stderr_color_sink_mt>();
    auto sink_6 = std::make_shared<learnlog::sinks::basic_file_sink_mt>(fname_1, false);
    auto sink_7 = std::make_shared<learnlog::sinks::rolling_file_sink_mt>(fname_2, 1024, 3);
    learnlog::logger logger_mt("logger_mt", {sink_1, sink_2, sink_3, sink_4, sink_5, sink_6, sink_7});
    
    std::vector<std::thread> threads;
    for (size_t i = 0; i < 2; i++) {
        threads.emplace_back([&logger_mt]() {
            logger_mt.info("These sinks are avaliable in multi-threads");
        });
    }
    for (auto &t : threads) {
        t.join();
    }
}

#include "base/lock_thread_pool.h"
#include "base/lockfree_thread_pool.h"
#include "base/lockfree_concurrent_thread_pool.h"
void async_mode() {
#ifdef _WIN32
    learnlog::filename_t fname = L"example_tmp/async.log";
#else
    learnlog::filename_t fname = "example_tmp/async.log";
#endif
    learnlog::remove_all();
    learnlog::create_async_lock<learnlog::sinks::basic_file_sink_mt>("async_logger_1",
                                                                     fname,
                                                                     true);
    learnlog::create_async_lock<learnlog::sinks::basic_file_sink_mt>("async_logger_2",
                                                                     fname,
                                                                     false);
    learnlog::set_global_pattern("[%n] tid (%-6t): %v");
    learnlog::exec_all([](learnlog::logger_shr_ptr l) {
        for (size_t i = 0; i < 10; i++) {
            l->info("msg #{} is displayed by lock_thread_pool", i);
        }
    });
    
    // 单例中只有一个线程池，使用相同线程池的 async_logger 可以共享，
    // 但是如果要注册使用不同线程池的 async_logger，需要请求单例中所有 async_logger 在完成任务后销毁，然后
    // 重新创建并注册一个新的线程池
    auto async_logger_3 = 
        learnlog::create_async_lockfree<learnlog::sinks::basic_file_sink_mt>("async_logger_3",
                                                                             fname,
                                                                             false);
    std::atomic<int> msg_seq{0};
    auto func_3 = [&async_logger_3, &msg_seq] {
        async_logger_3->info("msg #{} is displayed by lockfree_thread_pool", 
                             msg_seq.fetch_add(1, std::memory_order_relaxed));
    };
    std::vector<std::thread> threads;
    for (size_t i = 0; i < 10; i++) {
        threads.emplace_back(func_3);
    }
    for (auto &t : threads) {
        t.join();
    }

    // 可以直接初始化新的线程池，单例中原本的线程池会在任务完成后销毁
    learnlog::initialize_thread_pool<learnlog::base::lockfree_concurrent_thread_pool>(8192, 10);
    auto async_logger_4 = 
        learnlog::create_async_lockfree_concurrent<learnlog::sinks::stdout_color_sink_mt>("async_logger_4");
    msg_seq.store(0, std::memory_order_relaxed);
    auto func_4 = [&async_logger_4, &msg_seq] {
        async_logger_4->info("msg #{} is displayed by lockfree_concurrent_thread_pool", 
                             msg_seq.fetch_add(1, std::memory_order_relaxed));
    };
    threads.clear();
    for (size_t i = 0; i < 10; i++) {
        threads.emplace_back(func_4);
    }
    for (auto &t : threads) {
        t.join();
    }
    async_logger_4->flush();
    async_logger_4->info("Messages have been written to '{}' asynchronously", 
                         learnlog::base::wstring_to_string(fname));

    learnlog::flush_every(learnlog::milliseconds(100));
}