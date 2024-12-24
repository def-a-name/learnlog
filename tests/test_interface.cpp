#include <catch2/catch_all.hpp>
#include "mylog.h"
#include "base/exception.h"
#include "sinks/null_sink.h"
#include "sinks/std_sinks.h"
#include "test_sink.h"

#define LOGGER_NAME "test_interface_logger"
#define LOGGER_NAME_2 "test_interface_logger_2"
#define DEFAULT_LOG_LEVEL mylog::level::info

TEST_CASE("default_logger", "[interface]") {
    auto default_logger = mylog::get_default_logger();
    REQUIRE(default_logger->get_log_level() == DEFAULT_LOG_LEVEL);
    default_logger->info("message from default logger");

    auto sink = std::make_shared<mylog::sinks::null_sink_mt>();
    auto new_logger = std::make_shared<mylog::logger>(LOGGER_NAME, sink);
    mylog::set_default_logger(new_logger);
    REQUIRE(mylog::get_default_logger() == mylog::get_logger(LOGGER_NAME));
    new_logger->info("message will not be displayed");
}

TEST_CASE("create_logger", "[interface]") {
    mylog::remove_all();
    
    mylog::create<mylog::sinks::null_sink_st>(LOGGER_NAME);
    REQUIRE(mylog::get_logger(LOGGER_NAME) != nullptr);
    
    // catch exception 1
    mylog::create<mylog::sinks::null_sink_st>(LOGGER_NAME);
}

TEST_CASE("register_logger", "[interface]") {
    mylog::remove_all();
    
    auto sink_mt = std::make_shared<mylog::sinks::null_sink_mt>();
    auto new_logger_mt = std::make_shared<mylog::logger>(LOGGER_NAME, sink_mt);
    mylog::initialize_logger(new_logger_mt);
    REQUIRE(mylog::get_logger(LOGGER_NAME) != nullptr);

    mylog::set_auto_register_logger(false);
    mylog::level::level_enum log_level = mylog::level::debug;
    mylog::set_global_log_level(log_level);
    auto sink_st = std::make_shared<mylog::sinks::null_sink_st>();
    auto new_logger_st = std::make_shared<mylog::logger>(LOGGER_NAME_2, sink_st);
    
    // 没有被 register 注册，但是仍然按照全局设置初始化
    mylog::initialize_logger(new_logger_st);
    REQUIRE(mylog::get_logger(LOGGER_NAME_2) == nullptr);
    REQUIRE(new_logger_st->get_log_level() == log_level);
    mylog::register_logger(new_logger_st);
    REQUIRE(mylog::get_logger(LOGGER_NAME_2) != nullptr);

    mylog::set_auto_register_logger(true);
    mylog::set_global_log_level(DEFAULT_LOG_LEVEL);
}

TEST_CASE("remove_logger", "[interface]") {
    mylog::remove_all();

    mylog::create<mylog::sinks::null_sink_mt>(LOGGER_NAME);
    mylog::remove_logger(LOGGER_NAME);
    REQUIRE(mylog::get_logger(LOGGER_NAME) == nullptr);

    auto sink_st = std::make_shared<mylog::sinks::null_sink_st>();
    auto new_logger_st = std::make_shared<mylog::logger>(LOGGER_NAME, sink_st);
    mylog::set_default_logger(new_logger_st);
    mylog::remove_logger(LOGGER_NAME);
    REQUIRE(mylog::get_logger(LOGGER_NAME) == nullptr);
    REQUIRE(mylog::get_default_logger() == nullptr);

    mylog::create<mylog::sinks::null_sink_mt>(LOGGER_NAME);
    mylog::remove_logger(LOGGER_NAME_2);    // 删除未注册的 logger 时不会有操作
    REQUIRE(mylog::get_logger(LOGGER_NAME) != nullptr);
    mylog::create<mylog::sinks::null_sink_st>(LOGGER_NAME_2);
    mylog::remove_all();
    REQUIRE(mylog::get_logger(LOGGER_NAME) == nullptr);
    REQUIRE(mylog::get_logger(LOGGER_NAME_2) == nullptr);
}

TEST_CASE("patterns", "[interface]") {
    mylog::remove_all();

    mylog::set_global_pattern("%n [%v]");
    mylog::create<mylog::sinks::null_sink_mt>(LOGGER_NAME);
    mylog::create<mylog::sinks::null_sink_st>(LOGGER_NAME_2);
    REQUIRE(mylog::get_logger(LOGGER_NAME)->get_pattern() == "%n [%v]");
    REQUIRE(mylog::get_logger(LOGGER_NAME_2)->get_pattern() == "%n [%v]");

    mylog::set_pattern(LOGGER_NAME, "%n <%v>");
    mylog::set_pattern(LOGGER_NAME_2, "%n {%v}");
    REQUIRE(mylog::get_logger(LOGGER_NAME)->get_pattern() == "%n <%v>");
    REQUIRE(mylog::get_logger(LOGGER_NAME_2)->get_pattern() == "%n {%v}");

    mylog::remove_logger(LOGGER_NAME_2);
    // catch exception 4
    mylog::set_pattern(LOGGER_NAME_2, "%v");
}

TEST_CASE("levels", "[interface]") {
    mylog::remove_all();

    mylog::set_global_log_level(mylog::level::trace);
    mylog::set_global_flush_level(mylog::level::trace);

    auto sink = std::make_shared<mylog::sinks::null_sink_mt>();
    auto new_logger = std::make_shared<mylog::logger>(LOGGER_NAME, sink);
    mylog::initialize_logger(new_logger);
    REQUIRE(new_logger->get_log_level() == mylog::level::trace);
    REQUIRE(new_logger->get_flush_level() == mylog::level::trace);

    mylog::set_log_level(LOGGER_NAME, mylog::level::warn);
    mylog::set_flush_level(LOGGER_NAME, mylog::level::warn);
    REQUIRE(new_logger->get_log_level() == mylog::level::warn);
    REQUIRE(new_logger->get_flush_level() == mylog::level::warn);
    
    // catch exception 2
    mylog::set_log_level(LOGGER_NAME_2, mylog::level::error);
    // catch exception 3
    mylog::set_flush_level(LOGGER_NAME_2, mylog::level::error);
}

TEST_CASE("execute_all", "[interface]") {
    mylog::remove_all();

    mylog::create<mylog::sinks::null_sink_mt>(LOGGER_NAME);
    mylog::create<mylog::sinks::null_sink_st>(LOGGER_NAME_2);
    int logger_cnt = 0;
    auto func = [&logger_cnt](mylog::logger_shr_ptr) { logger_cnt++; };
    mylog::exec_all(func);
    REQUIRE(logger_cnt == 2);
}

TEST_CASE("log", "[interface]") {
    mylog::remove_all();

    auto test_sink = std::make_shared<mylog::sinks::test_sink_st>();
    test_sink->set_sink_delay_ms(1);
    auto logger = std::make_shared<mylog::logger>(LOGGER_NAME, test_sink);
    mylog::set_default_logger(logger);

    mylog::log(mylog::level::info, 3.14);
    mylog::get_logger(LOGGER_NAME)->info("message {}", 3.14);
    REQUIRE(test_sink->msg_count() == 2);

    mylog::remove_all();
}