#include <catch2/catch_all.hpp>
#include "learnlog.h"
#include "base/exception.h"
#include "sinks/null_sink.h"
#include "sinks/std_sinks.h"
#include "test_sink.h"

#define LOGGER_NAME "test_interface_logger"
#define LOGGER_NAME_2 "test_interface_logger_2"
#define DEFAULT_LOG_LEVEL learnlog::level::info

TEST_CASE("default_logger", "[interface]") {
#ifndef _WIN32
    auto default_logger = learnlog::get_default_logger();
    REQUIRE(default_logger->get_log_level() == DEFAULT_LOG_LEVEL);
    default_logger->info("message from default logger");
#endif

    auto sink = std::make_shared<learnlog::sinks::null_sink_mt>();
    auto new_logger = std::make_shared<learnlog::logger>(LOGGER_NAME, sink);
    learnlog::set_default_logger(new_logger);
    REQUIRE(learnlog::get_default_logger() == learnlog::get_logger(LOGGER_NAME));
    new_logger->info("message will not be displayed");
}

TEST_CASE("create_logger", "[interface]") {
    learnlog::remove_all();
    
    learnlog::create<learnlog::sinks::null_sink_st>(LOGGER_NAME);
    REQUIRE(learnlog::get_logger(LOGGER_NAME) != nullptr);
    
    // throw exception
    learnlog::create<learnlog::sinks::null_sink_st>(LOGGER_NAME);
}

TEST_CASE("register_logger", "[interface]") {
    learnlog::remove_all();
    
    auto sink_mt = std::make_shared<learnlog::sinks::null_sink_mt>();
    auto new_logger_mt = std::make_shared<learnlog::logger>(LOGGER_NAME, sink_mt);
    learnlog::initialize_logger(new_logger_mt);
    REQUIRE(learnlog::get_logger(LOGGER_NAME) != nullptr);

    learnlog::set_auto_register_logger(false);
    learnlog::level::level_enum log_level = learnlog::level::debug;
    learnlog::set_global_log_level(log_level);
    auto sink_st = std::make_shared<learnlog::sinks::null_sink_st>();
    auto new_logger_st = std::make_shared<learnlog::logger>(LOGGER_NAME_2, sink_st);
    
    // 没有被 register 注册，但是仍然按照全局设置初始化
    learnlog::initialize_logger(new_logger_st);
    REQUIRE(learnlog::get_logger(LOGGER_NAME_2) == nullptr);
    REQUIRE(new_logger_st->get_log_level() == log_level);
    learnlog::register_logger(new_logger_st);
    REQUIRE(learnlog::get_logger(LOGGER_NAME_2) != nullptr);

    learnlog::set_auto_register_logger(true);
    learnlog::set_global_log_level(DEFAULT_LOG_LEVEL);
}

TEST_CASE("remove_logger", "[interface]") {
    learnlog::remove_all();

    learnlog::create<learnlog::sinks::null_sink_mt>(LOGGER_NAME);
    learnlog::remove_logger(LOGGER_NAME);
    REQUIRE(learnlog::get_logger(LOGGER_NAME) == nullptr);

    auto sink_st = std::make_shared<learnlog::sinks::null_sink_st>();
    auto new_logger_st = std::make_shared<learnlog::logger>(LOGGER_NAME, sink_st);
    learnlog::set_default_logger(new_logger_st);
    learnlog::remove_logger(LOGGER_NAME);
    REQUIRE(learnlog::get_logger(LOGGER_NAME) == nullptr);
    REQUIRE(learnlog::get_default_logger() == nullptr);

    learnlog::create<learnlog::sinks::null_sink_mt>(LOGGER_NAME);
    learnlog::remove_logger(LOGGER_NAME_2);    // 删除未注册的 logger 时不会有操作
    REQUIRE(learnlog::get_logger(LOGGER_NAME) != nullptr);
    learnlog::create<learnlog::sinks::null_sink_st>(LOGGER_NAME_2);
    learnlog::remove_all();
    REQUIRE(learnlog::get_logger(LOGGER_NAME) == nullptr);
    REQUIRE(learnlog::get_logger(LOGGER_NAME_2) == nullptr);
}

TEST_CASE("patterns", "[interface]") {
    learnlog::remove_all();

    learnlog::set_global_pattern("%n [%v]");
    learnlog::create<learnlog::sinks::null_sink_mt>(LOGGER_NAME);
    learnlog::create<learnlog::sinks::null_sink_st>(LOGGER_NAME_2);
    REQUIRE(learnlog::get_logger(LOGGER_NAME)->get_pattern() == "%n [%v]");
    REQUIRE(learnlog::get_logger(LOGGER_NAME_2)->get_pattern() == "%n [%v]");

    learnlog::set_pattern(LOGGER_NAME, "%n <%v>");
    learnlog::set_pattern(LOGGER_NAME_2, "%n {%v}");
    REQUIRE(learnlog::get_logger(LOGGER_NAME)->get_pattern() == "%n <%v>");
    REQUIRE(learnlog::get_logger(LOGGER_NAME_2)->get_pattern() == "%n {%v}");

    learnlog::remove_logger(LOGGER_NAME_2);
    // throw exception
    learnlog::set_pattern(LOGGER_NAME_2, "%v");
}

TEST_CASE("levels", "[interface]") {
    learnlog::remove_all();

    learnlog::set_global_log_level(learnlog::level::trace);
    learnlog::set_global_flush_level(learnlog::level::trace);

    auto sink = std::make_shared<learnlog::sinks::null_sink_mt>();
    auto new_logger = std::make_shared<learnlog::logger>(LOGGER_NAME, sink);
    learnlog::initialize_logger(new_logger);
    REQUIRE(new_logger->get_log_level() == learnlog::level::trace);
    REQUIRE(new_logger->get_flush_level() == learnlog::level::trace);

    learnlog::set_log_level(LOGGER_NAME, learnlog::level::warn);
    learnlog::set_flush_level(LOGGER_NAME, learnlog::level::warn);
    REQUIRE(new_logger->get_log_level() == learnlog::level::warn);
    REQUIRE(new_logger->get_flush_level() == learnlog::level::warn);
    
    // throw exception
    learnlog::set_log_level(LOGGER_NAME_2, learnlog::level::error);
    // throw exception
    learnlog::set_flush_level(LOGGER_NAME_2, learnlog::level::error);
}

TEST_CASE("execute_all", "[interface]") {
    learnlog::remove_all();

    learnlog::create<learnlog::sinks::null_sink_mt>(LOGGER_NAME);
    learnlog::create<learnlog::sinks::null_sink_st>(LOGGER_NAME_2);
    int logger_cnt = 0;
    auto func = [&logger_cnt](learnlog::logger_shr_ptr) { logger_cnt++; };
    learnlog::exec_all(func);
    REQUIRE(logger_cnt == 2);
}

TEST_CASE("log", "[interface]") {
    learnlog::remove_all();

    auto test_sink = std::make_shared<learnlog::sinks::test_sink_st>();
    test_sink->set_sink_delay_ms(1);
    auto logger = std::make_shared<learnlog::logger>(LOGGER_NAME, test_sink);
    learnlog::set_default_logger(logger);

    learnlog::log(learnlog::level::info, 3.14);
    learnlog::get_logger(LOGGER_NAME)->info("message {}", 3.14);
    REQUIRE(test_sink->msg_count() == 2);

    learnlog::remove_all();
}