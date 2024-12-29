#pragma once

#include "definitions.h"
#include "base/exception.h"

namespace learnlog {
namespace base {

// 由 std::string 生成对应的 std::wstring
inline std::wstring string_to_wstring(const std::string& str) {
    const char* ustr = str.c_str();
    size_t ustr_size = str.size();
    if (ustr_size <= 0) return 0;

    int size_to_write = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, 
                                              ustr, ustr_size, NULL, 0);
    std::wstring ret;
    if (size_to_write > 0) {
        wchar_t* wstr = new wchar_t[size_to_write];
        int size_written = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, 
                                                 ustr, ustr_size, wstr, size_to_write);
        ret.assign(wstr, wstr + size_written);
        delete[] wstr;
    }
    
    if (size_to_write != ret.size()) {
        source_loc loc{__FILE__, __LINE__, __func__};
        throw_learnlog_excpt("learnlog::wchar_support: string_to_wstring() failed", 
                             ::GetLastError(), loc);
    }

    return ret;
}

// 由 std::wstring 生成对应的 std::string
inline std::string wstring_to_string(const std::wstring& str) {
    const wchar_t* wstr = str.c_str();
    size_t wstr_size = str.size();
    if (wstr_size <= 0) return 0;

    int size_to_write = ::WideCharToMultiByte(CP_UTF8, 0, 
                                              wstr, wstr_size, NULL, 0, NULL, NULL);

    std::string ret;
    if (size_to_write > 0) {
        char* ustr = new char[size_to_write];
        int size_written = ::WideCharToMultiByte(CP_UTF8, 0,
                                                 wstr, wstr_size, ustr, size_to_write, 
                                                 NULL, NULL);
        ret.assign(ustr, ustr + size_written);
        delete[] ustr;
    }
    
    if (size_to_write != ret.size()) {
        source_loc loc{__FILE__, __LINE__, __func__};
        throw_learnlog_excpt("learnlog::wchar_support: wstring_to_string() failed", 
                             ::GetLastError(), loc);
    }

    return ret;
}

// 由 char* 生成 wchar_t*，并写入 fmt_wmemory_buf，返回值是应该被写入的宽字符数
inline int utf8_to_wcharbuf_(const char* ustr, int ustr_size, fmt_wmemory_buf& wbuf) {
    if (ustr_size <= 0) return 0;

    int size_to_write = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, 
                                              ustr, ustr_size, NULL, 0);
    if (size_to_write > 0) {
        wchar_t* wstr = new wchar_t[size_to_write];
        int size_written = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, 
                                                 ustr, ustr_size, wstr, size_to_write);
        wbuf.append(wstr, wstr + size_written);
        delete[] wstr;
    }
    return size_to_write;
}

// 由 wchar_t* 生成 char*，并写入 fmt_memory_buf，返回值是应该被写入的字符数
inline int wchar_to_utf8buf_(const wchar_t* wstr, int wstr_size, fmt_memory_buf& ubuf) {
    if (wstr_size <= 0) return 0;

    int size_to_write = ::WideCharToMultiByte(CP_UTF8, 0, 
                                              wstr, wstr_size, NULL, 0, NULL, NULL);

    if (size_to_write > 0) {
        char* ustr = new char[size_to_write];
        int size_written = ::WideCharToMultiByte(CP_UTF8, 0,
                                                 wstr, wstr_size, ustr, size_to_write, 
                                                 NULL, NULL);
        ubuf.append(ustr, ustr + size_written);
        delete[] ustr;
    }
    return size_to_write;
}

// 将 fmt_memory_buf 中的 UTF-8 字符串存储为 fmt_wmemory_buf 中的 WCHAR 字符串，
// 同时更新上色字符区间
inline void utf8buf_to_wcharbuf(const fmt_memory_buf& ubuf, 
                                size_t& color_start, size_t& color_end, 
                                fmt_wmemory_buf& wbuf) {
    if (ubuf.size() > static_cast<size_t>(INT_MAX - 1)) {
        throw_learnlog_excpt(
            "learnlog::wchar_support: UTF-8 string is too long to be converted to WCHAR");
    }
    if (ubuf.size() == 0) {
        wbuf.resize(0);
        return;
    }

    int wchars_to_write = 0;
    if (color_start < color_end) {
        wchars_to_write += utf8_to_wcharbuf_(ubuf.data(), static_cast<int>(color_start), wbuf);
        size_t new_color_start = wbuf.size();
        
        wchars_to_write += utf8_to_wcharbuf_(ubuf.data() + color_start, 
                                            static_cast<int>(color_end - color_start), wbuf);
        size_t new_color_end = wbuf.size(); 
        
        wchars_to_write += utf8_to_wcharbuf_(ubuf.data() + color_end, 
                                            static_cast<int>(ubuf.size() - color_end), wbuf);
        color_start = new_color_start;
        color_end = new_color_end;
    }
    else {
        wchars_to_write = utf8_to_wcharbuf_(ubuf.data(), ubuf.size(), wbuf);
    }

    if (wchars_to_write != wbuf.size()) {
        source_loc loc{__FILE__, __LINE__, __func__};
        throw_learnlog_excpt("learnlog::wchar_support: utf8buf_to_wcharbuf() failed", 
                             ::GetLastError(), loc);
    }
}

// 将 fmt_memory_buf 中的 UTF-8 字符串存储为 fmt_wmemory_buf 中的 WCHAR 字符串
inline void utf8buf_to_wcharbuf(const fmt_memory_buf& ubuf, 
                                fmt_wmemory_buf& wbuf) {
    if (ubuf.size() > static_cast<size_t>(INT_MAX - 1)) {
        throw_learnlog_excpt(
            "learnlog::wchar_support: UTF-8 string is too long to be converted to WCHAR");
    }
    if (ubuf.size() == 0) {
        wbuf.resize(0);
        return;
    }

    int wchars_to_write = utf8_to_wcharbuf_(ubuf.data(), ubuf.size(), wbuf);

    if (wchars_to_write != wbuf.size()) {
        source_loc loc{__FILE__, __LINE__, __func__};
        throw_learnlog_excpt("learnlog::wchar_support: utf8buf_to_wcharbuf() failed", 
                             ::GetLastError(), loc);
    }
}

// 将 fmt_wmemory_buf 中的 WCHAR 字符串存储为 fmt_memory_buf 中的 UTF-8 字符串
inline void wcharbuf_to_utf8buf(const fmt_wmemory_buf& wbuf, 
                                fmt_memory_buf& ubuf) {
    if (wbuf.size() > static_cast<size_t>(INT_MAX / 4 - 1) ) {
        throw_learnlog_excpt(
            "learnlog::wchar_support: WCHAR string is too long to be converted to UTF-8");
    }
    if (wbuf.size() == 0) {
        ubuf.resize(0);
        return;
    }

    int chars_to_write = wchar_to_utf8buf_(wbuf.data(), wbuf.size(), ubuf);

    if (chars_to_write != ubuf.size()) {
        source_loc loc{__FILE__, __LINE__, __func__};
        throw_learnlog_excpt("learnlog::wchar_support: wcharbuf_to_utf8buf() failed", 
                             ::GetLastError(), loc);
    }
}

}   // namespace base
}   // namespace learnlog