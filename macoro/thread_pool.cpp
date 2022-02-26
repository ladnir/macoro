#include "thread_pool.h"



thread_local macoro::detail::thread_pool_state * macoro::detail::thread_pool_state::mCurrentExecutor;