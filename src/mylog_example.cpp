#include "mylog_example.h"

#include "base/exception.h"
#include "base/thread_pool.h"
#include "logger.h"
#include "sinks/formatters/pattern_formatter.h"
#include "sinks/std_sinks.h"
#include "sinks/std_color_sinks.h"
#include "sinks/basic_file_sink.h"
#include "sinks/rolling_file_sink.h"
#include "mylog.h"

void mylog::create_thread_pool() {
    mylog::base::thread_pool thread_pool(10, 10);
}

void mylog::handle_exception(const std::string& msg) {
    mylog::source_loc s;
    try {
        throw std::runtime_error(msg);
    }
    MYLOG_CATCH
}

void mylog::filler_helloworld(fmt_memory_buf& dest_buf) {
    mylog::sinks::spaces_info s_info(6, mylog::sinks::spaces_info::fill_side::center, true);
    mylog::sinks::filler filler(4, s_info, dest_buf);
    filler.fill_msg(2024);
}

void mylog::pattern_formatter_helloworld(fmt_memory_buf& dest_buf) {
    mylog::base::log_msg msg(mylog::level::level_enum::info, "hello world", "logger01");
    std::unique_ptr<mylog::sinks::formatter> p = mylog::make_unique<mylog::sinks::pattern_formatter>();
    p->format(msg, dest_buf);
}

void mylog::custom_formatter_hello(fmt_memory_buf& dest_buf) {
    std::unique_ptr<mylog::sinks::pattern_formatter> p = 
        mylog::make_unique<mylog::sinks::pattern_formatter>("");
    fmt_string_view str_view{"你好，世界！"};
    mylog::base::log_msg msg(mylog::level::level_enum::info, str_view, "test_logger");

    p->add_custom_flag<custom_formatter_test>('#');
    p->set_pattern("%#");
    p->format(msg, dest_buf);
}

void mylog::print_fmt_buf(fmt_memory_buf& buf) {
    fmt_string_view str_view(buf.data(), buf.size());
    fmt::print("{}", str_view);
}

void mylog::logger_helloworld() { 
    auto stdout_sink = std::make_shared<mylog::sinks::stdout_sink_st>();
    auto stdout_color_sink = std::make_shared<mylog::sinks::stdout_color_sink_st>();
#ifdef _WIN32
    filename_t fname(L"test_tmp/中文路径.log");
#else
    filename_t fname("test_tmp/basic_file.log");
#endif
    auto basic_file_sink = std::make_shared<mylog::sinks::basic_file_sink_st>(fname);
    
    sinks_init_list sinks_list{stdout_sink, stdout_color_sink, basic_file_sink};
    logger example_logger("example logger", sinks_list);
    
    example_logger.set_log_level(mylog::level::level_enum::trace);
    example_logger.set_pattern("你好是 %v");
    example_logger.info("hello");
    
    example_logger.set_log_level(mylog::level::level_enum::trace);
    example_logger.set_pattern("%^world是%$ %^%v%$");
    example_logger.critical("世界");

    example_logger.set_pattern("%c hello, world! %v");
    example_logger.warn("你好，世界！");
}

void mylog::async_helloworld() {
    mylog::create_async<mylog::sinks::rolling_file_sink_mt>("async_example", 
                                                            "test_tmp/async.txt",
                                                            5 * 1024,
                                                            10);
    mylog::set_pattern("async_example", "thread [%t]: %v");
    auto func = [] (logger_shr_ptr async_logger) {
        for (int i = 0; i < base::default_queue_size * 2; i++) {
            async_logger->info("hello world #{}", i);
        }
    };
    mylog::exec_all(func);
    mylog::flush_every(mylog::milliseconds(500));
}