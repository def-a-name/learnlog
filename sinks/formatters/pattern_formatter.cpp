#include "sinks/formatters/pattern_formatter.h"
#include "base/os.h"

#include <cstring>
#include <cctype>

using namespace mylog;
using namespace sinks;

pattern_formatter::pattern_formatter(std::string pattern,
                                     std::string eol,
                                     cust_flag_table custom_flags_in) 
    : pattern_(std::move(pattern)),
      eol_(std::move(eol)),
      custom_flags_(std::move(custom_flags_in)) {
    
    analyse_pattern_(pattern_);
}

pattern_formatter::pattern_formatter()
    : pattern_("%+"),
      eol_(DEFAULT_EOL) {

    formatters_.push_back(mylog::make_unique<full_formatter>(spaces_info{}));
}

void pattern_formatter::format(const base::log_msg& msg, fmt_memory_buf& dest_buf) {
    std::tm msg_tm = base::os::time_point_to_tm(msg.time);
    for (auto& f : formatters_) {
        f->format(msg, msg_tm, dest_buf);
    }
    base::fmt_base::append_string_view(eol_, dest_buf);     // 末尾换行符
}

formatter_uni_ptr pattern_formatter::clone() const {
    cust_flag_table custom_flags_clone;
    for (const auto& p : custom_flags_) {
        custom_flags_clone[p.first] = p.second->clone();
    }
    return mylog::make_unique<pattern_formatter>(pattern_, eol_, std::move(custom_flags_clone));
}

void pattern_formatter::set_pattern(std::string pattern) {
    pattern_ = std::move(pattern);
    analyse_pattern_(pattern_);
}

void pattern_formatter::analyse_pattern_(const std::string& pattern) {
    using str_const_iter = std::string::const_iterator;
    
    str_const_iter end = pattern.end();
    std::unique_ptr<aggregate_formatter> as_is_formatter = nullptr;  // 该格式化器用于处理连续的非格式字符
    formatters_.clear();
    for (str_const_iter it = pattern.begin(); it != end; ++it) {
        if (*it == '%') {
            if (as_is_formatter) {  // 遇到 '%', 代表一段连续非格式字符的结束，提交格式化器到处理队列
                formatters_.push_back(std::move(as_is_formatter));
            }

            spaces_info sp_info = analyse_spaces_info_(++it, end);

            if (it != end) {
                append_flag_formatter_(*it, sp_info);
            }
        }
        else {
            if (!as_is_formatter) {
                as_is_formatter = mylog::make_unique<aggregate_formatter>();
            }
            as_is_formatter->add_ch(*it);
        }
    }
    if (as_is_formatter) {  // 最后还有一段连续非格式字符，提交格式化器
        formatters_.push_back(std::move(as_is_formatter));
    }
}

// 空格填充信息在 '%' 与格式字符之间，形如 "%10T" "%-10T" "%=10T" "%10!v" "%-10!v" "%=10!v"
spaces_info pattern_formatter::analyse_spaces_info_(std::string::const_iterator& it,
                                                    std::string::const_iterator end) {
    if(it == end) return spaces_info{};   // spaces_info.enabled() = false

    spaces_info::fill_side side;
    switch (*it) {
        case '-':     // 左对齐，空格填右边
            side = spaces_info::fill_side::right;
            ++it;
            break;
        case '=':     // 居中对齐
            side = spaces_info::fill_side::center;
            ++it;
            break;
        default:      // 默认右对齐，空格填左边
            side = spaces_info::fill_side::left;
            break;
    }

    // 无空格填充长度，视为无填充信息
    if (it == end || !std::isdigit(static_cast<unsigned char>(*it))) {
        return spaces_info{};   // spaces_info.enabled() = false
    }

    size_t fill_len = static_cast<size_t>(*it) - '0';
    ++it;
    while (it != end && std::isdigit(static_cast<unsigned char>(*it))) {
        fill_len = fill_len * 10 + static_cast<size_t>(*it) - '0';
        ++it;
    }

    // 是否截断
    if (it == end || *it != '!') {
        return spaces_info{std::min<size_t>(fill_len, MAX_SPACES_LEN), side, false};
    }
    else {
        it++;
        return spaces_info{std::min<size_t>(fill_len, MAX_SPACES_LEN), side, true};
    }
}

