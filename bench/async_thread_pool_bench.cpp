#include "learnlog.h"
#include "sinks/basic_file_sink.h"


template <typename Threadpool>
void bench_msg(int q_size,
               const std::vector<int>& msg_ratios,
               int pthread_num,
               int cthread_num,
               const std::string& filename,
               std::string&& logger_name);

template <typename Threadpool>
void bench_pthread(int q_size,
                   size_t msg_num,
                   const std::vector<int>& pthread_nums,
                   int cthread_num,
                   const std::string& filename,
                   std::string&& logger_name);

template <typename Threadpool>
void bench_cthread(int q_size,
                   size_t msg_num,
                   int pthread_num,
                   const std::vector<int>& cthread_nums,
                   const std::string& filename,
                   std::string&& logger_name);

int main(int argc, char *argv[]) {
    int q_size = 8192;
    int iters = 3;
    int msg_num = 1000000;
    int pthread_num = 8;
    int cthread_num = 8;

    try {
        learnlog::set_global_pattern("[%^%l%$] %v");
        learnlog::set_global_log_level(learnlog::level::debug);

        if (argc > 1) {
            q_size = atoi(argv[1]);
            if (q_size < 512) {
                learnlog::warn("Message queue size should be at least 512 !");
                q_size = 512;
            }
        }
        if (argc > 2) {
            iters = atoi(argv[2]);
        }
        if (argc > 3) {
            msg_num = atoi(argv[3]);
        }
        if (argc > 4) {
            pthread_num = atoi(argv[4]);
        }
        if (argc > 5) {
            cthread_num = atoi(argv[5]);
        }
        if (argc > 6) {
            learnlog::error(
                "Unknown args! Usage: {} <queue_size> <iter_times> <msg_num> <pthread_num> <cthread_num>", argv[0]);
            return 0;
        }

        std::vector<int> msg_fill_ratios{1, 4, 16, 64, 256};
        std::vector<int> pthread_nums{1, 2, 4, 6, 8};
        std::vector<int> cthread_nums{1, 2, 4, 6, 8};
#ifdef _WIN32
        learnlog::filename_t fname = L"bench_tmp/async_thread_pool.log";
#else
        learnlog::filename_t fname = "bench_tmp/async_thread_pool.log";
#endif

        learnlog::info("*********************************");
        learnlog::info("Change total messages count (throughput | msg/ms)");
        learnlog::info("*********************************");
        learnlog::info("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        learnlog::info("Message queue size  : {:L}", q_size);
        learnlog::info("Iterations          : {:L}", iters);
        learnlog::info("Produce threads     : {:L}", pthread_num);
        learnlog::info("Consume threads     : {:L}", cthread_num);
        learnlog::info("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        learnlog::debug("tp: thread pool");
        learnlog::debug("msg_num: total messages number");
        
        for (int i = 1; i <= iters; i++) {
            learnlog::info("~~~~~~~~~~~~~~~~~~~");
            learnlog::info("Iteration: {:d}", i);
            learnlog::info("~~~~~~~~~~~~~~~~~~~");
            auto it = msg_fill_ratios.rbegin();
            learnlog::info("-------------------------------------------------");
            learnlog::info("{:24s}|{:<8L}*{:<4d}|{:<8L}*{:<4d}|{:<8L}*{:<4d}|{:<8L}*{:<4d}|{:<8L}*{:<4d}",
                           "tp\\msg_num",
                           q_size, *(it++), q_size, *(it++), q_size, *(it++), q_size, *(it++), q_size, *(it++));
            learnlog::info("-------------------------------------------------");
            
            bench_msg<learnlog::base::lock_thread_pool>(q_size, msg_fill_ratios, pthread_num, cthread_num, fname, "lock");
            bench_msg<learnlog::base::lockfree_thread_pool>(q_size, msg_fill_ratios, pthread_num, cthread_num, fname, "lockfree");
            bench_msg<learnlog::base::lockfree_concurrent_thread_pool>(q_size, msg_fill_ratios, pthread_num, cthread_num, fname, "lockfree_concurrent");
        }

        learnlog::debug("\n");
        learnlog::info("*********************************");
        learnlog::info("Change produce threads count (throughput | msg/ms)");
        learnlog::info("*********************************");
        learnlog::info("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        learnlog::info("Message queue size          : {:L}", q_size);
        learnlog::info("Iterations                  : {:L}", iters);
        learnlog::info("Total messages              : {:L}", msg_num);
        learnlog::info("Consume threads             : {:L}", cthread_num);
        learnlog::info("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        learnlog::debug("tp: thread pool");
        learnlog::debug("p_thd_num: produce threads(writers) number");
        
        for (int i = 1; i <= iters; i++) {
            learnlog::info("~~~~~~~~~~~~~~~~~~~");
            learnlog::info("Iteration: {}", i);
            learnlog::info("~~~~~~~~~~~~~~~~~~~");
            auto it = pthread_nums.rbegin();
            learnlog::info("-------------------------------------------------");
            learnlog::info("{:24s}| {:<12d}| {:<12d}| {:<12d}| {:<12d}| {:<12d}",
                           "tp\\p_thd_num",
                           *(it++), *(it++), *(it++), *(it++), *(it++));
            learnlog::info("-------------------------------------------------");

            bench_pthread<learnlog::base::lock_thread_pool>(q_size, msg_num, pthread_nums, cthread_num, fname, "lock");
            bench_pthread<learnlog::base::lockfree_thread_pool>(q_size, msg_num, pthread_nums, cthread_num, fname, "lockfree");
            bench_pthread<learnlog::base::lockfree_concurrent_thread_pool>(q_size, msg_num, pthread_nums, cthread_num, fname, "lockfree_concurrent");
        }

        learnlog::debug("\n");
        learnlog::info("*********************************");
        learnlog::info("Change consume threads count (throughput | msg/ms)");
        learnlog::info("*********************************");
        learnlog::info("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        learnlog::info("Message queue size          : {:L}", q_size);
        learnlog::info("Iterations                  : {:L}", iters);
        learnlog::info("Total messages              : {:L}", msg_num);
        learnlog::info("Produce threads             : {:L}", pthread_num);
        learnlog::info("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        learnlog::debug("tp: thread pool");
        learnlog::debug("c_thd_num: consume threads(readers) number");
        
        for (int i = 1; i <= iters; i++) {
            learnlog::info("~~~~~~~~~~~~~~~~~~~");
            learnlog::info("Iteration: {}", i);
            learnlog::info("~~~~~~~~~~~~~~~~~~~");
            auto it = cthread_nums.rbegin();
            learnlog::info("-------------------------------------------------");
            learnlog::info("{:24s}| {:<12d}| {:<12d}| {:<12d}| {:<12d}| {:<12d}",
                           "tp\\c_thd_num",
                           *(it++), *(it++), *(it++), *(it++), *(it++));
            learnlog::info("-------------------------------------------------");

            bench_cthread<learnlog::base::lock_thread_pool>(q_size, msg_num, pthread_num, cthread_nums, fname, "lock");
            bench_cthread<learnlog::base::lockfree_thread_pool>(q_size, msg_num, pthread_num, cthread_nums, fname, "lockfree");
            bench_cthread<learnlog::base::lockfree_concurrent_thread_pool>(q_size, msg_num, pthread_num, cthread_nums, fname, "lockfree_concurrent");
        }
    }
    LEARNLOG_CATCH

    return 0;
}

size_t bench_(size_t msg_num, learnlog::logger_shr_ptr logger, int pthread_num) {
    auto thread_func = [&logger] (size_t msg_num_pt) {
        for (size_t i = 0; i < msg_num_pt; i++) {
            logger->info("message #{}", i);
        }
    };
    
    size_t msg_num_per_thread = msg_num / pthread_num;
    size_t msg_num_mod = msg_num % pthread_num;
    std::vector<std::thread> threads;

    auto start_tp = learnlog::sys_clock::now();

    threads.emplace_back(thread_func, msg_num_per_thread + msg_num_mod);
    for (int i = 1; i < pthread_num; i++) {
        threads.emplace_back(thread_func, msg_num_per_thread);
    }
    for (auto& t : threads) {
        t.join();
    }

    auto delta = learnlog::sys_clock::now() - start_tp;
    return msg_num * 1000000 / delta.count();
}

template <typename Threadpool>
void bench_msg(int q_size,
               const std::vector<int>& msg_ratios,
               int pthread_num,
               int cthread_num,
               const std::string& filename,
               std::string&& logger_name) {
    auto tp = std::make_shared<Threadpool>(q_size, cthread_num);
    auto sink = std::make_shared<learnlog::sinks::basic_file_sink_mt>(filename, true);
    auto logger = std::make_shared<learnlog::async_logger>( logger_name,
                                                            std::move(sink),
                                                            std::move(tp));
    logger->set_pattern("[%n]: %v");
    std::vector<size_t> throughputs;
    for (auto &fill_ratio : msg_ratios) {
        throughputs.push_back(bench_(q_size * fill_ratio, logger, pthread_num));
    }
    learnlog::info( "{:24s}| {:<12L}| {:<12L}| {:<12L}| {:<12L}| {:<12L}",
                    std::move(logger_name),
                    throughputs[0],
                    throughputs[1],
                    throughputs[2],
                    throughputs[3],
                    throughputs[4]);
}

template <typename Threadpool>
void bench_pthread(int q_size,
                   size_t msg_num,
                   const std::vector<int>& pthread_nums,
                   int cthread_num,
                   const std::string& filename,
                   std::string&& logger_name) {
    auto tp = std::make_shared<Threadpool>(q_size, cthread_num);
    auto sink = std::make_shared<learnlog::sinks::basic_file_sink_mt>(filename, true);
    auto logger = std::make_shared<learnlog::async_logger>( logger_name,
                                                            std::move(sink),
                                                            std::move(tp));
    logger->set_pattern("[%n]: %v");
    std::vector<size_t> throughputs;
    for (auto &thread_num : pthread_nums) {
        throughputs.push_back(bench_(msg_num, logger, thread_num));
    }
    learnlog::info( "{:24s}| {:<12L}| {:<12L}| {:<12L}| {:<12L}| {:<12L}",
                    std::move(logger_name),
                    throughputs[0],
                    throughputs[1],
                    throughputs[2],
                    throughputs[3],
                    throughputs[4]);
}

template <typename Threadpool>
void bench_cthread(int q_size,
                   size_t msg_num,
                   int pthread_num,
                   const std::vector<int>& cthread_nums,
                   const std::string& filename,
                   std::string&& logger_name) {
    std::vector<size_t> throughputs;
    for (auto &thread_num : cthread_nums) {
        auto tp = std::make_shared<Threadpool>(q_size, thread_num);
        auto sink = std::make_shared<learnlog::sinks::basic_file_sink_mt>(filename, true);
        auto logger = std::make_shared<learnlog::async_logger>(logger_name, 
                                                               std::move(sink), 
                                                               std::move(tp));
        logger->set_pattern("[%n]: %v");
        throughputs.push_back(bench_(msg_num, std::move(logger), pthread_num));
    }
    learnlog::info( "{:24s}| {:<12L}| {:<12L}| {:<12L}| {:<12L}| {:<12L}",
                    std::move(logger_name),
                    throughputs[0],
                    throughputs[1],
                    throughputs[2],
                    throughputs[3],
                    throughputs[4]);
}