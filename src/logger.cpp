#include "logger.h"
#include "sinks/formatters/pattern_formatter.h"
#include "sinks/sink.h"

using namespace learnlog;

void logger::swap(logger& other) noexcept {
    name_.swap(other.name_);
    sinks_.swap(other.sinks_);
    std::swap(log_level_, other.log_level_);
    std::swap(flush_level_, other.flush_level_);
    std::swap(tracer_, other.tracer_);
}

void logger::set_pattern(std::string pattern) {
    pattern_ = pattern;
    auto p_formatter = learnlog::make_unique<sinks::pattern_formatter>(std::move(pattern));
    set_formatter(std::move(p_formatter));
}

void logger::set_formatter(formatter_uni_ptr formatter) {
    for(auto it = sinks_.begin(); it != sinks_.end(); ++it) {
        if(it + 1 == sinks_.end()) { (*it)->set_formatter(std::move(formatter)); }
        else { (*it)->set_formatter(formatter->clone()); }
    }
}

void logger::dump_backtrace() {
    if (tracer_ != nullptr && tracer_->is_enabled()) {
        base::log_msg s{level::info, "*********************** Backtrace Start ***********************", name()};
        sink_log_(s);
        tracer_->foreach_do([this](const base::log_msg& msg) { this->sink_log_(msg); });
        base::log_msg e{level::info, "************************ Backtrace End ************************", name()};
        sink_log_(e);
    }
}

logger_shr_ptr logger::clone(std::string new_name) {
    auto cloned = std::make_shared<logger>(*this);
    cloned->name_ = std::move(new_name);
    return cloned;
}

/* protected */

void logger::do_log_(const base::log_msg& msg, bool log_enabled, bool traceback_enabled) {
    if (log_enabled) {
        sink_log_(msg);
    }
    if (traceback_enabled) {
        tracer_->push_back(msg);
    }
}

void logger::sink_log_(const base::log_msg& msg) {
    for (auto &sink : sinks_) {
        if (sink->should_log(msg.level)) {
            try { sink->log(msg); }
            LEARNLOG_CATCH
        }
    }

    if (should_flush_(msg.level)) {
        flush_sink_();
    }
}

void logger::flush_sink_() {
    for(auto &sink : sinks_) {
        try { sink->flush(); }
        LEARNLOG_CATCH
    }
}


void swap(logger& a, logger& b) { a.swap(b); }