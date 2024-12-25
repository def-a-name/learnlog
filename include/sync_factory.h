#pragma once

#include "base/registry.h"

namespace learnlog {
class logger;

// sync_factory 是同步模式下的工厂类，提供了创建 logger 对象的统一接口，
// 创建时需要指定 logger 使用的 sink、logger 的名称以及 sink 的创建参数，
// 调用 initialize_logger() 时，使用 LEARNLOG_CATCH 捕获异常，在 stdout 输出错误信息，但不终止程序

struct sync_factory {
    template <typename Sink, typename... SinkArgs>
    static logger_shr_ptr create(std::string logger_name, SinkArgs &&...args) {
        auto sink = std::make_shared<Sink>(std::forward<SinkArgs>(args)...);
        auto new_logger = std::make_shared<logger>(std::move(logger_name), std::move(sink));
        
        try {
            base::registry::instance().initialize_logger(new_logger);
        }
        LEARNLOG_CATCH
        
        return new_logger;
    }
};

}   // namespace learnlog