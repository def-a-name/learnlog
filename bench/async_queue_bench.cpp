#include "learnlog.h"

enum token_t : int {
    no_token,
    only_consumer,
    only_producer,
    producer_consumer
};

double bench_lock_q(int q_size, int p_thread_cnt, int c_thread_cnt, int msg_cnt);
double bench_block_concurrent_q(int q_size, int p_thread_cnt, int c_thread_cnt, int msg_cnt, token_t token_type);
double bench_concurrent_q(int q_size, int p_thread_cnt, int c_thread_cnt, int msg_cnt, token_t token_type);

int main(int argc, char *argv[]) {
    int q_size = 8192;

    try {
        learnlog::set_global_pattern("[%^%l%$] %v");
        learnlog::set_global_log_level(learnlog::level::debug);

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

        std::vector<double> mpsc_msg_fill_rate{1.0, 4.0, 8.0, 16.0};
        std::vector<int> thread_cnts{1, 2, 4, 8, 16};

        learnlog::info("*********************************");
        learnlog::info("multiple producers single consumer (throughput | msg/ms)");
        learnlog::info("*********************************");
        learnlog::debug("p_thd_cnt: produce_threads(writers) count");
        learnlog::debug("q1: lock queue");
        learnlog::debug("q2: concurrent queue (no token)");
        learnlog::debug("q3: block concurrent queue (no token)");
        learnlog::debug("q4: concurrent queue (enq:no_token; deq:consumer_token)");
        learnlog::debug("q5: block concurrent queue (enq:no_token; deq:consumer_token)");
        learnlog::debug("q6: concurrent queue (enq:producer_token; deq:consumer_token)");
        learnlog::debug("q7: block concurrent queue (enq:producer_token; deq:consumer_token)");
        learnlog::debug("q8: concurrent queue (enq:producer_token; deq:producer_token)");
        learnlog::debug("q9: block concurrent queue (enq:producer_token; deq:no_token)");
        learnlog::info("-------------------------------------------------");
        learnlog::info("{:12s}| {:12s}| {:12s}| {:12s}{:12s}| {:12s}{:12s}| {:12s}{:12s}| {:12s}{:12s}",
                       "p_thd_cnt", "msg_cnt",
                       "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9");
        learnlog::info("-------------------------------------------------");
        for (int& thread_cnt : thread_cnts) {
            for (auto& fill_rate : mpsc_msg_fill_rate) {
                int msg_cnt = static_cast<int>(q_size * fill_rate);
                learnlog::info(
                    "{:<12d}|{:8L}*{:4.1f}| {:<12d}| {:<12d}{:<12d}| {:<12d}{:<12d}| {:<12d}{:<12d}| {:<12d}{:<12d}",
                    thread_cnt, q_size, fill_rate,
                    static_cast<int>(msg_cnt / bench_lock_q(q_size, thread_cnt, 1, msg_cnt)),
                    static_cast<int>(msg_cnt / bench_concurrent_q(q_size, thread_cnt, 1, msg_cnt, no_token)),
                    static_cast<int>(msg_cnt / bench_block_concurrent_q(q_size, thread_cnt, 1, msg_cnt, no_token)),
                    static_cast<int>(msg_cnt / bench_concurrent_q(q_size, thread_cnt, 1, msg_cnt, only_consumer)),
                    static_cast<int>(msg_cnt / bench_block_concurrent_q(q_size, thread_cnt, 1, msg_cnt, only_consumer)),
                    static_cast<int>(msg_cnt / bench_concurrent_q(q_size, thread_cnt, 1, msg_cnt, producer_consumer)),
                    static_cast<int>(msg_cnt / bench_block_concurrent_q(q_size, thread_cnt, 1, msg_cnt, producer_consumer)),
                    static_cast<int>(msg_cnt / bench_concurrent_q(q_size, thread_cnt, 1, msg_cnt, only_producer)),
                    static_cast<int>(msg_cnt / bench_block_concurrent_q(q_size, thread_cnt, 1, msg_cnt, only_producer)));
            }
            learnlog::info("-------------------------------------------------");
        }

        double mpmc_msg_fill_rate = mpsc_msg_fill_rate.back();
        int mpmc_msg_cnt = static_cast<int>(q_size * mpmc_msg_fill_rate);
        int p_thread_cnt = 8;
        learnlog::info("*********************************");
        learnlog::info("multiple producers multiple consumers (throughput | msg/ms)");
        learnlog::info("*********************************");
        learnlog::info("*********************************");
        learnlog::info("messages count: {:L}*{:.2f}; produce threads count: {:L}", q_size, mpmc_msg_fill_rate, p_thread_cnt);
        learnlog::info("*********************************");
        learnlog::debug("c_thd_cnt: consume_threads(readers) count");
        learnlog::debug("q1: lock queue");
        learnlog::debug("q2: concurrent queue (no token)");
        learnlog::debug("q3: block concurrent queue (no token)");
        learnlog::debug("q4: concurrent queue (enq:no_token; deq:consumer_token)");
        learnlog::debug("q5: block concurrent queue (enq:no_token; deq:consumer_token)");
        learnlog::debug("q6: concurrent queue (enq:producer_token; deq:consumer_token)");
        learnlog::debug("q7: block concurrent queue (enq:producer_token; deq:consumer_token)");
        learnlog::debug("q8: concurrent queue (enq:producer_token; deq:producer_token)");
        learnlog::debug("q9: block concurrent queue (enq:producer_token; deq:no_token)");
        learnlog::info("-------------------------------------------------");
        learnlog::info("{:12s}| {:12s}| {:12s}{:12s}| {:12s}{:12s}| {:12s}{:12s}| {:12s}{:12s}",
                       "c_thd_cnt",
                       "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9");
        learnlog::info("-------------------------------------------------");
        for (int& c_thread_cnt : thread_cnts) {
            learnlog::info(
                "{:<12d}| {:<12d}| {:<12d}{:<12d}| {:<12d}{:<12d}| {:<12d}{:<12d}| {:<12d}{:<12d}",
                c_thread_cnt,
                static_cast<int>(mpmc_msg_cnt / bench_lock_q(q_size, p_thread_cnt, c_thread_cnt, mpmc_msg_cnt)),
                static_cast<int>(mpmc_msg_cnt / bench_concurrent_q(q_size, p_thread_cnt, c_thread_cnt, mpmc_msg_cnt, no_token)),
                static_cast<int>(mpmc_msg_cnt / bench_block_concurrent_q(q_size, p_thread_cnt, c_thread_cnt, mpmc_msg_cnt, no_token)),
                static_cast<int>(mpmc_msg_cnt / bench_concurrent_q(q_size, p_thread_cnt, c_thread_cnt, mpmc_msg_cnt, only_consumer)),
                static_cast<int>(mpmc_msg_cnt / bench_block_concurrent_q(q_size, p_thread_cnt, c_thread_cnt, mpmc_msg_cnt, only_consumer)),
                static_cast<int>(mpmc_msg_cnt / bench_concurrent_q(q_size, p_thread_cnt, c_thread_cnt, mpmc_msg_cnt, producer_consumer)),
                static_cast<int>(mpmc_msg_cnt / bench_block_concurrent_q(q_size, p_thread_cnt, c_thread_cnt, mpmc_msg_cnt, producer_consumer)),
                static_cast<int>(mpmc_msg_cnt / bench_concurrent_q(q_size, p_thread_cnt, c_thread_cnt, mpmc_msg_cnt, only_producer)),
                static_cast<int>(mpmc_msg_cnt / bench_block_concurrent_q(q_size, p_thread_cnt, c_thread_cnt, mpmc_msg_cnt, only_producer)));
        }
            
    }
    LEARNLOG_CATCH

    return 0;
}

