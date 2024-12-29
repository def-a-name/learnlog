#pragma once

#include "sinks/basic_sink.h"
#include "base/exception.h"
#include "base/file_base.h"
#include "sync_factory.h"
#include "base/null_mutex.h"

namespace learnlog {
namespace sinks {

// basic_sink 的派生类，
// 是模板参数为 Mutex 的模板类，在需要保证线程安全时传入 std::mutex，不需要时传入 base::null_mutex，
// rolling_file_sink 会把格式化后的 log_msg 依次写入自动编号的多个文件中，
// 单个文件的大小不超过 max_file_size，文件的总个数不超过 max_file_num

template <typename Mutex>
class rolling_file_sink final : public basic_sink<Mutex> {
public:
    explicit rolling_file_sink(const filename_t& base_fname,
                          size_t max_file_size,
                          size_t max_file_num)
        : base_fname_(base_fname),
          max_file_size_(max_file_size), 
          max_file_num_(max_file_num) 
    {
        if (max_file_size == 0) {
            throw_learnlog_excpt("learnlog::rolling_file_sink: max_file_size equals 0");
        }
        if (max_file_num_ > 10000) {
            throw_learnlog_excpt("learnlog::rolling_file_sink: max_file_num_ exceeds 10000");
        }

        // 找到当前文件路径下不引起冲突的最小文件编号，且保证编号范围是 [1, max_file_num_]
        for (file_index_ = 1; file_index_ < max_file_num_; ++file_index_) {
            if ( !base::os::dir_exist(get_rolling_filename(file_index_)) )
                break;
        }
        base::file_base::open(&file_, get_rolling_filename(file_index_), true);
        cur_file_size_ = 0;
    }

    ~rolling_file_sink() { 
        basic_sink<Mutex>::flush();
        base::file_base::close(&file_, get_rolling_filename(file_index_));
    }
   
    filename_t base_filename() const { return base_fname_; }

    // 由 base_fname_ 和 roll_index 组合为滚动文件名，
    // 文件编号的长度由 max_file_num_ 的位数决定，填充前缀0，
    // 如 "test_log/log.txt" -> "test_log/log_01.txt"
    filename_t get_rolling_filename(size_t roll_index) {
#ifdef _WIN32
        size_t suffix_pos = base::file_base::suffix_pos(base_fname_);
        std::string basename = base::wstring_to_string(base_fname_.substr(0, suffix_pos));
        std::string suffix = base::wstring_to_string(base_fname_.substr(suffix_pos));

        fmt_memory_buf buf;
        base::fmt_base::append_string_view(basename, buf);
        base::fmt_base::append_string_view("_", buf);
        size_t index_len = base::fmt_base::count_unsigned_digits(max_file_num_);
        base::fmt_base::fill_uint(roll_index, index_len, buf);
        base::fmt_base::append_string_view(suffix, buf);

        fmt_wmemory_buf wbuf;
        base::utf8buf_to_wcharbuf(buf, wbuf);
        return filename_t{wbuf.data(), wbuf.size()};
#else
        size_t suffix_pos = base::file_base::suffix_pos(base_fname_);
        std::string basename = base_fname_.substr(0, suffix_pos);
        std::string suffix = base_fname_.substr(suffix_pos);

        fmt_memory_buf buf;
        base::fmt_base::append_string_view(basename, buf);
        base::fmt_base::append_string_view("_", buf);
        size_t index_len = base::fmt_base::count_unsigned_digits(max_file_num_);
        base::fmt_base::fill_uint(roll_index, index_len, buf);
        base::fmt_base::append_string_view(suffix, buf);

        return filename_t{buf.data(), buf.size()};
#endif
    }

private:
    // 判断在写入 log_msg 后当前文件的大小是否会超过 max_file_size_，如果会则
    // 滚动文件（即保存关闭当前文件，打开新文件），
    // 当前文件的大小由 cur_file_size_ 维护，file_base::size() 的开销较大，只用于
    // 避免因 log_msg 过大而出现空文件
    void output_(const base::log_msg &msg) override {
        fmt_memory_buf buf;
        basic_sink<Mutex>::formatter_->format(msg, buf);
        if (cur_file_size_ + buf.size() > max_file_size_) {
            flush_();
            if (base::file_base::size(file_, get_rolling_filename(file_index_)) > 0) {
                roll_file_();
            }
            cur_file_size_ = 0;
        }
        base::file_base::write(file_, get_rolling_filename(file_index_), buf);
        cur_file_size_ += buf.size();
    }

    void flush_() override {
        base::file_base::flush(file_, get_rolling_filename(file_index_));
    }

    // 保存关闭当前文件，然后找到新的滚动文件名，打开新文件，
    // 如果文件编号已经是 max_file_num_，用 truncate 模式重新打开当前文件
    void roll_file_() {
        base::file_base::close(&file_, get_rolling_filename(file_index_));

        // 找到当前文件路径下不引起冲突的最小文件编号，且保证编号范围是 [1, max_file_num_]
        for (; file_index_ < max_file_num_; ++file_index_) {
            if ( !base::os::dir_exist(get_rolling_filename(file_index_)) )
                break;
        }

        base::file_base::open(&file_, get_rolling_filename(file_index_), true);
    }

    FILE* file_{nullptr};
    filename_t base_fname_;
    
    size_t file_index_;
    size_t cur_file_size_;

    size_t max_file_size_;
    size_t max_file_num_;
};

using rolling_file_sink_mt = rolling_file_sink<std::mutex>;
using rolling_file_sink_st = rolling_file_sink<base::null_mutex>;

}   // namespace sinks

// factory 函数，创建使用 rolling_file_sink 的 logger 对象

template <typename Factory = learnlog::sync_factory>
logger_shr_ptr rolling_file_logger_mt(const std::string& logger_name,
                                      const filename_t& base_filename,
                                      size_t max_file_size,
                                      size_t max_file_num) {
    return Factory::template create<sinks::rolling_file_sink_mt>(logger_name, 
                                                                 base_filename, 
                                                                 max_file_size, 
                                                                 max_file_num);
}

template <typename Factory = learnlog::sync_factory>
logger_shr_ptr rolling_file_logger_st(const std::string& logger_name,
                                      const filename_t& base_filename,
                                      size_t max_file_size,
                                      size_t max_file_num) {
    return Factory::template create<sinks::rolling_file_sink_st>(logger_name, 
                                                                 base_filename, 
                                                                 max_file_size, 
                                                                 max_file_num);
}

}   // namespace learnlog