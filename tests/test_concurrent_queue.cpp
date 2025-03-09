#include <catch2/catch_all.hpp>
#include "base/concurrentqueue/blockingconcurrentqueue.h"
#include "base/os.h"

using concurrent_queue = moodycamel::ConcurrentQueue<int>;
using block_concurrent_queue = moodycamel::BlockingConcurrentQueue<int>;

TEST_CASE("concurrent_queue_notoken_st", "[concurrent_queue]") {
    size_t q_size = 128;
    concurrent_queue q(q_size);

    for (int i = 0; i < static_cast<int>(q_size); i++) {
        q.try_enqueue(std::move(i));
    }
    REQUIRE(q.try_enqueue(2025) == false);
    int tmp;
    for (int i = 0; i < static_cast<int>(q_size); i++) {
        q.try_dequeue(tmp);
        REQUIRE(tmp == i);
    }
    REQUIRE(q.try_dequeue(tmp) == false);

    for (int i = 0; i < static_cast<int>(q_size); i++) {
        q.enqueue(std::move(i));
    }
    REQUIRE(q.enqueue(2025) == true);
    REQUIRE(q.size_approx() == q_size + 1);
    for (int i = 0; i < static_cast<int>(q_size); i++) {
        q.try_dequeue(tmp);
        REQUIRE(tmp == i);
    }
    q.try_dequeue(tmp);
    REQUIRE(tmp == 2025);

    size_t vec_size = 60;
    std::vector<int> vec(vec_size, 2025);
    for (size_t i = 0; i < q_size / vec_size; i++) {
        q.try_enqueue_bulk(vec.begin(), vec_size);
    }
    REQUIRE(q.try_enqueue_bulk(vec.begin(), vec_size) == false);
    REQUIRE(q.size_approx() == q_size / vec_size * vec_size);
    size_t dequeue_size = vec_size / 2 + 1;
    size_t dequeue_cnt = 0;
    for (size_t i = 0; i < q_size / dequeue_size + 1; i++) {
        dequeue_cnt += q.try_dequeue_bulk(vec.begin(), dequeue_size);
    }
    REQUIRE(dequeue_cnt == q_size / vec_size * vec_size);
}

TEST_CASE("concurrent_queue_token_st", "[concurrent_queue]") {
    size_t q_size = 128;
    concurrent_queue q(q_size);
    moodycamel::ProducerToken p_token(q);
    moodycamel::ConsumerToken c_token(q);

    for (int i = 0; i < static_cast<int>(q_size); i++) {
        q.try_enqueue(p_token, std::move(i));
    }
    REQUIRE(q.try_enqueue(p_token, 2025) == false);
    int tmp;
    for (int i = 0; i < static_cast<int>(q_size); i++) {
        q.try_dequeue(c_token, tmp);
        REQUIRE(tmp == i);
    }
    REQUIRE(q.try_dequeue(c_token, tmp) == false);

    for (int i = 0; i < static_cast<int>(q_size); i++) {
        q.enqueue(p_token, std::move(i));
    }
    REQUIRE(q.enqueue(p_token, 2025) == true);
    REQUIRE(q.size_approx() == q_size + 1);
    for (int i = 0; i < static_cast<int>(q_size); i++) {
        q.try_dequeue(c_token, tmp);
        REQUIRE(tmp == i);
    }
    q.try_dequeue(c_token, tmp);
    REQUIRE(tmp == 2025);

    size_t vec_size = 60;
    std::vector<int> vec(vec_size, 2025);
    for (size_t i = 0; i < q_size / vec_size; i++) {
        q.try_enqueue_bulk(p_token, vec.begin(), vec_size);
    }
    REQUIRE(q.try_enqueue_bulk(p_token, vec.begin(), vec_size) == false);
    REQUIRE(q.size_approx() == q_size / vec_size * vec_size);
    size_t dequeue_size = vec_size / 2 + 1;
    size_t dequeue_cnt = 0;
    for (size_t i = 0; i < q_size / dequeue_size + 1; i++) {
        dequeue_cnt += q.try_dequeue_bulk(c_token, vec.begin(), dequeue_size);
    }
    REQUIRE(dequeue_cnt == q_size / vec_size * vec_size);
}


enum token_t : int {
    producer_consumer_token,
    only_producer_token,
    no_token
};

