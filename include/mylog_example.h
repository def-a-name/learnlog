#pragma once

#include "definitions.h"
#include "sinks/formatters/filler.h"
#include "sinks/formatters/flag_formatter.h"

namespace mylog {

void helloworld();

void create_thread_pool();

void handle_exception(const std::string& msg);

void filler_helloworld(fmt_memory_buf& dest_buf);

void pattern_formatter_helloworld(fmt_memory_buf& dest_buf);

class custom_formatter_test : public sinks::custom_flag_formatter {
public:
    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        sinks::filler f(6, spaces_info_, dest_buf);
        f.fill_msg(fmt_string_view{msg.msg.begin(), 6});
    }
    std::unique_ptr<custom_flag_formatter> clone() const {
        return mylog::make_unique<custom_formatter_test>();
    }
};

void custom_formatter_hello(fmt_memory_buf& dest_buf);

void print_fmt_buf(fmt_memory_buf& buf);

void logger_helloworld();

void async_helloworld();

}   // namespace mylog