#ifndef  __curl_cpp_shared_mutex_HPP__
# define __curl_cpp_shared_mutex_HPP__

# include <pthread.h>
# include <system_error>

namespace curl::util {
class shared_mutex {
    pthread_rwlock_t rwlock;

public:
    /**
     * @Exception std::system_error.
     */
    shared_mutex();

    shared_mutex(const shared_mutex&) = delete;
    shared_mutex(shared_mutex&&) = delete;

    shared_mutex& operator = (const shared_mutex&) = delete;
    shared_mutex& operator = (shared_mutex&&) = delete;

    ~shared_mutex();

    auto lock() noexcept;
    /**
     * Undefined behavior if deadlocks.
     */
    auto lock_shared() noexcept;

    auto unlock() noexcept;
};
} /* namespace curl::util */

#endif
