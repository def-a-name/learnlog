#include <catch2/catch_all.hpp>
#include "base/mpmc_blocking_queue.h"
#include "base/os.h"

#include <iostream>
#include <thread>

using q_type = mylog::base::mpmc_blocking_queue<int>;
using std::chrono::milliseconds;
using test_clock = std::chrono::high_resolution_clock;

static milliseconds millis_from(const test_clock::time_point &tp0) {
    return std::chrono::duration_cast<milliseconds>(test_clock::now() - tp0);
}

TEST_CASE("full_enqueue_empty_dequeue", "[mpmc_blocking_q]") {
    size_t q_size = 6;
    q_type q(q_size);
    
    for (int i = 0; i < static_cast<int>(q_size); i++) {
        q.enqueue_if_have_room(i + 0);
    }
    q.enqueue_if_have_room(6);
    REQUIRE(q.get_discard_count() == 1);
    q.enqueue_nowait(6);
    REQUIRE(q.get_override_count() == 1);

    int item = -1;
    for (int i = 0; i < static_cast<int>(q_size); i++) {
        q.dequeue(item);
    }

    milliseconds wait_ms(100);
    milliseconds tolerance_wait(5);

    auto start = test_clock::now();
    bool res = q.dequeue_for(item, wait_ms);
    auto delta_ms = millis_from(start);

    REQUIRE(res == false);
    REQUIRE(delta_ms >= wait_ms - tolerance_wait);
    REQUIRE(delta_ms <= wait_ms + tolerance_wait);
}

TEST_CASE("invalid_queue", "[mpmc_blocking_q]") {
    size_t q_size = 0;
    q_type q(q_size);
    q.enqueue_nowait(1);
    REQUIRE(q.get_override_count() == 1);
    REQUIRE(q.size() == 0);
    
    int item = -1;
    milliseconds tolerance_wait(5);
    auto start = test_clock::now();
    bool res = q.dequeue_for(item, milliseconds(0));
    auto delta_ms = millis_from(start);

    REQUIRE(res == false);
    REQUIRE(delta_ms <= tolerance_wait);
}

TEST_CASE("rolling", "[mpmc_blocking_q]") {
    size_t q_size = 100;
    q_type q(q_size);
    for (int i = 0; i < static_cast<int>(q_size); i++) {
        q.enqueue(std::move(i));
    }
    q.enqueue_nowait(2024);
    REQUIRE(q.get_override_count() == 1);

    for (int i = 1; i < static_cast<int>(q_size); i++) {
        int item = -1;
        q.dequeue(item);
        REQUIRE(item == i);
    }
    // 检查被覆盖的元素
    int item = -1;
    q.dequeue(item);
    REQUIRE(item == 2024);

    REQUIRE(q.dequeue_for(item, milliseconds(0)) == false);
}

enum enqueue_mode : int {
    enqueue_if_have_room,
    enqueue,
    enqueue_nowait
};

int mpmc(q_type& q, size_t q_size, 
         size_t w_thread_num, size_t r_thread_num, 
         enqueue_mode mode) {
    std::vector<std::thread> threads;
    size_t enqueue_num = q_size * w_thread_num;
    size_t write_interval = 10;
    size_t read_interval = 20;
    std::atomic<bool> finished{false};
    
    std::atomic<int> enqueue_cnt{0};
    auto write_func = [&] {
        for (size_t i = 0; i < q_size; i++) {
            switch (mode) {
                case enqueue_if_have_room:
                    q.enqueue_if_have_room(2024);
                    break;
                case enqueue:
                    q.enqueue(2024);
                    break;
                case enqueue_nowait:
                    q.enqueue_nowait(2024);
                    break;
            }
            enqueue_cnt++;
            mylog::base::os::sleep_for_ms(write_interval);
        }
        if (enqueue_cnt.load() == enqueue_num) {
            finished.store(true);
        }
    };

    std::atomic<int> dequeue_cnt{0};
    auto read_func = [&] {
        int item;
        while (true) {
            if (q.dequeue_for(item, milliseconds(read_interval))) {
                REQUIRE(item == 2024);
                dequeue_cnt++;
            }
            else {
                if (finished.load()) break;
            }
        }
    };

    for (size_t i = 0; i < w_thread_num; i++) {
        threads.emplace_back(write_func);
    }
    for (int i = 0; i < r_thread_num; i++) {
        threads.emplace_back(read_func);
    }
    int thread_cnt = 0;
    for (auto& t : threads) {
        t.join();
        thread_cnt++;
    }
    REQUIRE(thread_cnt == w_thread_num + r_thread_num);

    return dequeue_cnt.load();
}

TEST_CASE("multithread_enqueue_if_have_room", "[mpmc_blocking_q]") {
    size_t q_size = 100;
    q_type q(q_size);
    size_t write_thread_num = 50;
    size_t read_thread_num = 50;
    
    int dequeue_cnt = mpmc(q, q_size, write_thread_num, read_thread_num,
                           enqueue_mode::enqueue_if_have_room);

    std::cout << "discard elements count: " << q.get_discard_count() << std::endl;
    REQUIRE(dequeue_cnt + q.get_discard_count() == q_size * write_thread_num);
    REQUIRE(q.get_override_count() == 0);
}

TEST_CASE("multithread_enqueue", "[mpmc_blocking_q]") {
    size_t q_size = 100;
    q_type q(q_size);
    size_t write_thread_num = 50;
    size_t read_thread_num = 50;
    
    int dequeue_cnt = mpmc(q, q_size, write_thread_num, read_thread_num,
                           enqueue_mode::enqueue);

    REQUIRE(dequeue_cnt == q_size * write_thread_num);
    REQUIRE(q.get_override_count() == 0);
    REQUIRE(q.get_discard_count() == 0);
}

TEST_CASE("multithread_enqueue_nowait", "[mpmc_blocking_q]") {
    size_t q_size = 100;
    q_type q(q_size);
    size_t write_thread_num = 50;
    size_t read_thread_num = 50;
    
    int dequeue_cnt = mpmc(q, q_size, write_thread_num, read_thread_num,
                           enqueue_mode::enqueue_nowait);

    std::cout << "override elements count: " << q.get_override_count() << std::endl;
    REQUIRE(dequeue_cnt + q.get_override_count() == q_size * write_thread_num);
    REQUIRE(q.get_discard_count() == 0);
}