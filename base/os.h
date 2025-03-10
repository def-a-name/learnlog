#pragma once

#include "definitions.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <sys/stat.h>
#include <thread>
#include <ctime>
#include <array>

#ifdef _WIN32
    #include "win.h"
    #include "io.h"

    #ifdef __MINGW32__
        #include <share.h>
    #endif

#else  // unix
    #include <fcntl.h>
    #include <unistd.h>

    #ifdef __linux__
        #include <sys/syscall.h>  //gettid() syscall
    #endif

#endif

namespace learnlog {
namespace base {
namespace os {

// ====================================time=========================================

inline std::tm _time_t_to_tm(const std::time_t& time_tt) noexcept {
#ifdef _WIN32
    std::tm tm;
    ::localtime_s(&tm, &time_tt);
#else
    std::tm tm;
    ::localtime_r(&time_tt, &tm);
#endif
    return tm;
}

inline std::time_t time_point_to_time_t(const sys_clock::time_point& tp) noexcept {
    return sys_clock::to_time_t(tp);
}

inline std::tm time_point_to_tm(const sys_clock::time_point& tp) noexcept {
    return _time_t_to_tm(time_point_to_time_t(tp));
}

inline void time_point_to_datetime_sec(char* dt_buf, size_t buf_len, const sys_clock::time_point& tp) noexcept {
    std::tm tm = time_point_to_tm(tp);
    std::strftime(dt_buf, buf_len, "%Y-%m-%d %H:%M:%S", &tm);
}

inline void time_point_to_datetime_nsec(char* dt_buf, size_t buf_len, const sys_clock::time_point& tp) noexcept {
    using std::chrono::duration_cast;
    
    auto duration = tp.time_since_epoch();
    auto secs = duration_cast<seconds>(duration);
    nanoseconds ns = duration_cast<nanoseconds>(duration) - duration_cast<nanoseconds>(secs);
    char ns_str[10];
    snprintf(ns_str, sizeof(ns_str), "%09lld", static_cast<long long>( ns.count() ));
    ns_str[9] = '\0';

    std::tm tm = time_point_to_tm(tp);
    std::strftime(dt_buf, buf_len, "%Y-%m-%d %H:%M:%S.", &tm);

    snprintf(dt_buf, buf_len, "%s%s", dt_buf, ns_str);
}

inline void sleep_for_ms(size_t ms) noexcept {
#ifdef _WIN32
    ::Sleep(static_cast<DWORD>(ms));
#else
    std::this_thread::sleep_for(microseconds{ms});
#endif
}

// ====================================time=========================================

// ====================================terminal=====================================

inline bool in_terminal(FILE *file) noexcept {
#ifdef _WIN32
    return ::_isatty(_fileno(file)) != 0;
#else
    return ::isatty(fileno(file)) != 0;
#endif
}

inline bool is_color_terminal() noexcept {
#ifdef _WIN32
    return true;
#else
    static const bool result = []() {
        const char *env_colorterm = std::getenv("COLORTERM");
        if (env_colorterm != nullptr) {
            return true;
        }

        static constexpr std::array<const char *, 16> terms = {
            {"ansi", "color", "console", "cygwin", "gnome", "konsole", "kterm", "linux", "msys",
             "putty", "rxvt", "screen", "vt100", "xterm", "alacritty", "vt102"}};

        const char *env_term = std::getenv("TERM");
        if (env_term == nullptr) {
            return false;
        }

        return std::any_of(terms.begin(), terms.end(), [&](const char *term) {
            return std::strstr(env_term, term) != nullptr;
        });
    };

    return result;
#endif
}

// ====================================terminal=====================================

// ====================================id===========================================

inline size_t pid() noexcept {
#ifdef _WIN32
    return static_cast<size_t>(::GetCurrentProcessId());
#else
    return static_cast<size_t>(::getpid());
#endif
}

// 返回当前线程的id，比std::this_thread::get_id()更快
inline size_t _thread_id() noexcept {
#ifdef _WIN32
    return static_cast<size_t>(::GetCurrentThreadId());
#elif defined(__linux__)
    return static_cast<size_t>(::syscall(SYS_gettid));
#else   // other unix
    return static_cast<size_t>(std::hash<std::thread::id>()(std::this_thread::get_id()));
#endif
}

inline size_t thread_id() noexcept {
#ifdef LEARNLOG_USE_TLS
    static thread_local const size_t tid = os::_thread_id();
    return tid;
#else
    return os::_thread_id();
#endif 
}

inline int get_errno() noexcept {
#ifdef _WIN32
    return static_cast<int>(::GetLastError());
#else
    return errno;
#endif
}

// ====================================id===========================================

// ====================================file=========================================

// 判断路径（目录或文件）是否存在
inline bool dir_exist(const filename_t& path) noexcept {
#ifdef _WIN32
    struct _stat buf;
    return (::_wstat(path.c_str(), &buf) == 0);
#else
    struct stat buf;
    return (::stat(path.c_str(), &buf) == 0);
#endif
}

static bool mkdir_(const filename_t& path) {
#ifdef _WIN32
    return ::_wmkdir(path.c_str()) == 0;
#else
    return ::mkdir(path.c_str(), mode_t(0755)) == 0;
#endif
}

// 根据给定路径创建目录，包括父目录，权限码为 0755
// 目录已存在或者目录创建成功返回 true；目录为空或者目录创建失败返回 false
inline bool create_dir(const filename_t& path) {
    if (dir_exist(path)) return true;
    if (path.size() == 0) return false;

    // 逐级检查父目录，不存在时创建
    size_t offset = 0;
    while (offset < path.size()) {
        size_t spr_pos = path.find_first_of(FOLDER_SPR, offset);
        spr_pos = (spr_pos == filename_t::npos) ? path.size() : spr_pos;
        filename_t curr_dir = path.substr(0, spr_pos);

#ifdef _WIN32
        // windows 目录可能出现如 "C:" 的情况，需要改为 "C:\"，dir_exist() 才能做出正确判断
        if (spr_pos == 2 && curr_dir[1] == ':') {
            curr_dir += '\\';
            spr_pos++;
        }
#endif

        if (!dir_exist(curr_dir) && !mkdir_(curr_dir)) {
            return false;
        }
        offset = spr_pos + 1;
    }

    return true;
}

// 从路径中提取目录，即最后一个分隔符之前的部分
inline filename_t get_dir(const filename_t& path) {
    size_t spr_pos = path.find_last_of(FOLDER_SPR);
    return (spr_pos == filename_t::npos) ? filename_t{} : path.substr(0, spr_pos);
}

// 打开文件，文件流指针保存到 *fp，
// windows 中 filename_t 是 std::wstring，其它系统中是 std::string，
// mode 为 "ab" 表示追加，"wb" 表示覆盖，
// 禁止子进程直接继承父进程的 fd，
// 成功获取到 *fp 返回 true，否则返回 false
inline bool open_file(FILE** fp, const filename_t& filename, const filename_t& mode) {
#ifdef _WIN32
    *fp = ::_wfsopen((filename.c_str()), mode.c_str(), _SH_DENYNO);
    if (*fp != nullptr) {
        auto file_handle = reinterpret_cast<HANDLE>(_get_osfhandle(::_fileno(*fp)));
        if (!::SetHandleInformation(file_handle, HANDLE_FLAG_INHERIT, 0)) {
            ::fclose(*fp);
            *fp = nullptr;
        }
    }
#else
    int op_flag = (mode == "ab") ? O_APPEND : O_TRUNC;
    const int fd = ::open(filename.c_str(), 
                          O_CREAT | O_WRONLY | O_CLOEXEC | op_flag, mode_t(0644));
    if (fd == -1) return false;
    *fp = ::fdopen(fd, mode.c_str());
    if (*fp == nullptr) ::close(fd);
#endif

    return *fp != nullptr;
}

inline int remove_file(const filename_t& filename) noexcept {
#ifdef _WIN32
    return ::_wremove(filename.c_str());
#else
    return std::remove(filename.c_str());
#endif
}

inline int remove_file_if_exist(const filename_t& filename) noexcept {
    return dir_exist(filename) ? remove_file(filename) : 0;
}

inline int rename_file(const filename_t& filename1, const filename_t& filename2) noexcept {
#ifdef _WIN32
    return ::_wrename(filename1.c_str(), filename2.c_str());
#else
    return std::rename(filename1.c_str(), filename2.c_str());
#endif
}

// 获取文件大小（字节），获取失败返回 -1
inline long_long filesize(FILE* fp) {
#if defined(_WIN32) && !defined(__CYGWIN__)
    int fd = ::_fileno(fp);
    #if defined(_WIN64)  // 64 位
        long long ret = ::_filelengthi64(fd);
        if (ret >= 0) return ret;
    #else               // 32 位
        int ret = ::_filelength(fd);
        if (ret >= 0) return ret;
    #endif
#else
    int fd = ::fileno(fp);
    #if defined(__linux__)  // 64 位
        struct stat64 buf64;
        if (::fstat64(fd, &buf64) == 0) {
            return static_cast<long_long>(buf64.st_size);
        }
    #else                   // 32 位
        struct stat buf;
        if (::fstat(fd, &buf) == 0) {
            return static_cast<long_long>(buf.st_size);
        }
    #endif
#endif

    return -1;
}

// fsync 成功返回 true，失败返回 false
inline bool fsync(FILE* fp) {
#ifdef _WIN32
    return FlushFileBuffers(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fp)))) != 0;
#else
    return ::fsync(fileno(fp)) == 0;
#endif
}

// ====================================file=========================================

}   // namespace os
}   // namespace base
}   // namespace learnlog