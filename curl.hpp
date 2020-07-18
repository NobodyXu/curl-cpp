#ifndef  __curl_cpp_curl_HPP__
# define __curl_cpp_curl_HPP__

# include <cstdint>
# include <cstdio>
# include <stdexcept>
# include <memory>

# include "return-exception/ret-exception.hpp"

namespace curl {
/**
 * Base class for all exceptions can throw
 * via Ret_except in this library.
 */
class Exception: public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
/**
 * This exception mean that the libcurl found at
 * runtime doesn't support particular feature/function
 * you called.
 */
class NotBuiltIn_error: public Exception {
public:
    using Exception::Exception;
};
/**
 * Bugs in the libcurl found at runtime!
 */
class libcurl_bug: public Exception {
public:
    using Exception::Exception;
};
/**
 * You call libcurl API which perform callback from libcurl
 * callback!
 */
class Recursive_api_call_Exception: public Exception {
public:
    using Exception::Exception;
};

class Share_base;
template <class Shared_mutex_t>
class Share;

/**
 * It is unsafe to use any of class defined below in multithreaded environment without synchronization.
 */
class Easy_ref_t;
class Multi_t;
class Url_ref_t;

/**
 * @warning Must be defined before any thread is created.
 *
 * There can be multiple instances of this object, as long as during
 * ctor and dtor, there is only one thread in the program.
 */
class curl_t {
public:
    /**
     * @example curl_version.cc
     *
     * This struct encapsulate operations on libcurl version.
     */
    struct Version {
        std::uint32_t num;

        static constexpr Version from(std::uint8_t major, std::uint8_t minor, std::uint8_t patch) noexcept;

        constexpr std::uint8_t get_major() const noexcept;
        constexpr std::uint8_t get_minor() const noexcept;
        constexpr std::uint8_t get_patch() const noexcept;

        /**
         * Check whether version x is older than version y.
         */
        friend constexpr bool operator <  (const Version &x, const Version &y) noexcept;
        /**
         * Check whether version x is older than or the same as version y.
         */
        friend constexpr bool operator <= (const Version &x, const Version &y) noexcept;
        /**
         * Check whether version x is newer than version y.
         */
        friend constexpr bool operator >  (const Version &x, const Version &y) noexcept;
        /**
         * Check whether version x is newer than or the same as version y.
         */
        friend constexpr bool operator >= (const Version &x, const Version &y) noexcept;
        /**
         * Check whether version x is the same as version y.
         */
        friend constexpr bool operator == (const Version &x, const Version &y) noexcept;
        /**
         * Check whether version x is not the same as version y.
         */
        friend constexpr bool operator != (const Version &x, const Version &y) noexcept;

        /**
         * @param buffer is required to be at least 12 bytes long.
         *               Will be write in format "uint8_t.uint8_t.uint8_t" (with trailing '\0').
         * @return If success, the number of characters writen to buffer (excluding trailing '\0');
         *         If failed, return negative value.
         */
        std::size_t to_string(char buffer[12]) const noexcept;
    };

    /**
     * Modification of the variable below is sure not thread-safe.
     */
    FILE *stderr_stream;

    /**
     * @pre has_disable_signal_handling_support()
     *
     * If your libcurl uses standard name resolver, disable signal handling might cause timeout to never 
     * occur during name resolution.
     *
     * Disable signal handling makes libcurl NOT ask the system to ignore SIGPIPE signals, which otherwise 
     * are sent by the system when trying to send data to a socket which is closed in the other end. 
     *
     * libcurl makes an effort to never cause such SIGPIPEs to trigger, but some operating systems have 
     * no way to avoid them and even on those that have there are some corner cases when they may still happen, 
     * contrary to our desire. 
     *
     * In addition, using CURLAUTH_NTLM_WB authentication could cause a SIGCHLD signal to be raised.
     *
     * Modifing this would only affect Easy_t handle created after the modification.
     */
    bool disable_signal_handling_v = false;

    /**
     * Result of curl_version_info(CURLVERSION_NOW).
     * Cached for faster access.
     */
    const void * const version_info;
    /**
     * Version of the libcurl loaded dynamically.
     */
    const Version version;
    /**
     * Version of the libcurl linked dynamically in string.
     */
    const char * const version_str;

