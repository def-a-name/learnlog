#include <catch2/catch_all.hpp>
#include "async_logger.h"
#include "test_sink.h"
#include "sinks/std_sinks.h"
#include "learnlog.h"

using learnlog::async_overflow_method;

TEST_CASE("block_wait", "[async_logger]") {
    auto test_sink = std::make_shared<learnlog::sinks::test_sink_mt>();
    test_sink->set_sink_delay_ms(2);
    auto overflow_m = async_overflow_method::block_wait;
    size_t msg_queue_size = 128;
    size_t thread_num = 1;
    size_t msg_num = msg_queue_size * 2;
    size_t override_cnt = 0;
    size_t discard_cnt = 0;

    {
        auto tp = std::make_shared<learnlog::base::thread_pool>(msg_queue_size, 
                                                             thread_num);
        auto logger = std::make_shared<learnlog::async_logger>("block_wait logger",
                                                            test_sink,
                                                            tp,
                                                            overflow_m);
        for (int i = 0; i < msg_num; i++) {
            logger->info("message {}", i);
        }
        logger->flush();
        override_cnt = tp->override_count();
        discard_cnt = tp->discard_count();
    }

    REQUIRE(test_sink->msg_count() == msg_num);
    REQUIRE(test_sink->flush_count() == 1);
    REQUIRE(override_cnt == 0);
    REQUIRE(discard_cnt == 0);
}

TEST_CASE("override_old", "[async_logger]") {
    auto test_sink = std::make_shared<learnlog::sinks::test_sink_mt>();
    test_sink->set_sink_delay_ms(2);
    auto overflow_m = async_overflow_method::override_old;
    size_t msg_queue_size = 128;
    size_t thread_num = 1;
    size_t msg_num = msg_queue_size * 2;
    size_t override_cnt = 0;
    size_t discard_cnt = 0;

    {
        auto tp = std::make_shared<learnlog::base::thread_pool>(msg_queue_size, 
                                                             thread_num);
        auto logger = std::make_shared<learnlog::async_logger>("override_old logger",
                                                            test_sink,
                                                            tp,
                                                            overflow_m);
        for (int i = 0; i < msg_num; i++) {
            logger->info("message {}", i);
        }
        logger->flush();
        override_cnt = tp->override_count();
        discard_cnt = tp->discard_count();
    }

    REQUIRE(test_sink->msg_count() + override_cnt == msg_num);
    REQUIRE(test_sink->flush_count() == 1);
    REQUIRE(override_cnt > 0);
    REQUIRE(discard_cnt == 0);
}

TEST_CASE("discard_new", "[async_logger]") {
    auto test_sink = std::make_shared<learnlog::sinks::test_sink_mt>();
    test_sink->set_sink_delay_ms(2);
    auto overflow_m = async_overflow_method::discard_new;
    size_t msg_queue_size = 128;
    size_t thread_num = 1;
    size_t msg_num = msg_queue_size * 2;
    size_t override_cnt = 0;
    size_t discard_cnt = 0;

    {
        auto tp = std::make_shared<learnlog::base::thread_pool>(msg_queue_size, 
                                                             thread_num);
        auto logger = std::make_shared<learnlog::async_logger>("discard_new logger",
                                                            test_sink,
                                                            tp,
                                                            overflow_m);
        for (int i = 0; i < msg_num; i++) {
            logger->info("message {}", i);
        }
        logger->flush();
        override_cnt = tp->override_count();
        discard_cnt = tp->discard_count();
    }
    // flush 请求可能被丢弃
    REQUIRE(test_sink->msg_count() + discard_cnt + test_sink->flush_count() == 
            msg_num + 1);
    REQUIRE(override_cnt == 0);
    REQUIRE(discard_cnt > 0);
}

TEST_CASE("reset thread pool", "[async_logger]") {
    auto test_sink = std::make_shared<learnlog::sinks::test_sink_mt>();
    test_sink->set_sink_delay_ms(2);
    auto overflow_m = async_overflow_method::block_wait;
    size_t msg_queue_size = 128;
    size_t thread_num = 1;
    size_t msg_num = msg_queue_size * 2;

    auto tp = std::make_shared<learnlog::base::thread_pool>(msg_queue_size, 
                                                         thread_num);
    auto logger = std::make_shared<learnlog::async_logger>("reset tp",
                                                        test_sink,
                                                        tp,
                                                        overflow_m);
    for (int i = 0; i < msg_num; i++) {
        logger->info("message {}", i);
    }
    logger->flush();
    tp.reset();

    REQUIRE(test_sink->msg_count() == msg_num);
    REQUIRE(test_sink->flush_count() == 1);
}

TEST_CASE("invalid thread pool", "[async_logger]") {
    auto test_sink = std::make_shared<learnlog::sinks::test_sink_mt>();
    std::shared_ptr<learnlog::base::thread_pool> tp;
    auto logger = std::make_shared<learnlog::async_logger>("invalid tp",
                                                        test_sink,
                                                        tp);
    logger->info("message will not be logged");
    REQUIRE(test_sink->msg_count() == 0);
}

TEST_CASE("create_from_registry", "[interface]") {
    learnlog::remove_all();
    size_t msg_q_size = 128;
    size_t thread_num = 1;
    learnlog::initialize_thread_pool(msg_q_size, thread_num);
    learnlog::set_global_pattern("[%n] %v");
    
    learnlog::create_async_nonblock<learnlog::sinks::test_sink_mt>("nonblock logger");
    auto sink_nonblock = std::static_pointer_cast<learnlog::sinks::test_sink_mt>(
        learnlog::get_logger("nonblock logger")->sinks()[0]
    );
    learnlog::create_async<learnlog::sinks::test_sink_mt>("block logger");
    auto sink = std::static_pointer_cast<learnlog::sinks::test_sink_mt>(
        learnlog::get_logger("block logger")->sinks()[0]
    );
    learnlog::flush_all();

    auto tp = learnlog::get_thread_pool();
    REQUIRE(sink_nonblock->flush_count() == 1);
    REQUIRE(sink->flush_count() == 1);
    REQUIRE(tp->override_count() + tp->discard_count() == 0);
}

TEST_CASE("flush_every", "[interface]") {
    learnlog::create_async<learnlog::sinks::test_sink_mt>("flush logger");
    auto sink = std::static_pointer_cast<learnlog::sinks::test_sink_mt>(
        learnlog::get_logger("flush logger")->sinks()[0]
    );

    learnlog::flush_every(learnlog::milliseconds(300));
    std::this_thread::sleep_for(learnlog::seconds(1));
    REQUIRE(sink->flush_count() == 3);
    
    learnlog::flush_every(learnlog::seconds(0));
    learnlog::remove_all();
}