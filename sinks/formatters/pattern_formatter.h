#pragma once

#include "sinks/formatters/formatter.h"
#include "sinks/formatters/flag_formatter.h"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace mylog {
namespace sinks {

// formatter 的派生类，通过 format() 每次格式化一条 log_msg，
// 由格式模板字符串生成一个格式化器队列，依次格式化 log_msg 的不同部分，将结果写入缓冲区
class pattern_formatter : public formatter {
public:
    using cust_flag_table = std::unordered_map<char, std::unique_ptr<custom_flag_formatter> >;
    explicit pattern_formatter(std::string pattern,
                               std::string eol = DEFAULT_EOL,
                               cust_flag_table custom_flags_in = cust_flag_table{});

    // 构造时不提供格式模板字符串，使用默认模板 "%+"，对应格式化器 full_formatter
    explicit pattern_formatter();

    pattern_formatter(const pattern_formatter& other) = delete;
    pattern_formatter &operator=(const pattern_formatter& other) = delete;

    void format(const base::log_msg& msg, fmt_memory_buf& dest_buf) override;
    formatter_uni_ptr clone() const override;

    void set_pattern(std::string pattern);

    // 添加自定义格式字符与格式化器，T 为 custom_flag_formatter 的派生类
    template <typename T, typename... Args>
    void add_custom_flag(char flag, Args &&...args) {
        custom_flags_[flag] = mylog::make_unique<T>(std::forward<Args>(args)...); 
    }

private: 
    void analyse_pattern_(const std::string& pattern);                  // 解析格式模板字符串
    spaces_info analyse_spaces_info_(std::string::const_iterator& it,
                                     std::string::const_iterator end);  // 解析空格填充信息
    void append_flag_formatter_(char flag, const spaces_info& sp_info); // 添加格式化器

    std::string pattern_;                                       // 格式模板字符串
    std::string eol_;                                           // end of line，换行符
    std::vector<std::unique_ptr<flag_formatter> > formatters_;  // 格式化器队列
    cust_flag_table custom_flags_;                              // 自定义格式化器的映射表
};

}   // namespace sinks
}   // namespace mylog