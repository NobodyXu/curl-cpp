#ifndef  __curl_cpp_curl_share_HPP__
# define __curl_cpp_curl_share_HPP__

# include "curl.hpp"
# include "utils/shared_mutex.hpp"
# include "return-exception/ret-exception.hpp"

# include <curl/curl.h>
# include <memory>

namespace curl {
/**
 * @example curl_share.cc
 *
 * All easy handler must be removed before Share_base
 * can be destroyed.
 *
 * To use in a thread-safe manner, you must call
 * add_lock with non-nullptr.
 */
class Share_base {
protected:
    curl_t::Share_t curl_share;

public:
    /**
     * @param share must be != nullptr
     *
     * After this ctor call, share.get() == nullptr,
     * this class will take over the ownership.
     */
    Share_base(curl_t::Share_t &&share) noexcept;

    /**
     * Since libcurl doesn't provide a dup 
     * function for curl_share, neither will Share_base.
     */
    Share_base(const Share_base&) = delete;

    /**
     * @param other bool(other) can be false, but then this new instance
     * of Share_base will be unusable.
     */
    Share_base(Share_base &&other) noexcept;

    /**
     * Since libcurl doesn't provide a dup 
     * function for curl_share, neither will Share_base.
     */
    Share_base& operator = (const Share_base&) = delete;
    /**
     * @param other if bool(other) is true, then it will be unusable after this operation.
     *              <br>Otherwise, this object will also be destroyed.
     * @param this this object can also be an unusable object which bool(*this) is false.
     *             <br>Calling this function on an unusable object will make that object again
     *             usable.
     *
     * **NOTE that if this object still holds easy handler and bool(other) is false,
     * then it is undefined behavior.**
     *
     */
    Share_base& operator = (Share_base &&other) noexcept;

    /**
     * @return true if this object is usable, false otherwise.
     */
    operator bool () const noexcept;

    /**
     * Defines data to share among Easy_t handles.
     */
    enum class Options {
        none = CURL_LOCK_DATA_NONE,
        cookie = CURL_LOCK_DATA_COOKIE,
        /**
         * share cached DNS hosts.
         *
         * If the libcurl has cookies disabled, then 
         * enable_sharing(Options::cookie) will return 0.
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
    /**
     * Possible value of data: same as enum class Options
     *
     * Possible value of access:
     *  - CURL_LOCK_ACCESS_SHARED,
     *  - CURL_LOCK_ACCESS_SINGLE,
     * The mutex you used must be pthread_rwlock_t or std::shared_mutex.
     */
    using unlock_function_t = void (*)(CURL *handle, curl_lock_data data, void *userptr);

    /**
     * Setting lock_func and unlock_func to nullptr disable locking.
     */
    void add_lock(lock_function_t lock_func, unlock_function_t unlock_func, void *userptr) noexcept;

    /**
     * @param option must be one of enum class Options.
     *               <br>Cannot be or-ed value.
     *
     * All sharing enable/disable must be done when no easy is added
     * or all easy is removed.
     */
    auto enable_sharing(Options option) noexcept -> Ret_except<int, std::bad_alloc>;
    /**
     * @param option must be one of enum class Options.
     *               <br>Cannot be or-ed value.
     *
     * All sharing enable/disable must be done when no easy is added
     * or all easy is removed.
     */
     void disable_sharing(Options option) noexcept;

    void add_easy(Easy_ref_t &easy) noexcept;
    void remove_easy(Easy_ref_t &easy) noexcept;
};

/**
 * @tparam Shared_mutex_t For shared or single lock, their unlock function must be
 *                        the same function -- Shared_mutex_t::unlock().
 *                        <br>If Shared_mutex_t lock, lock_shared or unlock
 *                        throw an exception, it would terminte the program.
 *                        <br>If Shared_mutex_t has type Ret_except_t (Ret == void), then 
 *                        Shared_mutex_t(Ret_except_t&) would be called.
 *                        <br>Pass void to disable locking, which make
 *                        multithreaded use unsafe.
 */
template <class Shared_mutex_t = utils::shared_mutex>
class Share: public Share_base {
    static constexpr const std::size_t mutex_num = 5;

    Shared_mutex_t mutexes[mutex_num];

    auto get_mutex(Options option) noexcept -> Shared_mutex_t*
    {
        switch (option) {
            case Options::none:
                return nullptr;
            case Options::cookie:
                return &mutexes[0];
            case Options::dns:
                return &mutexes[1];
            case Options::ssl_session:
                return &mutexes[2];
            case Options::connection_cache:
                return &mutexes[3];
            case Options::psl:
                return &mutexes[4];
            default:
                return nullptr;
        }
    }

public:
    using Share_base::Share_base;

    /**
     * @param base any usable Share_base object.
     */
    Share(Share_base &&base) noexcept:
        Share_base{std::move(base)}
    {}

    void enable_multithreaded_share() noexcept
    {
        add_lock([](CURL *handle, curl_lock_data data, curl_lock_access access, void *userptr) noexcept {
            auto &share = *static_cast<Share*>(userptr);
            auto *mutex = share.get_mutex(static_cast<Share_base::Options>(data));

            if (!mutex)
                return;

            if (access == CURL_LOCK_ACCESS_SHARED)
                mutex->lock_shared();
            else
                mutex->lock();
        }, [](CURL *handle, curl_lock_data data, void *userptr) noexcept {
            auto &share = *static_cast<Share*>(userptr);
            auto *mutex = share.get_mutex(static_cast<Share_base::Options>(data));

            if (mutex)
                mutex->unlock();
        }, this);
    }
    void disable_multithreaded_share() noexcept
    {
        add_lock(nullptr, nullptr, nullptr);
    }
};

/**
 * Sepecialiation of Share that does no locking, thus
 * is multithread unsafe.
 *
 * This provides the same interface as any other Share<T>, 
 * so that your template can simply pass void to 
 * disable locking.
 */
template <>
class Share<void>: public Share_base {
public:
    using Share_base::Share_base;

    constexpr void enable_multithreaded_share() const noexcept {}
    constexpr void disable_multithreaded_share() const noexcept {}
};
} /* namespace curl */

#endif
