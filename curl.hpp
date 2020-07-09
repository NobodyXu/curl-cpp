#ifndef  __curl_cpp_curl_HPP__
# define __curl_cpp_curl_HPP__

# include <cstdint>
# include <cstdio>
# include <stdexcept>
# include <memory>

# include "utils/unique_ptr_pair.hpp"
# include "return-exception/ret-exception.hpp"

namespace curl {
class Exception: public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
class libcurl_bug: public Exception {
public:
    using Exception::Exception;
};
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
 * Must be defined as a static global variable in main
 */
class curl_t {
public:
    struct Version {
        std::uint32_t num;

        static constexpr Version from(std::uint8_t major, std::uint8_t minor, std::uint8_t patch) noexcept;

        constexpr std::uint8_t get_major() const noexcept;
        constexpr std::uint8_t get_minor() const noexcept;
        constexpr std::uint8_t get_patch() const noexcept;

        constexpr bool operator < (const Version &other) const noexcept;
        constexpr bool operator <= (const Version &other) const noexcept;
        constexpr bool operator > (const Version &other) const noexcept;
        constexpr bool operator >= (const Version &other) const noexcept;
        constexpr bool operator == (const Version &other) const noexcept;
        constexpr bool operator != (const Version &other) const noexcept;

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
     * @Precondition has_disable_signal_handling_support()
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

    // User program should not modify these variable
    Version version;
    const void * version_info;
    const char * version_str;

    /**
     * Since curl_t is designed to be used as astatic variable,
     * it would call errx on error.
     *
     * It would make sure that handle_t::get_response_code() is usable before initializing libcurl.
     *
     * This is not thread-safe.
     */
    curl_t(FILE *stderr_stream_arg) noexcept;

    curl_t(const curl_t&) = delete;
    curl_t(curl_t&&) = delete;

    curl_t& operator = (const curl_t&) = delete;
    curl_t& operator = (curl_t&&) = delete;

    ~curl_t();

    /**
     * Any function below in this class is thread-safe!
     */

    bool has_compression_support() const noexcept;
    bool has_largefile_support() const noexcept;
    /**
     * @param protocol should be lower-case
     */
    bool has_protocol(const char *protocol) const noexcept;

    bool has_disable_signal_handling_support() const noexcept;

    bool has_readfunc_abort_support() const noexcept;
    bool has_header_option_support() const noexcept;
    bool has_set_ip_addr_only_support() const noexcept;

    bool has_sizeof_upload_support() const noexcept;
    bool has_sizeof_response_header_support() const noexcept;
    bool has_sizeof_response_body_support() const noexcept;
    bool has_transfer_time_support() const noexcept;
    bool has_redirect_url_support() const noexcept;

    bool has_buffer_size_tuning_support() const noexcept;
    bool has_buffer_size_growing_support() const noexcept;

    bool has_get_active_socket_support() const noexcept;

    struct Easy_deleter {
        void operator () (void *p) const noexcept;
    };

    using Easy_t = util::unique_ptr_pair<char, Easy_deleter, char[]>;
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
     * @return If Easy_t::p1 == nullptr, then curl_easy cannot be created.
     *          - It can be no memory left;
     *          - Or, initialization code for some part of lib failed.
     *         If Easy_t::p2 == nullptr, then it means not enough memory.
     * 
     * If stderr_stream set to non-NULL, verbose info will be printed
     * there.
     * If disable_signal_handling_v is set, signal handling is disabled.
     */
    auto create_easy(std::size_t buffer_size = 0) noexcept -> Easy_t;
    /**
     * @param e must not be nullptr
     * @param buffer_size same as create_easy
     * @preturn same as create_easy
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
     * If stderr_stream set to non-NULL, verbose info will be printed
     * there.
     * If disable_signal_handling_v is set, signal handling is disabled.
     *
     * This function is not thread-safe. e must not be used in any way during this functino call.
     */
    auto dup_easy(const Easy_t &e, std::size_t buffer_size = 0) noexcept -> Easy_t;

    /**
     * has curl::Url support
     */
    bool has_CURLU() const noexcept;

    struct Url_deleter {
        void operator () (char *p) const noexcept;
    };
    using Url_t = std::unique_ptr<char, Url_deleter>;

    /**
     * @return nullptr if not enough memory.
     *
     * Since Url_ref_t doesn't have any other data except pointer to
     * CURLU itself, returning std::unique_ptr instead of an object like Easy_t 
     * would make it easier to manage it in custom ways like std::shared_ptr.
     */
    auto create_Url() noexcept -> Url_t;
    /**
     * @param url != nullptr
     *
     * Not thread-safe. url must not be used during this function call.
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

    struct Share_deleter {
        void operator () (char *p) const noexcept;
    };
    using share_t = std::unique_ptr<char, Share_deleter>;
    auto create_share() noexcept -> share_t;
};

union Data_t {
    void *ptr;
    std::uint64_t unsigned_int;
    char spaces[8];
};
} /* namespace curl */

#endif
