#include "learnlog.h"
#include "base/concurrentqueue/blockingconcurrentqueue.h"
#include "base/concurrentqueue/concurrentqueue.h"
#include "base/block_queue.h"

enum token_t : int {
    only_consumer,
    only_producer,
    no_token
};

double bench_mpsc_lock_q(int q_size, int thread_cnt, int msg_cnt);
double bench_mpsc_block_concurrent_q(int q_size, int thread_cnt, int msg_cnt, token_t token_type);
double bench_mpsc_concurrent_q(int q_size, int thread_cnt, int msg_cnt, token_t token_type);
double bench_mpmc_concurrent_q(int q_size, int thread_cnt, int msg_cnt);

int main(int argc, char *argv[]) {
    int q_size = 32768;

    try {
        learnlog::set_global_pattern("[%^%l%$] %v");

        if (argc > 1) {
            q_size = atoi(argv[1]);
            if (q_size < 512) {
                learnlog::warn("Queue size should be at least 512!");
                q_size = 512;
            }
        }
        if (argc > 2) {
            learnlog::error("Unknown args! Usage: {} <queue_size>", argv[0]);
            return 0;
        }

        learnlog::info("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        learnlog::info("Queue size: {:L}", q_size);
        learnlog::info("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

        std::vector<double> mpsc_msg_fill_rate{0.5, 1.0, 2.0, 4.0};
        std::vector<int> mpsc_msg_cnts;
        for (double& rate : mpsc_msg_fill_rate) {
            mpsc_msg_cnts.push_back(static_cast<int>(q_size * rate));
        }
        std::vector<int> thread_cnts{1, 2, 4, 8, 16};

        learnlog::info("*********************************");
        learnlog::info("multiple producers single consumer (elapse/ms)");
        learnlog::info("*********************************");
        learnlog::info("thread_cnt  msg_cnt  {}  {}  {}  {}  {}",
                       "lock_q", "block_con_q_token", "block_con_q_no_token",
                       "con_q_token", "con_q_no_token");
        learnlog::info("-------------------------------------------------");
        for (int& thread_cnt : thread_cnts) {
            for (int& msg_cnt : mpsc_msg_cnts) {
                learnlog::info("{:8d} {:8d} {:8.3f} {:8.3f} {:8.3f} {:8.3f} {:8.3f}",
                               thread_cnt, msg_cnt,
                               bench_mpsc_lock_q(q_size, thread_cnt, msg_cnt),
                               bench_mpsc_block_concurrent_q(q_size, thread_cnt, msg_cnt, only_consumer),
                               bench_mpsc_block_concurrent_q(q_size, thread_cnt, msg_cnt, no_token),
                               bench_mpsc_concurrent_q(q_size, thread_cnt, msg_cnt, only_consumer),
                               bench_mpsc_concurrent_q(q_size, thread_cnt, msg_cnt, no_token));
            }
            learnlog::info("-------------------------------------------------");
        }

        learnlog::info("*********************************");
        learnlog::info("multiple producers single consumer (throughput/(msg/s))");
        learnlog::info("*********************************");
        learnlog::info("thread_cnt  msg_cnt  {}  {}  {}  {}  {}",
                       "lock_q", "block_con_q_token", "block_con_q_no_token",
                       "con_q_token", "con_q_no_token");
        learnlog::info("-------------------------------------------------");
        for (int& thread_cnt : thread_cnts) {
            for (int& msg_cnt : mpsc_msg_cnts) {
                learnlog::info("{:8d} {:8d} {:8d} {:8d} {:8d} {:8d} {:8d}",
                               thread_cnt, msg_cnt,
                               static_cast<int>(msg_cnt / bench_mpsc_lock_q(q_size, thread_cnt, msg_cnt)),
                               static_cast<int>(msg_cnt / bench_mpsc_block_concurrent_q(q_size, thread_cnt, msg_cnt, only_consumer)),
                               static_cast<int>(msg_cnt / bench_mpsc_block_concurrent_q(q_size, thread_cnt, msg_cnt, no_token)),
                               static_cast<int>(msg_cnt / bench_mpsc_concurrent_q(q_size, thread_cnt, msg_cnt, only_consumer)),
                               static_cast<int>(msg_cnt / bench_mpsc_concurrent_q(q_size, thread_cnt, msg_cnt, no_token)));
                }
            learnlog::info("-------------------------------------------------");
        }

        learnlog::info("*********************************");
        learnlog::info("multiple producers multiple consumers bind (each consumer consumes from exact producer)");
        learnlog::info("*********************************");
        learnlog::info("prod/cons_thread_cnt  msg_cnt  elapse/ms  throughput/(msg/s)");
        learnlog::info("-------------------------------------------------");
        std::vector<double> mpmc_msg_fill_rate{0.5, 1.0};
        std::vector<int> mpmc_msg_cnts;
        for (double& rate : mpmc_msg_fill_rate) {
            mpmc_msg_cnts.push_back(static_cast<int>(q_size * rate));
        }
        for (int& thread_cnt : thread_cnts) {
            for (int& msg_cnt : mpmc_msg_cnts) {
                double elapse_ms = bench_mpmc_concurrent_q(q_size, thread_cnt, msg_cnt);
                learnlog::info("{:8d} {:8d} {:8.3f} {:8d}",
                               thread_cnt, msg_cnt,
                               elapse_ms,
                               static_cast<int>(msg_cnt / elapse_ms));
                }
            learnlog::info("-------------------------------------------------");
        }
    }
    LEARNLOG_CATCH

    return 0;
}

double bench_mpsc_lock_q(int q_size, int thread_cnt, int msg_cnt) {
    using item_t = learnlog::base::async_msg;
    learnlog::base::block_queue<item_t> q(static_cast<size_t>(q_size));
    int msg_cnt_per_thread = msg_cnt / thread_cnt;

    auto write_func = [&] {
        for (int i = 0; i < msg_cnt_per_thread; i++) {
            item_t msg;
            q.enqueue(std::move(msg));
        }
    };

    std::atomic<int> dequeue_cnt{0};
    auto read_func = [&] {
        item_t msg;
        while (true) {
            if (q.dequeue_for(msg, learnlog::milliseconds(1))) {
                ++dequeue_cnt;
            }
            else {
                if (dequeue_cnt.load(std::memory_order_relaxed) == msg_cnt) 
                    break;
            }
        }
    };

    auto start_tp = learnlog::sys_clock::now();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < thread_cnt; i++) {
        threads.emplace_back(write_func);
    }
    threads.emplace_back(read_func);
    for (auto& t : threads) {
        t.join();
    }

    auto delta = learnlog::sys_clock::now() - start_tp;
    return static_cast<double>(delta.count()) / 1e6;
}

double bench_mpsc_block_concurrent_q(int q_size, int thread_cnt, int msg_cnt, token_t token_type) {
    using item_t = learnlog::base::async_msg;
    moodycamel::BlockingConcurrentQueue<item_t> q(static_cast<size_t>(q_size));
    int msg_cnt_per_thread = msg_cnt / thread_cnt;

    auto write_func = [&] {
        for (int i = 0; i < msg_cnt_per_thread; i++) {
            item_t msg;
            while (!q.try_enqueue(std::move(msg)))
                learnlog::base::os::sleep_for_ms(1);    // 避免死锁
        }
    };

    std::atomic<int> dequeue_cnt{0};
    auto read_func = [&] {
        moodycamel::ConsumerToken c_token(q);
        item_t msg;
        while (true) {
            bool dequeue_flag = false;

            if (token_type == no_token) {
                dequeue_flag = q.wait_dequeue_timed(msg, learnlog::milliseconds(1));
            }
            else {
                dequeue_flag = q.wait_dequeue_timed(c_token, msg, learnlog::milliseconds(1));
            }

            if (dequeue_flag) {
                ++dequeue_cnt;
            }
            else {
                if (dequeue_cnt.load(std::memory_order_relaxed) == msg_cnt) 
                    break;
            }
        }
    };

    auto start_tp = learnlog::sys_clock::now();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < thread_cnt; i++) {
        threads.emplace_back(write_func);
    }
    threads.emplace_back(read_func);
    for (auto& t : threads) {
        t.join();
    }

    auto delta = learnlog::sys_clock::now() - start_tp;
    return static_cast<double>(delta.count()) / 1e6;
}

