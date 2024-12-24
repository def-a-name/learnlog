#pragma once

#include "base/null_mutex.h"
#include <mutex>

namespace mylog {
namespace base {

struct static_mutex {
    using mutex_t = std::mutex;
    static mutex_t &mutex() {
        static mutex_t s_mutex;
        return s_mutex;
    }
};

struct static_nullmutex {
    using mutex_t = null_mutex;
    static mutex_t &mutex() {
        static mutex_t s_mutex;
        return s_mutex;
    }
};

}    // namespace base
}    // namespace mylog