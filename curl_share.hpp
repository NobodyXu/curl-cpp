#ifndef  __curl_cpp_curl_share_HPP__
# define __curl_cpp_curl_share_HPP__

# include "curl.hpp"
# include <shared_mutex>

namespace curl {
class Share_base {
    void *curl_share;

public:
    Share_base(Ret_except<void, Exception> &e) noexcept;

    /**
     * Delete cp/mv ctor and assignment:
     *  - such operation is useless;
     *  - is deleted for std::shared_mutex.
     */
    Share_base(const Share_base&) = delete;
    Share_base(Share_base&&) = delete;

    Share_base& operator = (const Share_base&) = delete;
    Share_base& operator = (Share_base&&) = delete;

    /**
     * Defines data to share among Easy_t handles.
     */
    enum class Options {
      cookie = CURL_LOCK_DATA_COOKIE,
      /**
       * share cached DNS hosts.
       *
       * NOTE that Multi_t interface share this by default 
       * without setting this option.
       */
      dns = CURL_LOCK_DATA_DNS, 
      /**
       * SSL session IDs will be shared across the easy handles 
       * using this shared object. 
       *
       * This will reduce the time spent in the SSL handshake 
       * when reconnecting to the same server. 
       *
       * If curl_t::has_ssl_session_sharing_support() == false, setting
       * this option shares nothing.
       *
       * NOTE SSL session IDs are reused within the same easy handle by default.
       */
      ssl_session = CURL_LOCK_DATA_SSL_SESSION,
      /**
       * If curl_t::has_connection_cache_sharing_support() == false, setting
       * this option shares nothing.
       *
       * NOTE that Multi_t interface share this by default 
       * without setting this option.
       */
      connection_cache = CURL_LOCK_DATA_CONNECT,
      /**
       * Share Public Suffix List.
       *
       * If curl_t::has_psl_sharing_support() == false, setting
       * this option shares nothing.
       *
       * NOTE that Multi_t interface share this by default 
       * without setting this option.
       */
      psl = CURL_LOCK_DATA_PSL,
    };

    /**
     * Possible value of data: same as enum class Options
     *
     * Possible value of access:
     *  - CURL_LOCK_ACCESS_SHARED,
     *  - CURL_LOCK_ACCESS_SINGLE,
     * The mutex you used must be pthread_rwlock_t or std::shared_mutex.
     */
    using lock_function_t = void (*)(CURL *handle, curl_lock_data data, curl_lock_access access, void *userptr);
    using unlock_function_t = void (*)(CURL *handle, curl_lock_data data, void *userptr);

    /**
     * Setting lock_func and unlock_func to nullptr disable locking.
     */
    void add_lock(lock_function_t lock_func, unlock_function_t unlock_func, void *userptr) noexcept;

    /**
     * @param option must be one of enum class Options.
     *               Cannot be or-ed value.
     */
    void enable_sharing(Options option) noexcept;
    void disable_sharing(Options option) noexcept;

    void add_easy(Easy_t &easy) noexcept;
    void remove_easy(Easy_t &easy) noexcept;

    /**
     * All easy handler must be removed before Share_base
     * can be destroyed.
     */
    ~Share_base();
};

/**
 * @tparam Shared_mutex_t For shared or single lock, their unlock function must be
 *                        the same function -- Shared_mutex_t::unlock().
 *                        Pass void to disable locking, which make
 *                        multithreaded use unsafe.
 */
template <class Shared_mutex_t>
class Share: public Share_base {
    Shared_mutex_t mutexes[5];

    auto& get_mutex(Options option) noexcept
    {
        Shared_mutex_t *mutex_p;
        switch (option) {
            case Options::cookie:
                mutex_p = &mutexes[0];
            case Options::dns:
                mutex_p = &mutexes[1];
            case Options::ssl_session:
                mutex_p = &mutexes[2];
            case Options::connection_cache:
                mutex_p = &mutexes[3];
            case Options::psl:
                mutex_p = &mutexes[4];
        }
        return *mutex_p;
    }

public:
    Share(Ret_except<void, Exception> &e);

    void enable_multithreaded_share() noexcept
    {
        add_lock([](CURL *handle, curl_lock_data data, curl_lock_access access, void *userptr) noexcept {
            auto &share = *static_cast<Share*>(userptr);
            auto &mutex = share.get_mutex(static_cast<Share_base::Options>(data));

            if (access == CURL_LOCK_ACCESS_SHARED)
                mutex.lock_shared();
            else
                mutex.lock();
        }, [](CURL *handle, curl_lock_data data, void *userptr) noexcept {
            auto &share = *static_cast<Share*>(userptr);
            auto &mutex = share.get_mutex(static_cast<Share_base::Options>(data));

            mutex.unlock();
        }, this);
    }
    void disable_multithreaded_share() noexcept
    {
        add_lock(nullptr, nullptr, nullptr);
    }
};

template <>
class Share<void>: public Share_base {
public:
    using Share_base::Share_base;

    constexpr void enable_multithreaded_share() const noexcept {}
    constexpr void disable_multithreaded_share() const noexcept {}
};
} /* namespace curl */

#endif
