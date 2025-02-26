#pragma once

#include "definitions.h"
#include "base/fmt_base.h"

#include <string>
#include <type_traits>

namespace learnlog {
namespace sinks {

// 空格填充信息
struct spaces_info {
    enum class fill_side { left, right, center };

    spaces_info() = default;
    spaces_info(size_t len, fill_side s, bool trunc, bool enabled = true)
        : target_len_(len), side_(s), truncate_(trunc), enabled_(enabled) {}

    spaces_info(const spaces_info&) = default;
    spaces_info& operator=(const spaces_info&) = default;
    
    bool enabled() const { return enabled_; }
    size_t target_len_ = 0;             // 填充至目标长度
    fill_side side_ = fill_side::left;  // 填充方向
    bool truncate_ = false;             // 实际长度大于目标长度时，是否截断
    bool enabled_ = false;              // 是否有效
};

// 数据填充器，将指定长度的数据填入 fmt_memory_buf，并根据 spaces_info 的要求填充空格，保证格式对齐；
// 在构造时填充左边的空格，析构时填充右边的空格，如果目标长度小于实际长度，可选择截断；
// 填入的数据可以是能隐式转换为 fmt_string_view 的字符串，也能是任意无符号 / 有符号的整数类型；
// 填入无符号整数类型时，可指定填充前缀0
class filler {
public:
    filler(size_t msg_len, const spaces_info& spaces_info, fmt_memory_buf& dest_buf)
        : msg_len_(msg_len), spaces_info_(spaces_info), dest_buf_(dest_buf) {    
        
        if(!spaces_info_.enabled()) return;     // 如果 spaces_info 信息无效则不填充空格
        
        spaces_.resize(MAX_SPACES_LEN, ' ');        // 最大空格填充数为 MAX_SPACES_LEN
        spaces_to_fill_ = static_cast<int>(spaces_info_.target_len_ - msg_len);
        if(spaces_to_fill_ <= 0) return;

        if(spaces_info_.side_ == spaces_info::fill_side::left) {
            fill_spaces_(spaces_to_fill_);
            spaces_to_fill_ = 0;
        }
        else if(spaces_info_.side_ == spaces_info::fill_side::center) {
            size_t half_to_fill = spaces_to_fill_ / 2;
            fill_spaces_(half_to_fill);
            spaces_to_fill_ = static_cast<int>(spaces_to_fill_ - half_to_fill);
        }
    }

    ~filler() {
        if(!spaces_info_.enabled()) return;     // 如果 spaces_info 信息无效则不填充空格

        if(spaces_to_fill_ > 0) {
            fill_spaces_(spaces_to_fill_);
        }
        else if(spaces_to_fill_ < 0 && spaces_info_.truncate_) {
            size_t new_size = dest_buf_.size() + spaces_to_fill_;
            dest_buf_.resize(new_size);
        }
    }

    // msg 的类型可隐式转换为 fmt_string_view
    void fill_msg(fmt_string_view strview) {
        base::fmt_base::append_string_view(strview, dest_buf_);
    }

    // msg 的类型是无符号整数类型之一，可填充前缀0
    template <typename T,
              typename std::enable_if<is_unsigned_integral<const T>::value,
                                      int>::type = 0>
    void fill_msg(T n) {
        base::fmt_base::fill_uint(n, msg_len_, dest_buf_);
    }

    // msg 的类型是带符号整数类型之一，直接填充
    template <typename T,
              typename std::enable_if<is_signed_integral<const T>::value,
                                      int>::type = 1>
    void fill_msg(T n) {
        base::fmt_base::append_int(n, dest_buf_);
    }

private:
    // 填充空格
    void fill_spaces_(size_t len) {
        base::fmt_base::append_string_view(fmt_string_view(spaces_.c_str(), len), 
                                            dest_buf_);
    }

    size_t msg_len_;                    // 填入的 msg 长度
    const spaces_info& spaces_info_;    // 空格填充信息
    fmt_memory_buf& dest_buf_;          // 目标缓冲区
    
    int spaces_to_fill_;                // 剩余填充的空格数
    std::string spaces_;                // 长度为 64 的空格字符串
};

}    // namespace sinks
}    // namespace learnlog