void pattern_formatter::append_flag_formatter_(char flag, const spaces_info& sp_info) {
    // 用户指定的格式字符有最高优先级
    auto it = custom_flags_.find(flag);
    if (it != custom_flags_.end()) {
        auto cust_flag_formatter = it->second->clone();
        cust_flag_formatter->set_spaces_info(sp_info);
        formatters_.push_back(std::move(cust_flag_formatter));
        return;
    }

    // 匹配预设格式字符
    /* 已使用：
        %+ == full formatter，格式化字符串为 "[%y-%m-%d %H:%M:%S.%E] [%n] [%l] [%s:%#] %v";
        %n == log_msg.logger_name;
        %l == log_msg.level;
        %a == 3位字符，表示星期几;
        %b == 3位字符，表示月份;
        %c == datetime formatter，格式化字符串为 "%a %b %d %H:%M:%S %y";
        %y == 年份，4位;
        %m == 月份，2位;
        %d == 日，2位;
        %H == 时，2位;
        %M == 分，2位;
        %S == 秒，2位;
        %E == 毫秒，3位;
        %F == 微秒，6位;
        %G == 纳秒，9位;
        %T == time formatter，格式化字符串为 "%H:%M:%S"；
        %W == 与上条 log_msg 的时间间隔，纳秒;
        %X == 与上条 log_msg 的时间间隔，微秒;
        %Y == 与上条 log_msg 的时间间隔，毫秒;
        %Z == 与上条 log_msg 的时间间隔，秒;
        %p == pid;
        %t == tid;
        %v == log_msg.msg;
        %^ == 上色区间开始;
        %$ == 上色区间结束;
        %@ == 消息位置，"filename:linenum";
        %A == 消息位置，"filename";
        %B == 消息位置，"linenum";
        %C == 消息位置，"funcname";
        %% == 字符 '%';
    */
    switch (flag) {
        case ('+'):  
            formatters_.push_back(mylog::make_unique<full_formatter>(sp_info));
            break;
        
        case ('n'):  
            formatters_.push_back(mylog::make_unique<name_formatter>(sp_info));
            break;

        case ('l'):  
            formatters_.push_back(mylog::make_unique<level_formatter>(sp_info));
            break;

        case ('a'):  
            formatters_.push_back(mylog::make_unique<a_formatter>(sp_info));
            break;

        case ('b'):  
            formatters_.push_back(mylog::make_unique<b_formatter>(sp_info));
            break;

        case ('c'):  
            formatters_.push_back(mylog::make_unique<c_formatter>(sp_info));
            break;

        case ('y'):  
            formatters_.push_back(mylog::make_unique<y_formatter>(sp_info));
            break;

        case ('m'):  
            formatters_.push_back(mylog::make_unique<m_formatter>(sp_info));
            break;

        case ('d'):  
            formatters_.push_back(mylog::make_unique<d_formatter>(sp_info));
            break;

        case ('H'):  
            formatters_.push_back(mylog::make_unique<H_formatter>(sp_info));
            break;

        case ('M'):  
            formatters_.push_back(mylog::make_unique<M_formatter>(sp_info));
            break;

        case ('S'):  
            formatters_.push_back(mylog::make_unique<S_formatter>(sp_info));
            break;

        case ('E'):  
            formatters_.push_back(mylog::make_unique<E_formatter>(sp_info));
            break;

        case ('F'):  
            formatters_.push_back(mylog::make_unique<F_formatter>(sp_info));
            break;

        case ('G'):  
            formatters_.push_back(mylog::make_unique<G_formatter>(sp_info));
            break;

        case ('T'):  
            formatters_.push_back(mylog::make_unique<T_formatter>(sp_info));
            break;

        case ('W'):
            formatters_.push_back(mylog::make_unique<duration_formatter<nanoseconds> >(sp_info));
            break;

        case ('X'):
            formatters_.push_back(mylog::make_unique<duration_formatter<microseconds> >(sp_info));
            break;

        case ('Y'):
            formatters_.push_back(mylog::make_unique<duration_formatter<milliseconds> >(sp_info));
            break;

        case ('Z'):
            formatters_.push_back(mylog::make_unique<duration_formatter<seconds> >(sp_info));
            break;

        case ('p'):  
            formatters_.push_back(mylog::make_unique<p_formatter>(sp_info));
            break;

        case ('t'):  
            formatters_.push_back(mylog::make_unique<t_formatter>(sp_info));
            break;

        case ('v'):  
            formatters_.push_back(mylog::make_unique<v_formatter>(sp_info));
            break;

        case ('^'):  
            formatters_.push_back(mylog::make_unique<color_start_formatter>(sp_info));
            break;

        case ('$'):  
            formatters_.push_back(mylog::make_unique<color_end_formatter>(sp_info));
            break;

        case ('@'):  
            formatters_.push_back(mylog::make_unique<source_loc_formatter>(sp_info));
            break;

        case ('A'):  
            formatters_.push_back(mylog::make_unique<source_filename_formatter>(sp_info));
            break;

        case ('B'):  
            formatters_.push_back(mylog::make_unique<source_linenum_formatter>(sp_info));
            break;

        case ('C'):  
            formatters_.push_back(mylog::make_unique<source_funcname_formatter>(sp_info));
            break;

        case ('%'):  
            formatters_.push_back(mylog::make_unique<char_formatter>('%'));
            break;

        // 格式字符未找到
        default:
            auto unknown_flag = mylog::make_unique<aggregate_formatter>();
            unknown_flag->add_ch('%');
            unknown_flag->add_ch(flag);
            formatters_.push_back(std::move(unknown_flag));
    }
}