// For pthread_rwlockattr_setkind_np
#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 200809L

#include "shared_mutex.hpp"
#include <cerrno>
#include <err.h>

#define CHECK(expr)  \
    if ((expr) != 0) \
        err(1, "In %s" # expr " failed", __PRETTY_FUNCTION__)

namespace curl::utils {
shared_mutex::shared_mutex() noexcept
{
    pthread_rwlockattr_t attr;

    CHECK(pthread_rwlockattr_init(&attr));
    pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);

    CHECK(pthread_rwlock_init(&rwlock, &attr));

    pthread_rwlockattr_destroy(&attr);
}
shared_mutex::~shared_mutex()
{
    pthread_rwlock_destroy(&rwlock);
}

void shared_mutex::lock() noexcept
{
    pthread_rwlock_wrlock(&rwlock);
}
void shared_mutex::lock_shared() noexcept
{
    int ret;
    do {
        ret = pthread_rwlock_rdlock(&rwlock);
    } while (ret != 0 && errno == EAGAIN);
}

void shared_mutex::unlock() noexcept
{
    pthread_rwlock_unlock(&rwlock);
}
} /* namespace curl::utils */