void mpmc_concurrent_queue(concurrent_queue& q, size_t thread_enqueue_size, 
                          size_t write_thread_num, size_t read_thread_num,
                          token_t token_type) {
    std::vector<std::thread> threads;
    size_t enqueue_num = thread_enqueue_size * write_thread_num;
    size_t write_interval_ms = 5;
    size_t read_interval_ms = 10;
    
    std::atomic<bool> enqueue_finished{false};
    std::atomic<size_t> enqueue_cnt{0};
    auto write_func = [&] {
        moodycamel::ProducerToken p_token(q);   

        for (int i = 0; i < static_cast<int>(thread_enqueue_size); i++) {
            if (token_type != no_token) {
                while (!q.try_enqueue(p_token, std::move(i)))
                    continue;
            }
            else  {
                while (!q.try_enqueue(std::move(i)))
                    continue;
            }
            ++enqueue_cnt;
            std::this_thread::sleep_for(learnlog::milliseconds(write_interval_ms));
        }

        if (enqueue_cnt.load() == enqueue_num) {
            enqueue_finished.store(true);
        }
    };

    std::vector<size_t> dequeue_cnts(thread_enqueue_size);
    std::mutex dequeue_cnts_mutex;
    auto read_func = [&] {
        moodycamel::ConsumerToken c_token(q);

        int item;
        while (true) {
            bool dequeue_flag = false;
            switch (token_type) {
                case producer_consumer_token:
                    dequeue_flag = q.try_dequeue(c_token, item);
                    break;
                case only_producer_token:
                    break;
                case no_token:
                    dequeue_flag = q.try_dequeue(item);
            }
            if (dequeue_flag) {
                {
                    std::lock_guard<std::mutex> lock(dequeue_cnts_mutex);
                    ++dequeue_cnts[item];
                }
                std::this_thread::sleep_for(learnlog::milliseconds(read_interval_ms));
            }
            else {
                if (enqueue_finished.load()) break;
            }
        }
    };

    for (size_t i = 0; i < write_thread_num; i++) {
        threads.emplace_back(write_func);
    }
    for (size_t i = 0; i < read_thread_num; i++) {
        threads.emplace_back(read_func);
    }
    size_t thread_cnt = 0;
    for (auto& t : threads) {
        t.join();
        thread_cnt++;
    }
    REQUIRE(thread_cnt == write_thread_num + read_thread_num);

    for (size_t i = 0; i < thread_enqueue_size; i++) {
        REQUIRE(dequeue_cnts[i] == write_thread_num);
    }
}

void mpmc_bind_concurrent_queue(concurrent_queue& q, size_t thread_enqueue_size, 
                                size_t thread_num, token_t token_type) {
    if (token_type != only_producer_token) return;

    std::vector<std::thread> threads;
    size_t enqueue_num = thread_enqueue_size * thread_num;
    size_t write_interval_ms = 5;
    size_t read_interval_ms = 10;
    
    std::atomic<bool> enqueue_finished{false};
    std::atomic<size_t> enqueue_cnt{0};
    std::vector<std::shared_ptr<moodycamel::ProducerToken> > p_tokens;
    std::mutex p_tokens_mutex;
    auto write_func = [&] {
        auto p_token = std::make_shared<moodycamel::ProducerToken>(q);
        {
            std::lock_guard<std::mutex> lock(p_tokens_mutex);
            p_tokens.push_back(p_token);
        }

        for (int i = 0; i < static_cast<int>(thread_enqueue_size); i++) {
            while (!q.try_enqueue(*p_token, std::move(i)))
                    continue;
            ++enqueue_cnt;
            std::this_thread::sleep_for(learnlog::milliseconds(write_interval_ms));
        }

        if (enqueue_cnt.load() == enqueue_num) {
            enqueue_finished.store(true);
        }
    };

    std::vector<size_t> dequeue_cnts(thread_enqueue_size);
    std::mutex dequeue_cnts_mutex;
    std::atomic<size_t> dequeue_cnt{0};
    std::atomic<size_t> p_tokens_idx{0};
    auto read_func = [&] {
        auto p_token = p_tokens[p_tokens_idx.fetch_add(1, std::memory_order_relaxed)];

        int item;
        while (true) {
            if (q.try_dequeue_from_producer(*p_token, item)) {
                {
                    std::lock_guard<std::mutex> lock(dequeue_cnts_mutex);
                    ++dequeue_cnts[item];
                }
                ++dequeue_cnt;
                std::this_thread::sleep_for(learnlog::milliseconds(read_interval_ms));
            }
            else {
                if (dequeue_cnt.load() == enqueue_num) break;
            }
        }
    };

    for (size_t i = 0; i < thread_num; i++) {
        threads.emplace_back(write_func);
    }
    for (size_t i = 0; i < thread_num; i++) {
        threads.emplace_back(read_func);
    }
    size_t thread_cnt = 0;
    for (auto& t : threads) {
        t.join();
        thread_cnt++;
    }
    REQUIRE(thread_cnt == 2 * thread_num);

    for (size_t i = 0; i < thread_enqueue_size; i++) {
        REQUIRE(dequeue_cnts[i] == thread_num);
    }
}

