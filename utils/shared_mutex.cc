// For pthread_rwlockattr_setkind_np
#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 200809L

#include "shared_mutex.hpp"
#include <system_error>
#include <cerrno>

#define CHECK(expr)  \
    if ((expr) != 0) \
        throw std::system_error(errno, std::generic_category(), # expr)

#define CHECK2(expr)       \
    do {                   \
        if ((expr) != 0) { \
            e.set_exception<std::system_error>(errno, std::generic_category(), # expr); \
            return; \
        } \
    } while (0)

namespace curl::util {
shared_mutex::shared_mutex()
{
    pthread_rwlockattr_t attr;

    CHECK(pthread_rwlockattr_init(&attr));
    pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);

    CHECK(pthread_rwlock_init(&rwlock, &attr));

    pthread_rwlockattr_destroy(&attr);
}
shared_mutex::shared_mutex(Ret_except<void, std::system_error> &e) noexcept
{
    pthread_rwlockattr_t attr;

    CHECK2(pthread_rwlockattr_init(&attr));
    pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);

    CHECK2(pthread_rwlock_init(&rwlock, &attr));

    pthread_rwlockattr_destroy(&attr);
}
shared_mutex::~shared_mutex()
{
    pthread_rwlock_destroy(&rwlock);
}

auto shared_mutex::lock() noexcept
{
    pthread_rwlock_wrlock(&rwlock);
}
auto shared_mutex::lock_shared() noexcept
{
    int ret;
    do {
        ret = pthread_rwlock_rdlock(&rwlock);
    } while (ret != 0 && errno == EAGAIN);
}

auto shared_mutex::unlock() noexcept
{
    pthread_rwlock_unlock(&rwlock);
}
} /* namespace curl::util */
