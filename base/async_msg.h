#pragma once

#include "base/log_msg_buf.h"
#include <future>

namespace learnlog {

enum async_overflow_method {
    discard_new,
    block_wait,
    override_old
};

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
              const log_msg& msg_in);

    // flush
    async_msg(async_logger_shr_ptr&& logger_in, 
              async_msg_type type_in,
              std::promise<void>&& promise_in);

    // terminate
    async_msg(async_logger_shr_ptr&& logger_in, 
              async_msg_type type_in);

    explicit async_msg(async_msg_type type_in);
};

}   // namespace base
}   // namespace learnlog