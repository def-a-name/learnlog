#include <catch2/catch_all.hpp>
#include "sinks/formatters/filler.h"

using learnlog::fmt_string_view;
using learnlog::fmt_memory_buf;
using learnlog::sinks::spaces_info;

TEST_CASE("fill_spaces", "[filler]") {
    learnlog::sinks::spaces_info s_info(4, spaces_info::fill_side::center, false);
    unsigned n = 1;

    fmt_memory_buf buf;
    {
        learnlog::sinks::filler filler(0, s_info, buf);
    }
    REQUIRE(fmt_string_view(buf.data(), buf.size()) == "    ");

    buf.clear();
    {
        learnlog::sinks::filler filler(1, s_info, buf);
        filler.fill_msg(n);
    }
    REQUIRE(fmt_string_view(buf.data(), buf.size()) == " 1  ");

    buf.clear();
    {
        learnlog::sinks::filler filler(2, s_info, buf);
        filler.fill_msg(n);
    }
    REQUIRE(fmt_string_view(buf.data(), buf.size()) == " 01 ");

    buf.clear();
    {
        learnlog::sinks::filler filler(4, s_info, buf);
        filler.fill_msg(n);
    }
    REQUIRE(fmt_string_view(buf.data(), buf.size()) == "0001");
}

TEST_CASE("spaces_info", "[filler]") {
    learnlog::sinks::spaces_info s_info(4, spaces_info::fill_side::left, false);
    int n = 1;
    unsigned int n_trunc = 1111;

    fmt_memory_buf buf;
    {
        learnlog::sinks::filler filler(1, s_info, buf);
        filler.fill_msg(n);
    }
    REQUIRE(fmt_string_view(buf.data(), buf.size()) == "   1");

    buf.clear();
    s_info.side_ = spaces_info::fill_side::right;
    {
        learnlog::sinks::filler filler(1, s_info, buf);
        filler.fill_msg(n);
    }
    REQUIRE(fmt_string_view(buf.data(), buf.size()) == "1   ");

    buf.clear();
    s_info.side_ = spaces_info::fill_side::center;
    {
        learnlog::sinks::filler filler(1, s_info, buf);
        filler.fill_msg(n);
    }
    REQUIRE(fmt_string_view(buf.data(), buf.size()) == " 1  ");

    // 构造 filler 时传入的 msg 长度要与实际填入的 msg 长度一致，
    // 若 参数msg_len < 实际msg_len，填充出的字符串长度会长于预期；
    // 若 参数msg_len > 实际msg_len，可能会截断 msg ；
    buf.clear();
    {
        learnlog::sinks::filler filler(0, s_info, buf);
        filler.fill_msg(n_trunc);
    }
    REQUIRE(fmt_string_view(buf.data(), buf.size()) == "  1111  ");

    buf.clear();
    s_info.truncate_ = true;
    s_info.target_len_ = 2;
    {
        learnlog::sinks::filler filler(6, s_info, buf);
        filler.fill_msg(n_trunc);
    }
    REQUIRE(fmt_string_view(buf.data(), buf.size()) == "00");

    buf.clear();
    {
        learnlog::sinks::filler filler(4, s_info, buf);
        filler.fill_msg(n_trunc);
    }
    REQUIRE(fmt_string_view(buf.data(), buf.size()) == "11");
}


TEST_CASE("fill_msg", "[filler]") {
    learnlog::sinks::spaces_info s_info(6, spaces_info::fill_side::center, false);
    fmt_memory_buf buf;
    int n = 2024;
    
    {
        learnlog::sinks::filler filler(5, s_info, buf);
        filler.fill_msg(static_cast<short>(-n));
    }
    REQUIRE(fmt_string_view(buf.data(), buf.size()) == "-2024 ");

    buf.clear();
    {
        learnlog::sinks::filler filler(6, s_info, buf);
        filler.fill_msg(static_cast<unsigned long long>(n));
    }
    REQUIRE(fmt_string_view(buf.data(), buf.size()) == "002024");
    
    buf.clear();
    {
        learnlog::sinks::filler filler(4, s_info, buf);
        filler.fill_msg(std::to_string(n));
    }
    REQUIRE(fmt_string_view(buf.data(), buf.size()) == " 2024 ");
}