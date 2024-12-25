#include <catch2/catch_all.hpp>
#include "learnlog.h"
#include "logger.h"
#include "async_logger.h"
#include "sinks/ostream_sink.h"

TEST_CASE("backtrace_default", "[backtracer]") {
    std::ostringstream oss;
    auto oss_sink = std::make_shared<learnlog::sinks::ostream_sink_st>(oss);
    auto oss_logger = std::make_shared<learnlog::logger>("backtrace test logger", oss_sink);
    oss_logger->set_log_level(learnlog::level::info);
    oss_logger->set_pattern("%v");
    
    for (int i = 0; i < 100; i++) {
        oss_logger->debug("debug {}", i);
    }
    REQUIRE(oss.str().empty());
    oss_logger->dump_backtrace();
    REQUIRE(oss.str().empty());
}

TEST_CASE("backtrace_sync", "[backtracer]") {
    std::ostringstream oss;
    auto oss_sink = std::make_shared<learnlog::sinks::ostream_sink_st>(oss);
    auto oss_logger = std::make_shared<learnlog::logger>("backtrace test logger", oss_sink);
    oss_logger->set_log_level(learnlog::level::info);
    oss_logger->set_pattern("%v");
    oss_logger->enable_backtrace_n(5);
    
    for (int i = 0; i < 100; i++) {
        oss_logger->debug("debug {}", i);
    }
    REQUIRE(oss.str().empty());
    oss_logger->dump_backtrace();
    std::string expected = 
        "*********************** Backtrace Start ***********************" DEFAULT_EOL
        "debug 95" DEFAULT_EOL
        "debug 96" DEFAULT_EOL
        "debug 97" DEFAULT_EOL
        "debug 98" DEFAULT_EOL
        "debug 99" DEFAULT_EOL
        "************************ Backtrace End ************************" DEFAULT_EOL;   
    REQUIRE(oss.str() == expected);

    oss.str("");
    oss_logger->disable_backtrace();
    oss_logger->debug("debug 100");
    oss_logger->dump_backtrace();
    REQUIRE(oss.str().empty());

    oss_logger->enable_backtrace_n(5);
    oss_logger->info("debug 101");
    oss_logger->dump_backtrace();
    expected = 
        "debug 101" DEFAULT_EOL
        "*********************** Backtrace Start ***********************" DEFAULT_EOL
        "debug 101" DEFAULT_EOL
        "************************ Backtrace End ************************" DEFAULT_EOL;
    REQUIRE(oss.str() == expected);
}

TEST_CASE("backtrace_async", "[backtracer]") {
    learnlog::initialize_thread_pool(1024, 1);
    std::weak_ptr<learnlog::base::thread_pool> tp(learnlog::get_thread_pool());

    std::ostringstream oss;
    auto oss_sink = std::make_shared<learnlog::sinks::ostream_sink_st>(oss);
    auto oss_async_logger = std::make_shared<learnlog::async_logger>("backtrace test logger",
                                                                  oss_sink,
                                                                  tp);
    oss_async_logger->set_log_level(learnlog::level::info);
    oss_async_logger->set_pattern("%v");
    oss_async_logger->enable_backtrace_n(3);
    
    for (int i = 0; i < 1000; i++) {
        oss_async_logger->debug("debug {}", i);
    }
    learnlog::base::os::sleep_for_ms(100);

    REQUIRE(oss.str().empty());
    oss_async_logger->dump_backtrace();
    std::string expected = 
        "*********************** Backtrace Start ***********************" DEFAULT_EOL
        "debug 997" DEFAULT_EOL
        "debug 998" DEFAULT_EOL
        "debug 999" DEFAULT_EOL
        "************************ Backtrace End ************************" DEFAULT_EOL;
    learnlog::base::os::sleep_for_ms(100);
    REQUIRE(oss.str() == expected);
}