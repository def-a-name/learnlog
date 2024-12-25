#pragma once

#include "definitions.h"
#include "base/exception.h"

namespace learnlog {
namespace base {

inline int utf8_to_wcharbuf(const char* ustr, int ustr_size, fmt_wmemory_buf& wbuf) {
    if (ustr_size <= 0) return 0;

    int size_to_write = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, 
                                                ustr, ustr_size, NULL, 0);
    if (size_to_write > 0) {
        wchar_t* wstr;
        int size_written = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, 
                                ustr, ustr_size, wstr, size_to_write);
        wbuf.append(wstr, wstr + size_written);
    }
    return size_to_write;
}

inline void utf8buf_to_wcharbuf(const fmt_memory_buf& ubuf, size_t& color_start, size_t& color_end, 
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
    if (color_start < color_end) {     // 同时更新上色区间
        wchars_to_write += utf8_to_wcharbuf(ubuf.data(), static_cast<int>(color_start), wbuf);
        size_t new_color_start = wbuf.size();
        
        wchars_to_write += utf8_to_wcharbuf(ubuf.data() + color_start, 
                                            static_cast<int>(color_end - color_start), wbuf);
        size_t new_color_end = wbuf.size(); 
        
        wchars_to_write += utf8_to_wcharbuf(ubuf.data() + color_end, 
                                            static_cast<int>(ubuf.size() - color_end), wbuf);
        color_start = new_color_start;
        color_end = new_color_end;
    }
    else {
        wchars_to_write = utf8_to_wcharbuf(ubuf.data(), ubuf.size(), wbuf);
    }

    if (wchars_to_write != wbuf.size()) {
        source_loc loc{__FILE__, __LINE__, __func__};
        throw_learnlog_excpt("learnlog::wchar_support: MultiByteToWideChar() failed", ::GetLastError(), loc);
    }
}

}   // namespace base
}   // namespace learnlog

