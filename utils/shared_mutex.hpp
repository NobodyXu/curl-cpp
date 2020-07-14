#ifndef  __curl_cpp_utils_shared_mutex_HPP__
# define __curl_cpp_utils_shared_mutex_HPP__

# include <pthread.h>

namespace curl::utils {
/**
 * C++ RAII wrapper for pthread_rwlock_t.
 *
 * To use this interface, you'd have to add
 * -lpthread to LDFLAGS of your project.
 *
 * This class is designed specifically for
 * curl::Share, which requires a std::shared_mutex
 * like interface, but have to be able to unlock
 * shared lock/normal lock with a single function.
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
    void lock() noexcept;
    /**
     * Undefined behavior if deadlocks.
     */
    void lock_shared() noexcept;

    /**
     * One unlock function for lock()/lock_shared().
     */
    void unlock() noexcept;
};
} /* namespace curl::utils */

#endif