    /**
     * Since curl_t is designed to be usable as static variable,
     * it would call errx on error.
     *
     * It would make sure that handle_t::get_response_code() is usable before initializing libcurl.
     *
     * This is not thread-safe.
     */
    curl_t(FILE *stderr_stream_arg) noexcept;

    using malloc_callback_t = void* (*)(std::size_t size);
    using free_callback_t = void (*)(void *ptr);
    using realloc_callback_t = void* (*)(void *old_ptr, std::size_t size);
    using strdup_callback_t = char* (*)(const char *str);
    using calloc_callback_t = void* (*)(std::size_t nmemb, std::size_t size);

    /**
     * If you are using libcurl from multiple threads or libcurl was built 
     * with the threaded resolver option, then the callback functions 
     * must be thread safe. 
     *
     * The threaded resolver is a common build option to enable 
     * (and in some cases the default) so portable application should make
     * these callback functions thread safe.
     * 
     * All callback arguments must be set to valid function pointers. 
     *
     * libcurl support this from 7.12.0.
     * If this wasn't supported, you binary probably would have problem
     * during dynamic binding.
     *
     * it would call errx on error.
     *
     * It would make sure that handle_t::get_response_code() is usable before initializing libcurl.
     *
     * This is not thread-safe.
     */
    curl_t(FILE *stderr_stream_arg, 
           malloc_callback_t  malloc_callback,
           free_callback_t    free_callback,
           realloc_callback_t realloc_callback,
           strdup_callback_t  strdup_callback,
           calloc_callback_t  calloc_callback) noexcept;

    curl_t(const curl_t&) = delete;
    curl_t(curl_t&&) = delete;

    curl_t& operator = (const curl_t&) = delete;
    curl_t& operator = (curl_t&&) = delete;

    /**
     * Destructor is not thread safe.
     *
     * You must not call it when any other thread in the program 
     * (i.e. a thread sharing the same memory) is running. 
     *
     * This doesn't just mean no other thread that is using libcurl. 
     * Because curl_global_cleanup calls functions of other libraries that 
     * are similarly thread unsafe, it could conflict with any other thread that 
     * uses these other libraries.
     *
     * This dtor does not block waiting for any libcurl-created threads 
     * to terminate (such as threads used for name resolving). 
     *
     * If a module containing libcurl is dynamically unloaded while 
     * libcurl-created threads are still running then your program 
     * may crash or other corruption may occur. 
     *
     * We recommend you do not run libcurl from any module that may be 
     * unloaded dynamically. 
     *
     * This behavior may be addressed in the future.
     */
    ~curl_t();

    bool has_pause_support() const noexcept;

    bool has_compression_support() const noexcept;
    bool has_largefile_support() const noexcept;
    /**
     * @param protocol should be lower-case
     */
    bool has_protocol(const char *protocol) const noexcept;
    bool has_ssl_support() const noexcept;
    bool has_ipv6_support() const noexcept;

    bool has_erase_all_cookies_in_mem_support() const noexcept;
    bool has_erase_all_session_cookies_in_mem_support() const noexcept;
    bool has_flush_cookies_to_jar() const noexcept;
    bool has_reload_cookies_from_file() const noexcept;

    bool has_disable_signal_handling_support() const noexcept;

    bool has_private_ptr_support() const noexcept;

    bool has_readfunc_abort_support() const noexcept;
    bool has_header_option_support() const noexcept;
    bool has_set_ip_addr_only_support() const noexcept;

    bool has_sizeof_upload_support() const noexcept;
    bool has_sizeof_response_header_support() const noexcept;
    bool has_sizeof_response_body_support() const noexcept;
    bool has_transfer_time_support() const noexcept;
    bool has_redirect_url_support() const noexcept;

    bool has_getinfo_cookie_list_support() const noexcept;

    bool has_buffer_size_tuning_support() const noexcept;
    bool has_buffer_size_growing_support() const noexcept;

    bool has_get_active_socket_support() const noexcept;

