#pragma once

#include "definitions.h"
#include "base/registry.h"
#include "base/async_msg.h"
#include "async_logger.h"

#include <mutex>

namespace mylog {

template <async_overflow_method Overflow>
struct async_factory_template {
    template <typename Sink, typename... SinkArgs>
    static async_logger_shr_ptr create(std::string logger_name, SinkArgs &&...args) {
        static std::mutex async_factory_mutex_;
        std::lock_guard<std::mutex> lock(async_factory_mutex_);
        auto tp = base::registry::instance().get_thread_pool();
        if (tp == nullptr) {
            tp = std::make_shared<base::thread_pool>();
            base::registry::instance().register_thread_pool(tp);
        }
        
        auto sink = std::make_shared<Sink>(std::forward<SinkArgs>(args)...);
        auto new_logger = std::make_shared<async_logger>(std::move(logger_name), 
                                                         std::move(sink),
                                                         std::move(tp),
                                                         Overflow);
        try {
            base::registry::instance().initialize_logger(new_logger);
        }
        MYLOG_CATCH
        
        return new_logger;
    }
};

using async_factory = async_factory_template<async_overflow_method::block_wait>;
using async_factory_override_old = async_factory_template<async_overflow_method::override_old>;

}   // namespace mylog