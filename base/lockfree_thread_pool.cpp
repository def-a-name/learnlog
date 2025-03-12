#include "base/lockfree_thread_pool.h"

using namespace learnlog;
using namespace base;

#ifdef LEARNLOG_USE_TLS
    thread_local lockfree_thread_pool::c_token_uni_ptr lockfree_thread_pool::token_ = nullptr;
#endif
