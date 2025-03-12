#include "base/lockfree_concurrent_thread_pool.h"

using namespace learnlog;
using namespace base;

#ifdef LEARNLOG_USE_TLS
    thread_local atomic_token* lockfree_concurrent_thread_pool::token_ = nullptr;
#endif
