#include "learnlog_example.h"

#include "base/exception.h"
#include "base/thread_pool.h"
#include "logger.h"
#include "sinks/formatters/pattern_formatter.h"
#include "sinks/std_sinks.h"
#include "sinks/std_color_sinks.h"
#include "sinks/basic_file_sink.h"
#include "sinks/rolling_file_sink.h"
#include "learnlog.h"

void learnlog::handle_exception(const std::string& msg) {
    learnlog::source_loc s;
    try {
        throw std::runtime_error(msg);
    }
    LEARNLOG_CATCH
}

void learnlog::filler_helloworld(fmt_memory_buf& dest_buf) {
    learnlog::sinks::spaces_info s_info(6, learnlog::sinks::spaces_info::fill_side::center, true);
    learnlog::sinks::filler filler(4, s_info, dest_buf);
    filler.fill_msg(2024);
}

void learnlog::pattern_formatter_helloworld(fmt_memory_buf& dest_buf) {
    learnlog::base::log_msg msg(learnlog::level::level_enum::info, "hello world", "logger01");
    std::unique_ptr<learnlog::sinks::formatter> p = learnlog::make_unique<learnlog::sinks::pattern_formatter>();
    p->format(msg, dest_buf);
}

void learnlog::custom_formatter_hello(fmt_memory_buf& dest_buf) {
    std::unique_ptr<learnlog::sinks::pattern_formatter> p = 
        learnlog::make_unique<learnlog::sinks::pattern_formatter>("");
    fmt_string_view str_view{"你好，世界！"};
    learnlog::base::log_msg msg(learnlog::level::level_enum::info, str_view, "test_logger");

    p->add_custom_flag<custom_formatter_test>('#');
    p->set_pattern("%#");
    p->format(msg, dest_buf);
}

void learnlog::print_fmt_buf(fmt_memory_buf& buf) {
    fmt_string_view str_view(buf.data(), buf.size());
    fmt::print("{}", str_view);
}

void learnlog::logger_helloworld() { 
    auto stdout_sink = std::make_shared<learnlog::sinks::stdout_sink_st>();
    auto stdout_color_sink = std::make_shared<learnlog::sinks::stdout_color_sink_st>();
#ifdef _WIN32
    filename_t fname(L"test_tmp/中文路径.log");
#else
    filename_t fname("test_tmp/basic_file.log");
#endif
    auto basic_file_sink = std::make_shared<learnlog::sinks::basic_file_sink_st>(fname);
    
    sinks_init_list sinks_list{stdout_sink, stdout_color_sink, basic_file_sink};
    logger example_logger("example logger", sinks_list);
    
    example_logger.set_log_level(learnlog::level::level_enum::trace);
    example_logger.set_pattern("你好是 %v");
    example_logger.info("hello");
    
    example_logger.set_log_level(learnlog::level::level_enum::trace);
    example_logger.set_pattern("%^world是%$ %^%v%$");
    example_logger.critical("世界");

    example_logger.set_pattern("%c hello, world! %v");
    example_logger.warn("你好，世界！");
}

void learnlog::async_helloworld() {
#ifdef _WIN32
    filename_t fname(L"test_tmp/异步日志.txt");
#else
    filename_t fname("test_tmp/异步日志.txt");
#endif
    learnlog::set_global_pattern("thread [%t]: <%T.%F> %v");
    size_t total_msg = base::default_queue_size * 2;
    size_t thread_cnt = 16;
    size_t msg_per_thread = total_msg / thread_cnt;
    std::vector<std::thread> threads;

    // learnlog::initialize_thread_pool<learnlog::base::lock_thread_pool>(32768,
    //                                                                    4);
    // auto async_logger = 
    //     learnlog::create_async<learnlog::sinks::rolling_file_sink_mt>("async_example", 
    //                                                                   fname,
    //                                                                   10 * 1024,
    //                                                                   10);
    // auto thread_func = [&async_logger, msg_per_thread] {
    //     for (size_t i = 0; i < msg_per_thread; i++) {
    //         async_logger->info("#{}", i);
    //     }
    // };
    // for (size_t i = 0; i < thread_cnt; i++) {
    //     threads.emplace_back(thread_func);
    // }
    // for (auto& t : threads) {
    //     t.join();
    // }
   
    // threads.clear();
    // learnlog::initialize_thread_pool<learnlog::base::lockfree_thread_pool>(32768,
    //                                                                        4);
    // async_logger = learnlog::create_async_lockfree<learnlog::sinks::rolling_file_sink_mt>(
    //     "async_lockfree_example", 
    //     fname,
    //     10 * 1024,
    //     20
    // );
    // for (size_t i = 0; i < thread_cnt; i++) {
    //     threads.emplace_back(thread_func);
    // }
    // for (auto& t : threads) {
    //     t.join();
    // }

    // threads.clear();
    // learnlog::initialize_thread_pool<learnlog::base::lockfree_concurrent_thread_pool>(32768,
    //                                                                                   4);
    // auto async_logger = learnlog::create_async_lockfree_concurrent<learnlog::sinks::rolling_file_sink_mt>(
    //     "async_lockfree_concurrent_example", 
    //     fname,
    //     10 * 1024,
    //     30
    // );
    auto thread_pool = std::make_shared<learnlog::base::lockfree_concurrent_thread_pool>(32768, 1);
    // learnlog::initialize_thread_pool<learnlog::base::lockfree_concurrent_thread_pool>(32768, 1);
    // auto thread_pool = learnlog::get_thread_pool();
    auto sink = std::make_shared<learnlog::sinks::basic_file_sink_mt>(fname, true);
    auto logger = std::make_shared<async_logger>("async_lockfree_concurrent_example", 
                                                 std::move(sink),
                                                 std::move(thread_pool));
    logger->set_pattern("thread [%5t]: <%T.%F> %v");

    auto thread_func = [&logger, msg_per_thread] {
        for (size_t i = 0; i < msg_per_thread; i++) {
            logger->info("#{}", i);
        }
    };
    for (size_t i = 0; i < thread_cnt; i++) {
        threads.emplace_back(thread_func);
    }
    for (auto& t : threads) {
        t.join();
    }

    learnlog::flush_every(learnlog::milliseconds(500));
}