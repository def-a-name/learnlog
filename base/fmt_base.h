#pragma once

#include "definitions.h"

namespace learnlog {
namespace base {
namespace fmt_base {

// 将 fmt_string_view 类型的字符串加入 dest_buf
inline void append_string_view(fmt_string_view str_view, fmt_memory_buf& dest_buf) {
    dest_buf.append(str_view.data(), str_view.data() + str_view.size());
}

// 将整数 n 以字符串形式加入 dest_buf
template <typename T>
inline void append_int(T n, fmt_memory_buf& dest_buf) {
    fmt::format_int i(n);
    dest_buf.append(i.data(), i.data() + i.size());
}

// 将 n 转换为无符号整型，再计算位数
template <typename T>
inline size_t count_unsigned_digits(T n) {
    using T_type =
        typename std::conditional<(sizeof(T) > sizeof(uint32_t)), uint64_t, uint32_t>::type;
    return static_cast<size_t>(fmt::detail::count_digits( static_cast<T_type>(n) ));
}

// 将无符号整数 n 填入以 '0' 为前缀，长度为 fill_len 的字符串中，再加入 dest_buf
// n_len 为 n 的位数，加入 dest_buf 的实际长度为 max(n_len, fill_len)
template <typename T>
inline void fill_uint_(T n, size_t n_len, size_t fill_len, fmt_memory_buf& dest_buf) {
    for(size_t i = n_len; i < fill_len; i++) {
        dest_buf.push_back('0');
    }
    append_int(n , dest_buf);
}

// 将 n 作为无符号整数填入以 '0' 为前缀，长度为 fill_len 的字符串中，再加入 dest_buf
// 如果 fill_len <= 3（常见情况），尝试直接通过 push_back 填入，更快
template <typename T>
inline void fill_uint(T n, size_t fill_len, fmt_memory_buf& dest_buf) {
    // static_assert(std::is_unsigned<T>::value, "fill_uint must get unsigned T");

    switch (fill_len) {
        case 2:
            if( n < 100) {
                dest_buf.push_back(static_cast<char>(n / 10 + '0'));
                dest_buf.push_back(static_cast<char>(n % 10 + '0'));
            }
            else {  // 如果 n 的实际位数 > 2，通过 append_int() 加入buf
                append_int(n, dest_buf);
            }
            break;
        case 3:
            if( n < 1000) {
                dest_buf.push_back(static_cast<char>(n / 100 + '0'));
                n = n % 100;
                dest_buf.push_back(static_cast<char>(n / 10 + '0'));
                dest_buf.push_back(static_cast<char>(n % 10 + '0'));
            }
            else {  // 如果 n 的实际位数 > 3，通过 append_int() 加入buf
                append_int(n, dest_buf);
            }
            break;
        default:
            fill_uint_(n, count_unsigned_digits(n), fill_len, dest_buf);
    }
}

// 得到 tp 在秒以下的精确时间
// Metric 可选类型：milliseconds、microseconds、nanoseconds
template <typename Metric>
inline Metric precise_time(sys_clock::time_point tp) {
    using std::chrono::duration_cast;

    auto duration = tp.time_since_epoch();
    auto secs = duration_cast<seconds>(duration);
    return duration_cast<Metric>(duration) - duration_cast<Metric>(secs);
}

}   // namespace fmt_base
}   // namespace base
}   // namespace learnlog