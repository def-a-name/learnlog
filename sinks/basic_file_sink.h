#pragma once

#include "sinks/basic_sink.h"
#include "base/file_base.h"
#include "base/null_mutex.h"
#include "sync_factory.h"

namespace mylog {
namespace sinks {

// basic_sink 的派生类，
// 是模板参数为 Mutex 的模板类，在需要保证线程安全时传入 std::mutex，不需要时传入 base::null_mutex，
// basic_file_sink 在构造时打开路径 filename 指向的文件（不存在时创建），在析构时关闭文件，
// 构造时的参数 truncate 指定是否清空文件已有内容，
// basic_file_sink 覆写了父类的 output_()、flush_() 函数，将格式化后的 log_msg 写入单个文件

template <typename Mutex>
class basic_file_sink final : public basic_sink<Mutex> {
public:
    explicit basic_file_sink(const filename_t& filename,
                             bool truncate = false)
        : filename_(filename) {
        base::file_base::open(&file_, filename_, truncate);
    }
    
    ~basic_file_sink() override {
        basic_sink<Mutex>::flush();
        base::file_base::close(&file_, filename_);
    }

    filename_t filename() const { return filename_; }

protected:
    void output_(const base::log_msg &msg) override {
        fmt_memory_buf buf;
        basic_sink<Mutex>::formatter_->format(msg, buf);
        base::file_base::write(file_, filename_, buf);
    }
    
    void flush_() override {
        base::file_base::flush(file_, filename_);
    }

private:
    FILE* file_{nullptr};
    filename_t filename_;
};

using basic_file_sink_mt = basic_file_sink<std::mutex>;
using basic_file_sink_st = basic_file_sink<base::null_mutex>;

}    // namespace sinks

// factory 函数，创建使用 basic_file_sink 的 logger 对象

template <typename Factory = mylog::sync_factory>
logger_shr_ptr basic_file_logger_mt(const std::string& logger_name,
                                    const filename_t& filename,
                                    bool truncate = false) {
    return Factory::template create<sinks::basic_file_sink_mt>(logger_name, filename, truncate);
}

template <typename Factory = mylog::sync_factory>
logger_shr_ptr basic_file_logger_st(const std::string& logger_name,
                                    const filename_t& filename,
                                    bool truncate = false) {
    return Factory::template create<sinks::basic_file_sink_st>(logger_name, filename, truncate);
}

}    // namespace mylog