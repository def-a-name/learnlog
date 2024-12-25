#include <catch2/catch_all.hpp>
#include "definitions.h"
#include "sinks/formatters/pattern_formatter.h"

#include <string>
#include <iostream>

using namespace learnlog;
using namespace sinks;

template <typename... Args>
std::string get_format_str(const std::string& text, Args &&...args) {
    std::unique_ptr<formatter> p = learnlog::make_unique<pattern_formatter>(args...);
    source_loc loc("learnlog.cpp", 1, "helloworld");
    base::log_msg msg(sys_clock::now(), loc, level::level_enum::debug, text, "test_logger");

    fmt_memory_buf buf;
    p->format(msg, buf);
    return std::string(buf.data(), buf.size());
}

TEST_CASE("eol", "[pattern_formatter]") {
    std::string eol = ":)";
    std::string msg = "hello!";
    REQUIRE(get_format_str(msg, "%v", eol) == msg + eol);
}

TEST_CASE("empty_pattern", "[pattern_formatter]") {
    REQUIRE(get_format_str("hello", "", "\n") == "\n");
}

TEST_CASE("spaces_limit", "[pattern_formatter]") {
    REQUIRE(get_format_str("hello", "%-64v") == get_format_str("hello", "%-200v"));
}

TEST_CASE("color_range", "[pattern_formatter]") {
    std::unique_ptr<formatter> p = learnlog::make_unique<pattern_formatter>("hello %^world%$");
    base::log_msg msg(sys_clock::now(), source_loc{}, level::level_enum::debug, "", "test_logger");
    fmt_memory_buf buf;
    p -> format(msg, buf);

    REQUIRE(msg.color_index_start == 6);
    REQUIRE(msg.color_index_end == 11);
}

TEST_CASE("clone", "[pattern_formatter]") {
    auto f1 = std::make_unique<pattern_formatter>("%+ [%]");
    auto f2 = f1->clone();
    source_loc loc("learnlog.cpp", 1, "helloworld");
    base::log_msg msg(sys_clock::now(), loc, level::level_enum::debug, "message", "test_logger");

    fmt_memory_buf buf1;
    fmt_memory_buf buf2;
    f1->format(msg, buf1);
    f2->format(msg, buf2);

    REQUIRE(fmt_string_view(buf1.data(), buf1.size()) == fmt_string_view(buf2.data(), buf2.size()));
}

// 自定义格式字符
class custom_formatter_test : public custom_flag_formatter {
public:
    void format(const base::log_msg& msg, const std::tm&, fmt_memory_buf& dest_buf) override {
        filler f(5, spaces_info_, dest_buf);
        f.fill_msg(fmt_string_view{msg.msg.begin(), 5});
    }
    std::unique_ptr<custom_flag_formatter> clone() const {
        return learnlog::make_unique<custom_formatter_test>();
    }
};

TEST_CASE("custom_formatter", "[pattern_formatter]") {
    std::unique_ptr<pattern_formatter> p = learnlog::make_unique<pattern_formatter>("%5!v");
    std::string text = "hello world";
    
    base::log_msg msg(level::level_enum::info, text, "test_logger");
    fmt_memory_buf buf1;
    p->format(msg, buf1);
    
    p->add_custom_flag<custom_formatter_test>('a');
    p->set_pattern("%a");
    fmt_memory_buf buf2;
    p->format(msg, buf2);
    
    REQUIRE(fmt_string_view(buf1.data(), buf1.size()) == fmt_string_view(buf2.data(), buf2.size()));
}

TEST_CASE("clone_custom_formatter", "[pattern_formatter]") {
    auto f1 = std::make_unique<pattern_formatter>("");
    f1->add_custom_flag<custom_formatter_test>('#');
    f1->set_pattern("%c %#");

    auto f2 = f1->clone();
    source_loc loc("learnlog.cpp", 1, "helloworld");
    base::log_msg msg(sys_clock::now(), loc, level::level_enum::debug, "message", "test_logger");

    fmt_memory_buf buf1;
    fmt_memory_buf buf2;
    f1->format(msg, buf1);
    f2->format(msg, buf2);

    REQUIRE(fmt_string_view(buf1.data(), buf1.size()) == fmt_string_view(buf2.data(), buf2.size()));
}

TEST_CASE("pattern_stdout", "[pattern_formatter]") {
    std::cout << get_format_str("==== pattern formatter tests ====", "%=64v");
    std::cout << get_format_str("0", "pattern='': ");
    std::cout << get_format_str("1", "pattern='%%c': %c");
    std::cout << get_format_str("2", "pattern='%%T.%%3!G': %T.%3!G");
    std::cout << get_format_str("3", "pattern='%%T.%%G': %T.%G");
    std::cout << get_format_str("4", "pattern='pid: %%p; tid: %%t': pid: %p; tid: %t");
    std::cout << get_format_str("5", "pattern='source_loc: %%@ - %%C': source_loc: %@ - %C");
    std::cout << get_format_str("6", "pattern='%%+': %+");
    std::cout << get_format_str("7", "");
}