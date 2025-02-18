#pragma once

#include "definitions.h"
#include "base/registry.h"
#include "async_logger.h"
#include "base/lock_thread_pool.h"

#include <mutex>

namespace learnlog {

template <typename Threadpool>
struct async_factory_template {
    template <typename Sink, typename... SinkArgs>
    static async_logger_shr_ptr create(std::string logger_name, SinkArgs &&...args) {
        static std::mutex async_factory_mutex_;
        std::lock_guard<std::mutex> lock(async_factory_mutex_);
        auto tp = base::registry::instance().get_thread_pool();
        if (tp == nullptr) {
            tp = std::make_shared<Threadpool>();
            base::registry::instance().register_thread_pool(tp);
        }
        
        auto sink = std::make_shared<Sink>(std::forward<SinkArgs>(args)...);
        auto new_logger = std::make_shared<async_logger>(std::move(logger_name), 
                                                         std::move(sink),
                                                         std::move(tp));
        try {
            base::registry::instance().initialize_logger(new_logger);
        }
        LEARNLOG_CATCH
        
        return new_logger;
    }
};

using async_factory = async_factory_template<base::lock_thread_pool>;

}   // namespace learnlog