#include "learnlog.h"

using namespace learnlog;

// 对 logger 增删改查的过程中出现异常时，
// 使用 LEARNLOG_CATCH 捕获异常，在 stdout 输出错误信息，但不终止程序

void learnlog::initialize_logger(logger_shr_ptr new_logger) {
    try {
        base::registry::instance().initialize_logger(std::move(new_logger));
    }
    LEARNLOG_CATCH
}

void learnlog::register_logger(logger_shr_ptr new_logger) {
    try {
        base::registry::instance().register_logger(std::move(new_logger));
    }
    LEARNLOG_CATCH
}

void learnlog::set_auto_register_logger(bool flag) {
    base::registry::instance().set_auto_register_logger(flag);
}

logger_shr_ptr learnlog::get_logger(const std::string& logger_name) {
    return base::registry::instance().get_logger(logger_name);
}

void learnlog::remove_logger(const std::string& logger_name) {
    base::registry::instance().remove_logger(logger_name);
}

logger_shr_ptr learnlog::get_default_logger() {
    return base::registry::instance().get_default_logger();
}

void learnlog::set_default_logger(logger_shr_ptr new_default_logger) {
    base::registry::instance().set_default_logger(std::move(new_default_logger));
}

void learnlog::set_global_pattern(std::string pattern) {
    base::registry::instance().set_global_pattern(std::move(pattern));
}

void learnlog::set_global_formatter(formatter_uni_ptr new_formatter) {
    base::registry::instance().set_global_formatter(std::move(new_formatter));
}

void learnlog::set_pattern(const std::string& logger_name, std::string pattern) {
    try {
        base::registry::instance().set_pattern(logger_name, std::move(pattern));
    }
    LEARNLOG_CATCH
}

void learnlog::set_global_log_level(level::level_enum log_level) {
    base::registry::instance().set_global_log_level(log_level);
}

void learnlog::set_log_level(const std::string& logger_name, level::level_enum log_level) {
    try {
        base::registry::instance().set_log_level(logger_name, log_level);
    }
    LEARNLOG_CATCH
}

void learnlog::set_global_flush_level(level::level_enum flush_level) {
    base::registry::instance().set_global_flush_level(flush_level);
}

void learnlog::set_flush_level(const std::string& logger_name, level::level_enum flush_level) {
    try {
        base::registry::instance().set_flush_level(logger_name, flush_level);
    }
    LEARNLOG_CATCH
}

void learnlog::exec_all(const std::function<void(logger_shr_ptr)>& func) {
    base::registry::instance().exec_all(func);
}

void learnlog::flush_all() {
    base::registry::instance().flush_all();
}

void learnlog::remove_all() {
    base::registry::instance().remove_all();
}

void learnlog::register_thread_pool(thread_pool_shr_ptr new_thread_pool) {
    base::registry::instance().register_thread_pool(std::move(new_thread_pool));
}

thread_pool_shr_ptr learnlog::get_thread_pool() {
    return base::registry::instance().get_thread_pool();
}

void learnlog::close() {
    try {
        base::registry::instance().close_registry();
    }
    LEARNLOG_CATCH
}
