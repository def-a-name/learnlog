#include <catch2/catch_all.hpp>
#include "base/fmt_base.h"

using learnlog::fmt_string_view;
using learnlog::fmt_memory_buf;

template <typename T>
void test_append_int(T n, const char* expected) {
    fmt_memory_buf buf;
    learnlog::base::fmt_base::append_int(n, buf);
    
    REQUIRE(fmt_string_view(buf.data(), buf.size()) == expected);
}

template <typename T>
void test_fill_uint(T n, size_t fill_len, const char* expected) {
    fmt_memory_buf buf;
    learnlog::base::fmt_base::fill_uint(n, fill_len, buf);

    REQUIRE(fmt_string_view(buf.data(), buf.size()) == expected);
}

template <typename T>
void test_count_digits(T n, size_t expected) {
    REQUIRE(learnlog::base::fmt_base::count_unsigned_digits(n) == expected);
}


TEST_CASE("test_append_int", "[fmt_base]") {
    test_append_int<int>(-1, "-1");
    test_append_int<size_t>(32768, "32768");
    test_append_int<long long>(1e10 + 5, "10000000005");
}

TEST_CASE("test_fill_uint", "[fmt_base]") {
    test_fill_uint<unsigned>(1, 2, "01");
    test_fill_uint<unsigned>(1, 3, "001");
    test_fill_uint<unsigned>(1, 6, "000001");
    test_fill_uint<unsigned>(1, 9, "000000001");

    test_fill_uint<size_t>(123, 2, "123");
    test_fill_uint<size_t>(123, 3, "123");
    test_fill_uint<size_t>(123, 4, "0123");
    
    test_fill_uint<unsigned long long>(10000000005, 9, "10000000005");

    // 如果传入负数，则填充长度失效，不会正确填充0，但是能正确填充负数
    test_fill_uint<int>(-1, 9, "-1");   
}

TEST_CASE("test_count_digits", "[fmt_base]") {
    test_count_digits<int>(1, 1);
    test_count_digits<double>(3.14, 1);
    test_count_digits<long long>(1e18 + 5, 19);
}