double bench_lock_q(int q_size, int p_thread_cnt, int c_thread_cnt, int msg_cnt) {
    using item_t = learnlog::base::async_msg;
    learnlog::base::block_queue<item_t> q(static_cast<size_t>(q_size));

    auto write_func = [&](int msg_num) {
        for (int i = 0; i < msg_num; i++) {
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

    int msg_cnt_per_thread = msg_cnt / p_thread_cnt;
    int msg_cnt_mod = msg_cnt % p_thread_cnt;
    std::vector<std::thread> threads;

    auto start_tp = learnlog::sys_clock::now();
    
    threads.emplace_back(write_func, msg_cnt_per_thread + msg_cnt_mod);
    for (int i = 1; i < p_thread_cnt; i++) {
        threads.emplace_back(write_func, msg_cnt_per_thread);
    }
    for (int i = 0; i < c_thread_cnt; i++) {
        threads.emplace_back(read_func);
    }
    for (auto& t : threads) {
        t.join();
    }

    auto delta = learnlog::sys_clock::now() - start_tp;
    return static_cast<double>(delta.count()) / 1e6;
}

double bench_block_concurrent_q(int q_size, 
                                int p_thread_cnt,
                                int c_thread_cnt,
                                int msg_cnt,
                                token_t token_type) {
    using item_t = learnlog::base::async_msg;

    moodycamel::BlockingConcurrentQueue<item_t> q(static_cast<size_t>(q_size));

    auto write_func_without_token = [&q](int msg_num) {
        for (int i = 0; i < msg_num; i++) {
            item_t msg;
            while (!q.try_enqueue(std::move(msg)))
                continue;
        }
    };

    std::vector<std::shared_ptr<moodycamel::ProducerToken> > p_tokens;
    std::mutex p_tokens_mutex;
    auto write_func_with_p_token = [&q, &p_tokens, &p_tokens_mutex](int msg_num) {
        auto p_token = std::make_shared<moodycamel::ProducerToken>(q);
        {
            std::lock_guard<std::mutex> lock(p_tokens_mutex);
            p_tokens.push_back(p_token);
        }

        for (int i = 0; i < msg_num; i++) {
            item_t msg;
            while (!q.try_enqueue(*p_token, std::move(msg)))
                if (q.enqueue(*p_token, std::move(msg)))
                    break;
        }
    };

    std::atomic<int> dequeue_cnt{0};

    auto read_func_without_token = [&q, &dequeue_cnt, msg_cnt] {
        item_t msg;
        while (true) {
            if (q.wait_dequeue_timed(msg, learnlog::milliseconds(1))) {
                ++dequeue_cnt;
            }
            else {
                if (dequeue_cnt.load(std::memory_order_relaxed) == msg_cnt) 
                    break;
            }
        }
    };

    auto read_func_with_c_token = [&q, &dequeue_cnt, msg_cnt] {
        moodycamel::ConsumerToken c_token(q);
        item_t msg;
        while (true) {
            if (q.wait_dequeue_timed(c_token, msg, learnlog::milliseconds(1))) {
                ++dequeue_cnt;
            }
            else {
                if (dequeue_cnt.load(std::memory_order_relaxed) == msg_cnt) 
                    break;
            }
        }
    };

    int msg_cnt_per_thread = msg_cnt / p_thread_cnt;
    int msg_cnt_mod = msg_cnt % p_thread_cnt;
    std::vector<std::thread> threads;

    auto start_tp = learnlog::sys_clock::now();
    
    switch (token_type) {
        case no_token:
            threads.emplace_back(write_func_without_token, 
                                 msg_cnt_per_thread + msg_cnt_mod);
            for (int i = 1; i < p_thread_cnt; i++) {
                threads.emplace_back(write_func_without_token,
                                     msg_cnt_per_thread);
            }
            for (int i = 0; i < c_thread_cnt; i++) {
                threads.emplace_back(read_func_without_token);
            }
            break;
        case only_consumer:
            threads.emplace_back(write_func_without_token, 
                                 msg_cnt_per_thread + msg_cnt_mod);
            for (int i = 1; i < p_thread_cnt; i++) {
                threads.emplace_back(write_func_without_token,
                                     msg_cnt_per_thread);
            }
            for (int i = 0; i < c_thread_cnt; i++) {
                threads.emplace_back(read_func_with_c_token);
            }
            break;
        case producer_consumer:
            threads.emplace_back(write_func_with_p_token, 
                                 msg_cnt_per_thread + msg_cnt_mod);
            for (int i = 1; i < p_thread_cnt; i++) {
                threads.emplace_back(write_func_with_p_token,
                                     msg_cnt_per_thread);
            }
            for (int i = 0; i < c_thread_cnt; i++) {
                threads.emplace_back(read_func_with_c_token);
            }
            break;
        case only_producer:
            threads.emplace_back(write_func_with_p_token, 
                                 msg_cnt_per_thread + msg_cnt_mod);
            for (int i = 1; i < p_thread_cnt; i++) {
                threads.emplace_back(write_func_with_p_token,
                                     msg_cnt_per_thread);
            }
            for (int i = 0; i < c_thread_cnt; i++) {
                threads.emplace_back(read_func_without_token);
            }
            break;
        default: 
            assert(false);
    }
    for (auto& t : threads) {
        t.join();
    }

    auto delta = learnlog::sys_clock::now() - start_tp;
    return static_cast<double>(delta.count()) / 1e6;
}

double bench_concurrent_q(int q_size, 
                          int p_thread_cnt,
                          int c_thread_cnt,
                          int msg_cnt,
                          token_t token_type) {
    using item_t = learnlog::base::async_msg;

    moodycamel::ConcurrentQueue<item_t> q(static_cast<size_t>(q_size));

    auto write_func_without_token = [&q](int msg_num) {
        for (int i = 0; i < msg_num; i++) {
            item_t msg;
            while (!q.try_enqueue(std::move(msg)))
                learnlog::base::os::sleep_for_ms(1);    // 避免死锁
        }
    };

    std::vector<std::shared_ptr<moodycamel::ProducerToken> > p_tokens;
    std::mutex p_tokens_mutex;
    auto write_func_with_p_token = [&q, &p_tokens, &p_tokens_mutex](int msg_num) {
        auto p_token = std::make_shared<moodycamel::ProducerToken>(q);
        {
            std::lock_guard<std::mutex> lock(p_tokens_mutex);
            p_tokens.push_back(p_token);
        }

        for (int i = 0; i < msg_num; i++) {
            item_t msg;
            while (!q.try_enqueue(*p_token, std::move(msg)))
                if (q.enqueue(*p_token, std::move(msg)))
                    break;
        }
    };

    std::atomic<int> dequeue_cnt{0};

    auto read_func_with_c_token = [&q, &dequeue_cnt, msg_cnt] {
        moodycamel::ConsumerToken c_token(q);
        item_t msg;
        while (true) {
            if (q.try_dequeue(c_token, msg)) {
                ++dequeue_cnt;
            }
            else {
                if (dequeue_cnt.load(std::memory_order_relaxed) == msg_cnt) 
                    break;
            }
        }
    };

    auto read_func_without_token = [&q, &dequeue_cnt, msg_cnt] {
        item_t msg;
        while (true) {
            if (q.try_dequeue(msg)) {
                ++dequeue_cnt;
            }
            else {
                if (dequeue_cnt.load(std::memory_order_relaxed) == msg_cnt) 
                    break;
            }
        }
    };

    std::atomic<size_t> tokens_idx{0};
    auto read_func_with_p_token = [&q,
                                   &p_tokens,
                                   &tokens_idx,
                                   &p_tokens_mutex,
                                   &dequeue_cnt,
                                   msg_cnt] {
        size_t idx = tokens_idx.fetch_add(1, std::memory_order_relaxed);
        while (p_tokens.size() == 0) {
            continue;
        }
        std::shared_ptr<moodycamel::ProducerToken> p_token;
        {
            std::lock_guard<std::mutex> lock(p_tokens_mutex);
            idx = idx % p_tokens.size();
            p_token = p_tokens[idx];
        }

        item_t msg;
        while (true) {
            if (q.try_dequeue_from_producer(*p_token, msg)) {
                ++dequeue_cnt;
            }
            else {
                if (dequeue_cnt.load(std::memory_order_relaxed) == msg_cnt) {
                    break;
                }
                else {
                    idx = tokens_idx.fetch_add(1, std::memory_order_relaxed);
                    {
                        std::lock_guard<std::mutex> lock(p_tokens_mutex);
                        idx = idx % p_tokens.size();
                        p_token = p_tokens[idx];
                    }
                }
            }
        }
    };
    
    int msg_cnt_per_thread = msg_cnt / p_thread_cnt;
    int msg_cnt_mod = msg_cnt % p_thread_cnt;
    std::vector<std::thread> threads;

    auto start_tp = learnlog::sys_clock::now();
    
    switch (token_type) {
        case no_token:
            threads.emplace_back(write_func_without_token, 
                                 msg_cnt_per_thread + msg_cnt_mod);
            for (int i = 1; i < p_thread_cnt; i++) {
                threads.emplace_back(write_func_without_token,
                                     msg_cnt_per_thread);
            }
            for (int i = 0; i < c_thread_cnt; i++) {
                threads.emplace_back(read_func_without_token);
            }
            break;
        case only_consumer:
            threads.emplace_back(write_func_without_token, 
                                 msg_cnt_per_thread + msg_cnt_mod);
            for (int i = 1; i < p_thread_cnt; i++) {
                threads.emplace_back(write_func_without_token,
                                     msg_cnt_per_thread);
            }
            for (int i = 0; i < c_thread_cnt; i++) {
                threads.emplace_back(read_func_with_c_token);
            }
            break;
        case producer_consumer:
            threads.emplace_back(write_func_with_p_token, 
                                 msg_cnt_per_thread + msg_cnt_mod);
            for (int i = 1; i < p_thread_cnt; i++) {
                threads.emplace_back(write_func_with_p_token,
                                     msg_cnt_per_thread);
            }
            for (int i = 0; i < c_thread_cnt; i++) {
                threads.emplace_back(read_func_with_c_token);
            }
            break;
        case only_producer:
            threads.emplace_back(write_func_with_p_token, 
                                 msg_cnt_per_thread + msg_cnt_mod);
            for (int i = 1; i < p_thread_cnt; i++) {
                threads.emplace_back(write_func_with_p_token,
                                     msg_cnt_per_thread);
            }
            for (int i = 0; i < c_thread_cnt; i++) {
                threads.emplace_back(read_func_with_p_token);
            }
            break;
        default: 
            assert(false);
    }
    for (auto& t : threads) {
        t.join();
    }

    auto delta = learnlog::sys_clock::now() - start_tp;
    return static_cast<double>(delta.count()) / 1e6;
}