void mpmc_block_concurrent_queue_notoken(block_concurrent_queue& q, 
                                         size_t thread_enqueue_size, 
                                         size_t write_thread_num,
                                         size_t read_thread_num) {
    std::vector<std::thread> threads;
    size_t enqueue_num = thread_enqueue_size * write_thread_num;
    size_t write_interval_ms = 10;
    size_t read_wait_interval_ms = 5;
    
    std::atomic<bool> enqueue_finished{false};
    std::atomic<size_t> enqueue_cnt{0};
    auto write_func = [&] {
        for (int i = 0; i < static_cast<int>(thread_enqueue_size); i++) {
            while (!q.try_enqueue(std::move(i)))
                    continue;
            enqueue_cnt++;
            std::this_thread::sleep_for(learnlog::milliseconds(write_interval_ms));
        }

        if (enqueue_cnt.load() == enqueue_num) {
            enqueue_finished.store(true);
        }
    };

    std::vector<size_t> dequeue_cnts(thread_enqueue_size);
    std::mutex dequeue_cnts_mutex;
    auto read_func = [&] {
        int item;
        while (true) {
            if (q.wait_dequeue_timed(item, 
                                     learnlog::milliseconds(read_wait_interval_ms))) {
                {
                    std::lock_guard<std::mutex> lock(dequeue_cnts_mutex);
                    ++dequeue_cnts[item];
                }
            }
            else {
                if (enqueue_finished.load()) break;
            }
        }
    };

    for (size_t i = 0; i < write_thread_num; i++) {
        threads.emplace_back(write_func);
    }
    for (size_t i = 0; i < read_thread_num; i++) {
        threads.emplace_back(read_func);
    }
    size_t thread_cnt = 0;
    for (auto& t : threads) {
        t.join();
        thread_cnt++;
    }
    REQUIRE(thread_cnt == read_thread_num + write_thread_num);

    for (size_t i = 0; i < thread_enqueue_size; i++) {
        REQUIRE(dequeue_cnts[i] == write_thread_num);
    }
}

TEST_CASE("concurrent_queue_mpmc", "[concurrent_queue]") {
    const size_t BLOCK_SIZE = 32;
    size_t q_size = 128;
    
    size_t write_thread_num = q_size / BLOCK_SIZE;
    size_t read_thread_num = 2;

    // 每个 producer 获取一个 producer_token 来入队，每个 consumer 获取一个 consumer_token 来出队，
    // 由于使用了 token 和 try_enqueue()，producer 请求到的 BLOCK 不会释放，所以
    // 每个 producer 只能分得 1 个 BLOCK
    concurrent_queue q1(q_size);
    mpmc_concurrent_queue(q1, BLOCK_SIZE, write_thread_num, read_thread_num, 
                          producer_consumer_token);
    REQUIRE(q1.size_approx() == 0);
    
    // 每个 producer 获取一个 producer_token 来入队，每个 consumer 使用一个 producer_token 来出队，
    // producer 与 consumer 一一对应，
    // 由于使用了 token 和 try_enqueue()，producer 请求到的 BLOCK 不会释放，所以
    // 每个 producer 只能分得 1 个 BLOCK
    concurrent_queue q2(q_size);
    mpmc_bind_concurrent_queue(q2, BLOCK_SIZE, write_thread_num, 
                               only_producer_token);
    REQUIRE(q2.size_approx() == 0);
    
    // 使用 try_enqueue()，但是不使用 token，每个 producer 请求到的 BLOCK 在这个 BLOCK 的所有元素
    // 出队后会释放，所以 BLOCK 可以循环利用，producer 可以获取到更多的 BLOCK
    concurrent_queue q3(q_size);
    mpmc_concurrent_queue(q3, q_size / 2, write_thread_num, read_thread_num, 
                          no_token);
    REQUIRE(q3.size_approx() == 0);
}

TEST_CASE("block_concurrent_queue_mpmc", "[concurrent_queue]") { 
    size_t q_size = 128;
    
    size_t write_thread_num = 4;
    size_t read_thread_num = write_thread_num * 2;

    // block_concurrent_queue 用轻量级无锁信号量实现了 wait_dequeue()，代替 concurrent_queue
    // 中的忙等待 while(!try_dequeue()) continue;
    block_concurrent_queue q(q_size);
    mpmc_block_concurrent_queue_notoken(q, q_size, 
                                        write_thread_num, read_thread_num);
    REQUIRE(q.size_approx() == 0);                                    
}