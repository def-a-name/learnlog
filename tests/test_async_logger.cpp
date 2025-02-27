#include <catch2/catch_all.hpp>
#include "async_logger.h"
#include "test_sink.h"
#include "sinks/std_sinks.h"
#include "learnlog.h"

TEST_CASE("lock_msg_queue", "[async_logger]") {
    auto test_sink = std::make_shared<learnlog::sinks::test_sink_mt>();
    // test_sink->set_sink_delay_ms(1);
    size_t msg_queue_size = 128;
    size_t thread_num = 1;
    size_t msg_num = msg_queue_size * 2;
    size_t override_cnt = 0;
    size_t discard_cnt = 0;

    {
        auto tp = std::make_shared<learnlog::base::lock_thread_pool>(msg_queue_size, 
                                                                     thread_num);
        auto logger = std::make_shared<learnlog::async_logger>("lock logger",
                                                               test_sink,
                                                               tp);
        for (size_t i = 0; i < msg_num; i++) {
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

TEST_CASE("lockfree_msg_queue", "[async_logger]") {
    auto test_sink = std::make_shared<learnlog::sinks::test_sink_mt>();
    // test_sink->set_sink_delay_ms(1);
    size_t msg_queue_size = 128;
    size_t thread_num = 1;
    size_t msg_num = msg_queue_size * 2;
    size_t current_cnt = 0;

    {
        auto tp = std::make_shared<learnlog::base::lockfree_thread_pool>(
            msg_queue_size, thread_num);
        auto logger = std::make_shared<learnlog::async_logger>("lockfree logger",
                                                               test_sink,
                                                               tp);
        for (size_t i = 0; i < msg_num; i++) {
            logger->info("message {}", i);
        }
        logger->flush();
        current_cnt = tp->current_msg_count();
    }

    REQUIRE(test_sink->msg_count() == msg_num);
    REQUIRE(test_sink->flush_count() == 1);
    REQUIRE(current_cnt == 0);
}

TEST_CASE("lockfree_concurrent_msg_queue", "[async_logger]") {
    auto test_sink = std::make_shared<learnlog::sinks::test_sink_mt>();
    // test_sink->set_sink_delay_ms(1);
    size_t msg_queue_size = 128;
    size_t thread_num = 1;
    size_t msg_num = msg_queue_size * 2;
    size_t current_cnt = 0;

    {
        auto tp = std::make_shared<learnlog::base::lockfree_concurrent_thread_pool>(
            msg_queue_size, thread_num);
        auto logger = std::make_shared<learnlog::async_logger>("lockfree concurrent logger",
                                                               test_sink,
                                                               tp);
        for (size_t i = 0; i < msg_num; i++) {
            logger->info("message {}", i);
        }
        logger->flush();
        current_cnt = tp->current_msg_count();
    }

    REQUIRE(test_sink->msg_count() == msg_num);
    REQUIRE(test_sink->flush_count() == 1);
    REQUIRE(current_cnt == 0);
}

TEST_CASE("reset thread pool", "[async_logger]") {
    auto test_sink = std::make_shared<learnlog::sinks::test_sink_mt>();
    // test_sink->set_sink_delay_ms(1);
    size_t msg_queue_size = 128;
    size_t thread_num = 1;
    size_t msg_num = msg_queue_size * 2;

    auto tp = std::make_shared<learnlog::base::lock_thread_pool>(msg_queue_size, 
                                                                 thread_num);
    auto logger = std::make_shared<learnlog::async_logger>("reset tp",
                                                           test_sink,
                                                           tp);
    for (size_t i = 0; i < msg_num; i++) {
        logger->info("message {}", i);
    }
    logger->flush();
    tp.reset();

    REQUIRE(test_sink->msg_count() == msg_num);
    REQUIRE(test_sink->flush_count() == 1);
}

TEST_CASE("invalid thread pool", "[async_logger]") {
    auto test_sink = std::make_shared<learnlog::sinks::test_sink_mt>();
    std::shared_ptr<learnlog::base::lock_thread_pool> tp;
    auto logger = std::make_shared<learnlog::async_logger>("invalid tp",
                                                           test_sink,
                                                           tp);
    // throw exception
    logger->info("message will not be logged");
    REQUIRE(test_sink->msg_count() == 0);
}

TEST_CASE("multithreads", "[async_logger]") {
    auto test_sink = std::make_shared<learnlog::sinks::test_sink_mt>();
    // test_sink->set_sink_delay_ms(1);
    size_t msg_queue_size = 128;
    size_t msg_num = msg_queue_size * 2;
    size_t thread_num = 2;
    size_t msg_num_per_thread = msg_num / thread_num;
    std::vector<learnlog::logger_shr_ptr> loggers;

    auto tp_lock = 
        std::make_shared<learnlog::base::lock_thread_pool>(msg_queue_size,
                                                           thread_num);
    loggers.emplace_back(std::make_shared<learnlog::async_logger>("async_lock", 
                                                                  test_sink,
                                                                  std::move(tp_lock)));
    auto tp_lockfree = 
        std::make_shared<learnlog::base::lockfree_thread_pool>(msg_queue_size,
                                                               thread_num);
    loggers.emplace_back(std::make_shared<learnlog::async_logger>("async_lockfree", 
                                                                  test_sink,
                                                                  std::move(tp_lockfree)));
    auto tp_lockfree_concurrent = 
        std::make_shared<learnlog::base::lockfree_concurrent_thread_pool>(msg_queue_size,
                                                                          thread_num);
    loggers.emplace_back(std::make_shared<learnlog::async_logger>("async_lockfree_concurrent", 
                                                                  test_sink,
                                                                  std::move(tp_lockfree_concurrent)));

    std::vector<std::thread> threads;
    auto thread_func = [&loggers, msg_num_per_thread] {
        for (auto &logger : loggers) {
            for (size_t i = 0; i < msg_num_per_thread; i++) {
                logger->info("hello");
            }
            logger->flush();
        }
    };
    for (size_t i = 0; i < thread_num; i++) {
        threads.emplace_back(thread_func);
    }
    for (auto& t : threads) {
        t.join();
    }

    REQUIRE(test_sink->msg_count() == msg_num * loggers.size());
    REQUIRE(test_sink->flush_count() == thread_num * loggers.size());
}

TEST_CASE("create_from_registry", "[interface]") {
    learnlog::remove_all();
    size_t msg_q_size = 128;
    size_t thread_num = 1;
    learnlog::initialize_thread_pool<learnlog::base::lock_thread_pool>(msg_q_size, 
                                                                       thread_num);
    learnlog::set_global_pattern("[%n] %v");
    
    learnlog::create_async<learnlog::sinks::test_sink_mt>("logger1");
    auto sink1 = std::static_pointer_cast<learnlog::sinks::test_sink_mt>(
        learnlog::get_logger("logger1")->sinks()[0]
    );
    learnlog::create_async<learnlog::sinks::test_sink_mt>("logger2");
    auto sink2 = std::static_pointer_cast<learnlog::sinks::test_sink_mt>(
        learnlog::get_logger("logger2")->sinks()[0]
    );
    learnlog::flush_all();
    REQUIRE(sink1->flush_count() == 1);
    REQUIRE(sink2->flush_count() == 1);

    learnlog::create_async_lockfree<learnlog::sinks::test_sink_mt>("logger3");
    auto sink3 = std::static_pointer_cast<learnlog::sinks::test_sink_mt>(
        learnlog::get_logger("logger3")->sinks()[0]
    );
    learnlog::flush_all();
    REQUIRE(sink3->flush_count() == 1);
    REQUIRE(learnlog::get_logger("logger1") == nullptr);
    REQUIRE(learnlog::get_logger("logger2") == nullptr);

    learnlog::create_async_lockfree_concurrent<learnlog::sinks::test_sink_mt>("logger4");
    auto sink4 = std::static_pointer_cast<learnlog::sinks::test_sink_mt>(
        learnlog::get_logger("logger4")->sinks()[0]
    );
    learnlog::flush_all();
    REQUIRE(sink4->flush_count() == 1);
    REQUIRE(learnlog::get_logger("logger3") == nullptr);
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