#include "base/registry.h"
#include "base/exception.h"
#include "sinks/std_color_sinks.h"
#include "logger.h"

using namespace learnlog;
using namespace base;

/* public */

void registry::initialize_logger(logger_shr_ptr new_logger) {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    new_logger->set_formatter(global_formatter_->clone());
    std::string pattern = global_pattern_;
    new_logger->set_pattern(pattern);
    new_logger->set_log_level(global_log_level_);
    new_logger->set_flush_level(global_flush_level_);
    
    if (auto_register_logger_) {
        register_logger_(std::move(new_logger));
    }
}

void registry::register_logger(logger_shr_ptr new_logger) {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    register_logger_(std::move(new_logger));
}

void registry::set_auto_register_logger(bool flag) {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    auto_register_logger_ = flag;
}

logger_shr_ptr registry::get_logger(const std::string& logger_name) {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    auto target = loggers_.find(logger_name);
    return target == loggers_.end() ? nullptr : target->second;
}

void registry::remove_logger(const std::string& logger_name) {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    loggers_.erase(logger_name);

    if (default_logger_ && default_logger_->name() == logger_name) {
        default_logger_.reset();
    }
}

logger_shr_ptr registry::get_default_logger() {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    return default_logger_;
}

void registry::set_default_logger(logger_shr_ptr new_default_logger) {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    if (new_default_logger != nullptr) {
        loggers_[new_default_logger->name()] = new_default_logger;
    }
    default_logger_ = std::move(new_default_logger);
}

void registry::set_global_pattern(std::string pattern) {
    {
        std::lock_guard<std::mutex> lock(loggers_mutex_);
        global_pattern_ = pattern;
    }
    auto new_pattern_formatter = 
        learnlog::make_unique<sinks::pattern_formatter>(std::move(pattern));
    set_global_formatter(std::move(new_pattern_formatter));
}

void registry::set_global_formatter(formatter_uni_ptr new_formatter) {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    global_formatter_ = std::move(new_formatter);
    for (auto &l : loggers_) {
        l.second->set_formatter(global_formatter_->clone());
    }
}

void registry::set_pattern(const std::string& logger_name, std::string pattern) {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    auto target = loggers_.find(logger_name);
    if (target != loggers_.end()) {
        target->second->set_pattern(std::move(pattern));
    }
    else {
        std::string err_str = fmt::format(
            "learnlog::registry: set_logger_pattern() failed: "
            "logger with name '{}' does not exist", logger_name);
        throw_learnlog_excpt(err_str);
    }
}

void registry::set_log_level(const std::string& logger_name, level::level_enum log_level) {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    auto target = loggers_.find(logger_name);
    if (target != loggers_.end()) {
        target->second->set_log_level(log_level);
    }
    else {
        std::string err_str = fmt::format(
            "learnlog::registry: set_log_level() failed: "
            "logger with name '{}' does not exist", logger_name);
        throw_learnlog_excpt(err_str);
    }
}

void registry::set_global_log_level(level::level_enum log_level) {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    global_log_level_ = log_level;
    for (auto &l : loggers_) {
        l.second->set_log_level(log_level);
    }
}

void registry::set_flush_level(const std::string& logger_name, level::level_enum flush_level) {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    auto target = loggers_.find(logger_name);
    if (target != loggers_.end()) {
        target->second->set_flush_level(flush_level);
    }
    else {
        std::string err_str = fmt::format(
            "learnlog::registry: set_flush_level() failed: "
            "logger with name '{}' does not exist", logger_name);
        throw_learnlog_excpt(err_str);
    }
}

void registry::set_global_flush_level(level::level_enum flush_level) {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    global_flush_level_ = flush_level;
    for (auto &l : loggers_) {
        l.second->set_flush_level(flush_level);
    }
}

void registry::exec_all(const std::function<void(logger_shr_ptr)>& func) {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    for (auto &l : loggers_) {
        func(l.second);
    }
}

void registry::flush_all() {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    for (auto &l : loggers_) {
        l.second->flush();
    }
}

void registry::remove_all() {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    loggers_.clear();
    default_logger_.reset();
}

void registry::register_thread_pool(thread_pool_shr_ptr new_thread_pool) {
    std::lock_guard<std::mutex> lock(thread_mutex_);
    thread_pool_ = std::move(new_thread_pool);
}

thread_pool_shr_ptr registry::get_thread_pool() {
    std::lock_guard<std::mutex> lock(thread_mutex_);
    return thread_pool_;
}

void registry::close_registry() {
    {
        std::lock_guard<std::mutex> lock(flusher_mutex_);
        global_flusher_.reset();
    }
    remove_all();
    {
        std::lock_guard<std::mutex> lock(thread_mutex_);
        thread_pool_.reset();
    }
}

registry& registry::instance() {
    static registry registry_instance;
    return registry_instance;
}

/* private */

registry::registry() 
    : global_formatter_(learnlog::make_unique<sinks::pattern_formatter>()) {
    sink_shr_ptr default_sink = std::make_shared<sinks::stdout_color_sink_mt>();
    std::string default_logger_name = "";
    default_logger_ = std::make_shared<logger>(default_logger_name, default_sink);
    loggers_[default_logger_name] = default_logger_;
}

registry::~registry() = default;

void registry::register_logger_(logger_shr_ptr new_logger) {
    std::string logger_name = new_logger->name();
    if (loggers_.find(logger_name) != loggers_.end()) {
        std::string err_str = fmt::format(
            "learnlog::registry: register_logger_() failed: "
            "logger with name '{}' already exists", logger_name);
        throw_learnlog_excpt(err_str);
    }
    loggers_[logger_name] = std::move(new_logger);
}