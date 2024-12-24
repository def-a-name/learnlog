#pragma once

#include "definitions.h"
#include "base/exception.h"

namespace mylog {
namespace base {
namespace file_base {

// 关闭文件
void close(FILE** fpp, const filename_t& fname) {
    if (*fpp != nullptr) {
        if (::fclose(*fpp) != 0) {
            source_loc loc{__FILE__, __LINE__, __func__};
            std::string fname_str(fname.begin(), fname.end());
            std::string err_str = fmt::format("mylog::file_base::close() failed, filename: '{}'", fname_str);
            throw_mylog_excpt(err_str, os::get_errno(), loc);
        }
        
        *fpp = nullptr;
    }
}

// 打开文件，
// 首先关闭已有文件流，然后尝试打开文件，文件流指针保存到 *fpp，
// 共尝试 try_times 次，每次间隔 try_every_ms 毫秒，
// 打开文件时先创建父级目录，再创建文件流，
// truncate 表示是否以覆写模式打开文件，默认为 false，
// 如果 truncate 为 true，则先以 wb 模式创建文件，再重新以 ab 模式打开，避免已写入内容意外被清空
void open(FILE** fpp, const filename_t& fname, bool truncate = false, 
          int try_times = 5, unsigned int try_every_ms = 10) {
    file_base::close(fpp, fname);

#ifdef _WIN32
    filename_t wb(L"wb");
    filename_t ab(L"ab");
#else
    filename_t wb("wb");
    filename_t ab("ab");
#endif

    for (int i=0; i < try_times; ++i) {
        os::create_dir(os::get_dir(fname));
        
        if (truncate) {
            FILE* fd_tmp;
            if (!os::open_file(&fd_tmp, fname, wb)) continue;
            ::fclose(fd_tmp);
        }
        
        if (os::open_file(fpp, fname, ab)) return;

        os::sleep_for_ms(try_every_ms);
    }

    source_loc loc{__FILE__, __LINE__, __func__};
    std::string fname_str(fname.begin(), fname.end());
    std::string err_str = fmt::format("mylog::file_base::open() failed, filename: '{}'", fname_str);
    throw_mylog_excpt(err_str, os::get_errno(), loc);
}

// 刷新文件流缓冲区
inline void flush(FILE* fp, const filename_t& fname) {
    if (::fflush(fp) != 0) {
        source_loc loc{__FILE__, __LINE__, __func__};
        std::string fname_str(fname.begin(), fname.end());
        std::string err_str = fmt::format("mylog::file_base::flush() failed, filename: '{}'", fname_str);
        throw_mylog_excpt(err_str, os::get_errno(), loc);
    }
}

// 刷新文件系统缓存，保证文件数据被写入磁盘
inline void sync(FILE* fp, const filename_t& fname) {
    if (!os::fsync(fp)) {
        source_loc loc{__FILE__, __LINE__, __func__};
        std::string fname_str(fname.begin(), fname.end());
        std::string err_str = fmt::format("mylog::file_base::sync() failed, filename: '{}'", fname_str);
        throw_mylog_excpt(err_str, os::get_errno(), loc);
    }
}

// 将 buf 的内容写入文件
inline void write(FILE* fp, const filename_t& fname, const fmt_memory_buf& buf) {
    if (fp == nullptr) return;

    if (::fwrite(buf.data(), sizeof(char), buf.size(), fp) != buf.size()) {
        source_loc loc{__FILE__, __LINE__, __func__};
        std::string fname_str(fname.begin(), fname.end());
        std::string err_str = fmt::format("mylog::file_base::write() failed, filename: '{}'", fname_str);
        throw_mylog_excpt(err_str, os::get_errno(), loc);
    }
}

// 获取文件大小（字节），u_long_long 为 64 位无符号整型
inline u_long_long size(FILE* fp, const filename_t& fname) {
    if (fp == nullptr) {
        std::string fname_str(fname.begin(), fname.end());
        std::string err_str = fmt::format(
            "mylog::file_base::size() failed, fp == nullptr, filename: '{}'", fname_str);
        throw_mylog_excpt(err_str);
    }
    
    long_long ret = os::filesize(fp);
    
    if (ret == -1) {
        source_loc loc{__FILE__, __LINE__, __func__};
        std::string fname_str(fname.begin(), fname.end());
        std::string err_str = fmt::format("mylog::file_base::size() failed, filename: '{}'", fname_str);
        throw_mylog_excpt(err_str, os::get_errno(), loc);
    }
    
    return static_cast<u_long_long>(ret);
}

// 从文件路径中找到后缀名（如.txt）的起始位置，路径不包含后缀名时返回超尾位置
size_t suffix_pos(const filename_t& fname) {
    size_t file_idx = fname.find_last_of(FOLDER_SPR);
    size_t suf_idx = fname.find_last_of('.');
    if (suf_idx <= file_idx + 1 || suf_idx == filename_t::npos) {
        return fname.size();
    }
    return suf_idx;
}

}   // namespace file_base
}   // namespace base
}   // namespace mylog