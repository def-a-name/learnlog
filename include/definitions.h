#pragma once

#ifdef LEARNLOG_USE_TLS
    #if (defined(_MSC_VER) && _MSC_VER < 1900) || (defined(__MINGW32__) || defined(__MINGW64__) && defined(__WINPTHREADS_VERSION)) || (defined(__GNUC__) && __GNUC__ <= 4)
        #define LEARNLOG_USE_TLS 0
    #endif
#endif

#ifdef LEARNLOG_USE_BUNDLED_FMT
    #include "base/fmt/format.h"
#else
    #include <fmt/format.h>
#endif

#include <chrono>
#include <memory>
#include <initializer_list>
#include <type_traits>

namespace learnlog {

// system_clock
using sys_clock = std::chrono::system_clock;
using seconds = std::chrono::seconds;
using milliseconds = std::chrono::milliseconds;
using microseconds = std::chrono::microseconds;
using nanoseconds = std::chrono::nanoseconds;

// fmt 库
using fmt_string_view = fmt::basic_string_view<char>;
using fmt_memory_buf = fmt::basic_memory_buffer<char, 250>;
using fmt_wstring_view = fmt::basic_string_view<wchar_t>;
using fmt_wmemory_buf = fmt::basic_memory_buffer<wchar_t, 250>;
template <typename... Args>
using fmt_format_string = fmt::format_string<Args...>;

// logger
class logger;
using logger_shr_ptr = std::shared_ptr<logger>;
class async_logger;
using async_logger_shr_ptr = std::shared_ptr<async_logger>;

// sinks
namespace sinks {
class sink;
}
using sink_shr_ptr = std::shared_ptr<sinks::sink>;
using sinks_init_list = std::initializer_list<sink_shr_ptr>;

// formatter
namespace sinks {
class formatter;
}
using formatter_uni_ptr = std::unique_ptr<sinks::formatter>;
#define MAX_SPACES_LEN 64

// thread_pool
namespace base {
class thread_pool;
}
using thread_pool_shr_ptr = std::shared_ptr<base::thread_pool>;

// c++14 前需要定义 make_unique
#if __cplusplus >= 201402L
    using std::make_unique;
#else
    template <typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args &&...args) {
        static_assert(!std::is_array<T>::value, "arrays not supported");
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
#endif

// 换行符
#if !defined(DEFAULT_EOL)
    #ifdef _WIN32
        #define DEFAULT_EOL "\r\n"
    #else
        #define DEFAULT_EOL "\n"
    #endif
#endif

// 路径分隔符
#if !defined(FOLDER_SPR)
    #ifdef _WIN32
        #define FOLDER_SPR L"\\/"
    #else
        #define FOLDER_SPR "/"
    #endif
#endif

// 路径字符串类型
#if !defined(filename_t)
    #ifdef _WIN32
        using filename_t = std::wstring;
    #else
        using filename_t = std::string;
    #endif
#endif

// long long 整数类型
#if !defined(long_long)
    #ifdef _WIN32
        using long_long = long long;
    #else
        using long_long = __int64_t;
    #endif
#endif

// unsigned long long 整数类型
#if !defined(u_long_long)
    #ifdef _WIN32
        using u_long_long = unsigned long long;
    #else
        using u_long_long = __uint64_t;
    #endif
#endif

// 使用模板类时的类型判断
template <typename T>
using remove_cvref_type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

template <typename T>
struct is_convertible_to_basic_format_string
    : std::integral_constant<bool,
                             std::is_convertible<T, fmt::basic_string_view<char> >::value ||
                             std::is_same<remove_cvref_type<T>, fmt::runtime_format_string<char> >::value> {
};

template <typename T>
struct is_unsigned_integral
    : std::integral_constant<bool,
                             std::is_integral<T>::value &&
                             std::is_unsigned<T>::value > {
};

template <typename T>
struct is_signed_integral
    : std::integral_constant<bool,
                             std::is_integral<T>::value &&
                             std::is_signed<T>::value > {
};

// 保存发生位置信息的结构体，代码文件名 -> 行号 -> 函数名
struct source_loc {
    constexpr source_loc() = default;
    constexpr source_loc(const char* filename_in, int line_in, const char* funcname_in=nullptr)
        : filename{filename_in},
          line{line_in},
          funcname{funcname_in} {}

    constexpr bool empty() const noexcept { return line <= 0; }

    const char* filename{nullptr};
    int line{0};
    const char* funcname{nullptr};
};


#define LEARNLOG_LEVEL_TRACE 0
#define LEARNLOG_LEVEL_DEBUG 1
#define LEARNLOG_LEVEL_INFO 2
#define LEARNLOG_LEVEL_WARN 3
#define LEARNLOG_LEVEL_ERROR 4
#define LEARNLOG_LEVEL_CRITICAL 5
#define LEARNLOG_LEVEL_OFF 6
#define LEARNLOG_LEVELS_NUM 7
// 日志等级枚举
namespace level {

enum level_enum : int {
    trace = LEARNLOG_LEVEL_TRACE,
    debug = LEARNLOG_LEVEL_DEBUG,
    info = LEARNLOG_LEVEL_INFO,
    warn = LEARNLOG_LEVEL_WARN,
    error = LEARNLOG_LEVEL_ERROR,
    critical = LEARNLOG_LEVEL_CRITICAL,
    off = LEARNLOG_LEVEL_OFF,
};

const fmt_string_view level_name[7] = { {"trace"}, {"debug"}, {"info"}, {"warn"}, 
                                        {"error"}, {"critical"}, {"off"} };

}   // namespace level

}   // namespace learnlog