    /**
     * Deleter for curl::curl_t::Easy_t.
     */
    struct Easy_deleter {
        void operator () (void *p) const noexcept;
    };

    /**
     * RAII wrapper for curl's easy handler.
     */
    using Easy_t = std::unique_ptr<char, Easy_deleter>;
    /**
     * @param buffer_size size of receiver buffer.
     *
     *                    This buffer size is by default CURL_MAX_WRITE_SIZE (16kB). 
     *                    The maximum buffer size allowed to be set is CURL_MAX_READ_SIZE (512kB). 
     *                    The minimum buffer size allowed to be set is 1024.
     *
     *                    Set to 0 to use default value.
     *
     *                    This param is just treated as a request, not an order.
     * @return If  == nullptr, then curl_easy cannot be created.
     *          - It can be no memory left;
     *          - Or, initialization code for some part of lib failed.
     * 
     * If stderr_stream set to non-NULL, verbose info will be printed
     * there.
     * If disable_signal_handling_v is set, signal handling is disabled.
     *
     * As long as stderr_stream and disable_signal_handling_v is not modified
     * when create_easy is called, this function is thread-safe.
     */
    auto create_easy(std::size_t buffer_size = 0) noexcept -> Easy_t;
    /**
     * @param easy must not be nullptr
     * @param buffer_size same as create_easy
     * @return same as create_easy
     *
     * All string passed into curl_easy_setopt using char* will be pointed by the 
     * new hanlde as well.
     * Thus they must be kept around until both handles is destroyed.
     *
     * The new handle will not inherit any state information, no connections, 
     * no SSL sessions and no cookies. 
     *
     * It also will not inherit any share object states or options 
     * (it will be made as if CURLOPT_SHARE was set to NULL).
     *
     * If stderr_stream set to non-nullptr, verbose info will be printed
     * there.
     * If disable_signal_handling_v is set, signal handling is disabled.
     *
     * easy must not be used in any way during this function call.
     */
    auto dup_easy(const Easy_t &easy, std::size_t buffer_size = 0) noexcept -> Easy_t;

    /**
     * has curl::Url support
     */
    bool has_CURLU() const noexcept;

    /**
     * Deleter for curl::curl_t::Url_t
     */
    struct Url_deleter {
        void operator () (char *p) const noexcept;
    };
    /**
     * RAII wrapper for curl's Url parser.
     */
    using Url_t = std::unique_ptr<char, Url_deleter>;

    /**
     * @return nullptr if not enough memory.
     *
     * Since Url_ref_t doesn't have any other data except pointer to
     * CURLU itself, returning std::unique_ptr instead of an object like Easy_t 
     * would make it easier to manage it in custom ways like std::shared_ptr.
     *
     * It is thread-safe.
     */
    auto create_Url() noexcept -> Url_t;
    /**
     * @param url != nullptr
     *
     * url must not be used during this function call.
     */
    auto dup_Url(const Url_t &url) noexcept -> Url_t;

    bool has_multi_poll_support() const noexcept;
    bool has_multi_socket_support() const noexcept;

    /**
     * http2 multiplex is turn on by default, if supported.
     */
    bool has_http2_multiplex_support() const noexcept;
    bool has_max_concurrent_stream_support() const noexcept;

    /**
     * NOTE that http1 pipeline is always disabled.
     */
    auto create_multi() noexcept -> Ret_except<Multi_t, curl::Exception>;

    bool has_ssl_session_sharing_support() const noexcept;
    bool has_connection_cache_sharing_support() const noexcept;
    bool has_psl_sharing_support() const noexcept;

    /**
     * Deleter for curl::curl_t::Share_t
     */
    struct Share_deleter {
        void operator () (char *p) const noexcept;
    };
    /**
     * RAII wrapper for curl's Share handler.
     */
    using Share_t = std::unique_ptr<char, Share_deleter>;
    /**
     * Create share handler.
     *
     * It is thread-safe.
     */
    auto create_share() noexcept -> Share_t;
};

using Easy_t = curl_t::Easy_t;
using Url_t = curl_t::Url_t;
using Share_t = curl_t::Share_t;
} /* namespace curl */

#endif
