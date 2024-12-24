#include "async_msg.h"
#include <functional>

using namespace mylog;
using namespace base;

async_msg::async_msg(async_logger_shr_ptr&& logger_in, 
                     async_msg_type type_in, 
                     const log_msg& msg_in)
    : log_msg_buf{msg_in}, 
      logger(std::move(logger_in)),
      msg_type(type_in), 
      flush_promise{} {}

async_msg::async_msg(async_logger_shr_ptr&& logger_in, 
                     async_msg_type type_in,
                     std::promise<void>&& promise_in) 
    : log_msg_buf{}, 
      logger(std::move(logger_in)),
      msg_type(type_in), 
      flush_promise{std::move(promise_in)} {}

async_msg::async_msg(async_logger_shr_ptr&& logger_in, 
                     async_msg_type type_in) 
    : log_msg_buf{}, 
      logger(std::move(logger_in)),
      msg_type(type_in), 
      flush_promise{} {}

async_msg::async_msg(async_msg_type type_in) 
    : async_msg(nullptr, type_in) {}