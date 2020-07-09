#ifndef  __curl_cpp_utils_shared_mutex_HPP__
# define __curl_cpp_utils_shared_mutex_HPP__

# include <pthread.h>

namespace curl::util {
/**
 * C++ RAII wrapper for pthread_rwlock_t.
 */
class shared_mutex {
    pthread_rwlock_t rwlock;

public:
    /**
     * On failure, call err to terminate the program.
     */
    shared_mutex() noexcept;

    shared_mutex(const shared_mutex&) = delete;
    shared_mutex(shared_mutex&&) = delete;

    shared_mutex& operator = (const shared_mutex&) = delete;
    shared_mutex& operator = (shared_mutex&&) = delete;

    ~shared_mutex();

    /**
     * Undefined behavior if deadlocks.
     */
    auto lock() noexcept;
    /**
     * Undefined behavior if deadlocks.
     */
    auto lock_shared() noexcept;

    auto unlock() noexcept;
};
} /* namespace curl::util */

#endif