double bench_mpsc_concurrent_q(int q_size, int thread_cnt, int msg_cnt, token_t token_type) {
    using item_t = learnlog::base::async_msg;
    moodycamel::ConcurrentQueue<item_t> q(static_cast<size_t>(q_size));
    int msg_cnt_per_thread = msg_cnt / thread_cnt;

    auto write_func = [&] {
        for (int i = 0; i < msg_cnt_per_thread; i++) {
            item_t msg;
            while (!q.try_enqueue(std::move(msg)))
                learnlog::base::os::sleep_for_ms(1);    // 避免死锁
        }
    };

    std::atomic<int> dequeue_cnt{0};
    auto read_func = [&] {
        moodycamel::ConsumerToken c_token(q);
        item_t msg;
        while (true) {
            bool dequeue_flag = false;

            if (token_type == no_token) {
                dequeue_flag = q.try_dequeue(msg);
            }
            else {
                dequeue_flag = q.try_dequeue(c_token, msg);
            }

            if (dequeue_flag) {
                ++dequeue_cnt;
            }
            else {
                if (dequeue_cnt.load(std::memory_order_relaxed) == msg_cnt) 
                    break;
            }
        }
    };

    auto start_tp = learnlog::sys_clock::now();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < thread_cnt; i++) {
        threads.emplace_back(write_func);
    }
    threads.emplace_back(read_func);
    for (auto& t : threads) {
        t.join();
    }

    auto delta = learnlog::sys_clock::now() - start_tp;
    return static_cast<double>(delta.count()) / 1e6;
}

