#pragma once

#include "logger.h"
#include "sinks/sink.h"
#include "base/thread_pool.h"

#include <future>

namespace learnlog {

class async_logger final : public std::enable_shared_from_this<async_logger>, 
                           public logger {
public:
    template <typename It>
    async_logger(std::string logger_name,
                 It begin,
                 It end,
                 std::weak_ptr<base::thread_pool> tp)
        : logger{std::move(logger_name), begin, end},
          thread_pool_(std::move(tp)) {
        logger::is_async_ = true;
    }

    async_logger(std::string logger_name,
                 sinks_init_list sinks_list,
                 std::weak_ptr<base::thread_pool> tp)
        : logger{std::move(logger_name), sinks_list.begin(), sinks_list.end()},
          thread_pool_(std::move(tp)) {
        logger::is_async_ = true;
    }

    async_logger(std::string logger_name,
                 sink_shr_ptr single_sink,
                 std::weak_ptr<base::thread_pool> tp)
        : logger{std::move(logger_name), std::move(single_sink)},
          thread_pool_(std::move(tp)) {
        logger::is_async_ = true;
    }

    logger_shr_ptr clone(std::string new_name) override {
        auto cloned = std::make_shared<async_logger>(*this);
        cloned->name_ = std::move(new_name);
        return cloned;
    }

private:
    void sink_log_(const base::log_msg& msg) override {
        try {
            auto tp_ptr = thread_pool_.lock();
            if (tp_ptr == nullptr) {
                throw_learnlog_excpt(
                    "learnlog::async_logger: sink_log_() failed: thread pool does not exist");
            }
            tp_ptr->enqueue_log(shared_from_this(), msg);
        }
        LEARNLOG_CATCH
    }

    void flush_sink_() override {
        try {
            auto tp_ptr = thread_pool_.lock();
            if (tp_ptr == nullptr) {
                throw_learnlog_excpt(
                    "learnlog::async_logger: flush_sink_() failed: thread pool does not exist");
            }
            std::future<void> future = tp_ptr->enqueue_flush(shared_from_this());
            future.get();
        }
        LEARNLOG_CATCH
    }

    friend class base::thread_pool;
    
    void do_sink_log_(const base::log_msg& msg_popped) {
        for (auto &sink : sinks_) {
            if (sink->should_log(msg_popped.level)) {
                try { sink->log(msg_popped); }
                LEARNLOG_CATCH
            }
        }

        if (should_flush_(msg_popped.level)) {
            flush_sink_();
        }
    }
    
    void do_flush_sink_() {
        for(auto &sink : sinks_) {
            try { sink->flush(); }
            LEARNLOG_CATCH
        }
    }

    std::weak_ptr<base::thread_pool> thread_pool_;
};

}   // namespace learnlog