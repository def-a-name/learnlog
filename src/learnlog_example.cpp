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

void learnlog::create_thread_pool() {
    learnlog::base::thread_pool thread_pool(10, 10);
}

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
    auto async_logger = 
        learnlog::create_async<learnlog::sinks::rolling_file_sink_mt>("async_example", 
                                                                      fname,
                                                                      5 * 1024,
                                                                      10);
    learnlog::set_pattern("async_example", "thread [%t]: %v");
    for (int i = 0; i < base::default_queue_size * 2; i++) {
        async_logger->info("hello world #{}", i);
    }
    learnlog::flush_every(learnlog::milliseconds(500));
}