double bench_mpmc_concurrent_q(int q_size, int thread_cnt, int msg_cnt) {
    using item_t = learnlog::base::async_msg;
    moodycamel::ConcurrentQueue<item_t> q(static_cast<size_t>(q_size));
    std::vector<std::shared_ptr<moodycamel::ProducerToken> > p_tokens;
    std::mutex p_tokens_mutex;
    int msg_cnt_per_thread = msg_cnt / thread_cnt;

    auto write_func = [&] {
        auto p_token = std::make_shared<moodycamel::ProducerToken>(q);
        {
            std::lock_guard<std::mutex> lock(p_tokens_mutex);
            p_tokens.push_back(p_token);
        }

        for (int i = 0; i < msg_cnt_per_thread; i++) {
            item_t msg;
            while (!q.try_enqueue(*p_token, std::move(msg)))
                continue;
        }
    };

    std::atomic<int> dequeue_cnt{0};
    std::atomic<size_t> p_tokens_idx{0};
    auto read_func = [&] {
        size_t p_token_idx = p_tokens_idx.fetch_add(1, std::memory_order_relaxed);
        while (p_token_idx >= p_tokens.size())
            continue;
        std::shared_ptr<moodycamel::ProducerToken> p_token;
        {
            std::lock_guard<std::mutex> lock(p_tokens_mutex);
            p_token = p_tokens[p_token_idx];
        }

        item_t msg;
        while (true) {
            if (q.try_dequeue_from_producer(*p_token, msg)) {
                ++dequeue_cnt;
            }
            else {
                if (dequeue_cnt.load(std::memory_order_relaxed) == msg_cnt) 
                    break;
            }
        }
    };

    auto start_tp = learnlog::sys_clock::now();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < thread_cnt; i++) {
        threads.emplace_back(write_func);
    }
    for (int i = 0; i < thread_cnt; i++) {
        threads.emplace_back(read_func);
    }
    for (auto& t : threads) {
        t.join();
    }

    auto delta = learnlog::sys_clock::now() - start_tp;
    return static_cast<double>(delta.count()) / 1e6;
}