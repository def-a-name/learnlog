#pragma once

#include "base/log_msg_buf.h"
#include <future>

namespace learnlog {

class async_logger;

namespace base {

enum async_msg_type { log, flush, terminate };

class async_msg : public log_msg_buf {
public:
    async_logger_shr_ptr logger;
    async_msg_type msg_type{async_msg_type::log};
    std::promise<void> flush_promise;
    
    async_msg() = default;
    ~async_msg() = default;
    async_msg(const async_msg &) = delete;
    async_msg& operator=(const async_msg &) = delete;
    async_msg(async_msg &&) = default;
    async_msg& operator=(async_msg &&) = default;

    // log
    async_msg(async_logger_shr_ptr&& logger_in, 
              async_msg_type type_in, 
              const log_msg& msg_in)
        : log_msg_buf{msg_in}, 
        logger(std::move(logger_in)),
        msg_type(type_in), 
        flush_promise{} {}

    // flush
    async_msg(async_logger_shr_ptr&& logger_in, 
              async_msg_type type_in,
              std::promise<void>&& promise_in)
        : log_msg_buf{}, 
        logger(std::move(logger_in)),
        msg_type(type_in), 
        flush_promise{std::move(promise_in)} {}

    // terminate
    async_msg(async_logger_shr_ptr&& logger_in, 
              async_msg_type type_in)
        : log_msg_buf{}, 
        logger(std::move(logger_in)),
        msg_type(type_in), 
        flush_promise{} {}

    explicit async_msg(async_msg_type type_in)
        : async_msg(nullptr, type_in) {}
};

}   // namespace base
}   // namespace learnlog