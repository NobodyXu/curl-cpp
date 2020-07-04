#ifndef  __curl_cpp_curl_HPP__
# define __curl_cpp_curl_HPP__

# include <cstddef>
# include <cstdint>
# include <cstdio>
# include <stdexcept>
# include <memory>
# include <string>

# include <curl/curl.h>

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

class Easy_t;
class Multi_t;
class Url;

/**
 * Must be defined as a static global variable in main
 */
class curl_t {
public:
    struct Version {
        std::uint32_t num;

        static constexpr Version from(std::uint8_t major, std::uint8_t minor, std::uint8_t patch) noexcept;

        std::uint8_t get_major() const noexcept;
        std::uint8_t get_minor() const noexcept;
        std::uint8_t get_patch() const noexcept;

        bool operator < (const Version &other) const noexcept;
        bool operator <= (const Version &other) const noexcept;
        bool operator > (const Version &other) const noexcept;
        bool operator >= (const Version &other) const noexcept;
        bool operator == (const Version &other) const noexcept;
        bool operator != (const Version &other) const noexcept;

        /**
         * @param buffer is required to be at least 12 bytes long.
         *               Will be write in format "uint8_t.uint8_t.uint8_t" (with trailing '\0').
         * @return If success, the number of characters writen to buffer (excluding trailing '\0');
         *         If failed, return negative value.
         */
        std::size_t to_string(char buffer[12]) const noexcept;
    };

    FILE *debug_stream;
    const void * const version_info;
    const Version version;
    const char * const version_str;

    /**
     * Since this variable is designed to be static, 
     * it would call errx on error.
     *
     * It would make sure that handle_t::get_response_code() is usable before initializing libcurl.
     */
    curl_t(FILE *debug_stream_arg) noexcept;

    curl_t(const curl_t&) = delete;
    curl_t(curl_t&&) = delete;

    curl_t& operator = (const curl_t&) = delete;
    curl_t& operator = (curl_t&&) = delete;

    ~curl_t();

    bool has_compression_support() const noexcept;
    bool has_largefile_support() const noexcept;
    /**
     * @param protocol should be lower-case
     */
    bool has_protocol(const char *protocol) const noexcept;

    /**
     * has curl::Url support
     */
    bool has_CURLU(const char *protocol) const noexcept;

    bool has_sizeof_upload_support() const noexcept;
    bool has_sizeof_response_header_support() const noexcept;
    bool has_sizeof_response_body_support() const noexcept;
    bool has_transfer_time_support() const noexcept;

    bool has_multi_support() const noexcept;

    auto create_easy() noexcept -> Ret_except<Easy_t, curl::Exception>;
    auto create_multi() noexcept -> Ret_except<Multi_t, curl::Exception>;
};

union Data_t {
    void *ptr;
    std::uint64_t unsigned_int;
    char spaces[8];
};

class Easy_t {
    void *curl_easy;
    char error_buffer[CURL_ERROR_SIZE];

public:
    /**
     * If return value is less than @param size, then it will singal an err cond to libcurl.
     * This will cause the transfer to get aborted and the libcurl function used will return CURLE_WRITE_ERROR.
     *
     * It would be undefined behavior to call any easy member function in writeback.
	 * 
     * @param buffer not null-terminated
     */
    using writeback_t = std::size_t (*)(char *buffer, std::size_t size, Data_t &data);

    /**
     * If set to nullptr, then all response content are ignored.
     */
    writeback_t writeback = nullptr;
    Data_t data;

public:
    friend Multi_t;

    class Exception: public curl::Exception {
    public:
        const long error_code;

        Exception(long err_code_arg);
        Exception(const Exception&) = default;

        auto what() const noexcept -> const char*;
    };
    class NotBuiltIn_error: public Exception {
    public:
        using Exception::Exception;
    };
    class ProtocolInternal_error: public Exception {
        static constexpr const auto buffer_size = 23 + 2 + CURL_ERROR_SIZE + 1;

    public:
        char buffer[buffer_size];

        /**
         * @error_code can only be one of:
         *  - CURLE_HTTP2
         *  - CURLE_SSL_CONNECT_ERROR
         *  - CURLE_UNKNOWN_OPTION
         *  - CURLE_HTTP3
         */
        ProtocolInternal_error(long error_code, const char *error_buffer);
        ProtocolInternal_error(const ProtocolInternal_error&) = default;

        auto what() const noexcept -> const char*;
    };

    /**
     * @param curl must be ret value of curl_easy_init(), must not be nullptr
     */
    Easy_t(void *curl) noexcept;

    Easy_t(const Easy_t&, Ret_except<void, curl::Exception> &e) noexcept;
    /**
     * @param other after mv operation, other is in invalid state and can only be destroyed
     *              or move assign another value.
     */
    Easy_t(Easy_t &&other) noexcept;

    Easy_t& operator = (const Easy_t&) = delete;
    Easy_t& operator = (Easy_t&&) = delete;

    /**
     * @Precondition curl_t::has_CURLU()
     * @param url content of it must not be changed during call to perform(),
     *            but can be changed once it is finished.
     */
    void set_url(const Url &url) noexcept;
    /**
     * @param url content of it must not be changed during call to perform(),
     *            but can be changed once it is finished.
     */
    auto set_url(const char *url) noexcept -> Ret_except<void, std::bad_alloc>;

    /**
     * @Precondition curl::has_protocol("http")
     * @param useragent pass nullptr for no useragent (default)
     */
    auto set_useragent(const char *useragent) noexcept -> Ret_except<void, std::bad_alloc>;
    /**
     * @Precondition curl::has_protocol("http")
     * @param encoding "" for enable all (default);
     *                 nullptr for disable all (including auto decompression).
     */
    auto set_encoding(const char *encoding) noexcept -> Ret_except<void, std::bad_alloc>;

    /**
     * @param ip_addr can be ipv4 or ipv6 address.
     *                If it is nullptr, then set to whatever TCP stack find available (default).
     */
    auto set_source_ip(const char *ip_addr) noexcept -> Ret_except<void, std::bad_alloc>;

    /**
     * @param timeout in milliseconds. Set to 0 to disable;
     *                should be less than std::numeric_limits<long>::max().
     */
    void set_timeout(unsigned long timeout) noexcept;

    /**
     * @Precondition: curl_t::has_protocol("http")
     */
    void request_get() noexcept;
    /**
     * The data pointed to is NOT copied by the library: as a consequence, it must be preserved by 
     * the calling application until the associated transfer finishes. 
     *
     * @Precondition: curl_t::has_protocol("http"), len <= 2 * 1024 * 1024 * 1024 (2GB)
     */
    void request_post(const void *data, std::uint32_t len) noexcept;
    /**
     * The data pointed to is NOT copied by the library: as a consequence, it must be preserved by 
     * the calling application until the associated transfer finishes. 
     *
     * @Precondition: curl_t::has_protocol("http"), curl_t::has_largefile_support()
     */
    void request_post_large(const void *data, std::size_t len) noexcept;

    /**
     * @Precondition curl_t::has_protocol(protocol you use in url)
     * @exception NotSupported_error, std::bad_alloc or any exception defined in this class
     */
    enum class code {
        ok = 0,
        url_malformat,
        cannot_resolve_proxy,
        cannot_resolve_host,
        cannot_connect, // Cannot connect to host or proxy
        remote_access_denied,
        writeback_error,
        upload_failure,
        timedout,
        recursive_api_call,
    };
    using perform_ret_t = Ret_except<code, std::bad_alloc, std::invalid_argument, std::length_error, 
                                     Exception, NotBuiltIn_error, ProtocolInternal_error>;

    auto perform() noexcept -> perform_ret_t;

    long get_response_code() const noexcept;

    /**
     * @Precondition curl_t::has_sizeof_upload_support()
     * @return in bytes
     */
    std::size_t getinfo_sizeof_uploaded() const noexcept;
    /**
     * @Precondition curl_t::has_sizeof_response_header_support()
     * @return in bytes
     */
    std::size_t getinfo_sizeof_response_header() const noexcept;
    /**
     * @Precondition curl_t::has_sizeof_response_body_support()
     * @return in bytes
     */
    std::size_t getinfo_sizeof_response_body() const noexcept;

    /**
     * @Precondition curl_t::has_transfer_time_support()
     * @return transfer time in ms
     */
    std::size_t getinfo_transfer_time() const noexcept;

    ~Easy_t();

    // High-level functions

    /**
     * readall() can be used for get or post.
     *
     * Since curl-cpp is compiled with -fno-exceptions, -fno-rtti by default, if std::string
     * happen to throw std::bad_alloc, then SIGABRT handle will be called.
     *
     * If you need pretty message on exit, you should register callback and print exception using 
     * std::current_exception().
     */
    auto readall(std::string &response) -> perform_ret_t;
    /**
     * read() can be used for get or post.
     * Read in response.capacity() bytes.
     *
     * Since curl-cpp is compiled with -fno-exceptions, -fno-rtti by default, if std::string
     * happen to throw std::bad_alloc, then SIGABRT handle will be called.
     *
     * If you need pretty message on exit, you should register callback and print exception using 
     * std::current_exception().
     */
    auto read(std::string &response) -> perform_ret_t;

    auto establish_connection_only() noexcept -> perform_ret_t;

private:
    auto check_perform(long code) -> perform_ret_t;
    static Easy_t& get_easy(void *curl_easy) noexcept;
};

class Multi_t {
    void *curl_multi;
    int running_handles = 0;

public:
    Data_t data;
    /**
     * perform_callback can call arbitary member functions on easy, but probably
     * not a good idea to call easy.perform().
     */
    void (*perform_callback)(Easy_t &easy, Easy_t::perform_ret_t ret, Data_t &data);

    class Exception: public curl::Exception {
    public:
        const long error_code;

        Exception(long err_code_arg);
        Exception(const Exception&) = default;

        auto what() const noexcept -> const char*;
    };

    Multi_t(void *multi) noexcept;

    /**
     * @param other after mv operation, other is in invalid state and can only be destroyed
     *              or move assign another value.
     */
    Multi_t(const Multi_t&) = delete;
    Multi_t(Multi_t&&) noexcept;

    Multi_t& operator = (const Multi_t&) = delete;
    /**
     * @param other after mv operation, other is in invalid state and can only be destroyed
     *              or move assign another value.
     */
    Multi_t& operator = (Multi_t&&) noexcept;

    /**
     * all member functions mustn't be called during perform()
     */

    /**
     * @param easy must be in valid state
     * @return true if not yet added;
     *         false if already added.
     */
    bool add_easy(Easy_t &easy) noexcept;
    /**
     * Undefined behavior if easy is not valid or not added to this multi.
     */
    void remove_easy(Easy_t &easy) noexcept;

    /**
     * @param timeout Must be >= 0, in ms. Pass 0 for infinite.
     */
    auto poll(curl_waitfd *extra_fds, unsigned extra_nfds, int timeout) noexcept -> 
        Ret_except<int, std::bad_alloc>;

    /**
     * After perform, perform_callback will be called for each completed
     * easy, and then remove_easy would be called on it immediately after callback returns.
     *
     * @return number of running handles
     */
    auto perform() noexcept -> Ret_except<int, std::bad_alloc, Exception, libcurl_bug>;

    ~Multi_t();
};

/**
 * @Precondition for using this class: curl_t::has_CURLU()
 */
class Url {
    void *url;

protected:
    static void check_url(int code);

public:
    friend void Easy_t::set_url(const Url &url) noexcept;

    /**
     * ctor and assignment can throw std::bad_alloc only.
     * noexcept mv ctor and assignment is noexcept.
     */

    Url(Ret_except<void, std::bad_alloc> &e) noexcept;
    Url(const Url&, Ret_except<void, std::bad_alloc> &e) noexcept;
    /**
     * After other is moved, it can only be destroyed or reassigned.
     */
    Url(Url &&other) noexcept;

    auto operator = (const Url&) noexcept -> Ret_except<void, std::bad_alloc>;
    /**
     * After other is moved, it can only be destroyed or reassigned.
     */
    Url& operator = (Url &&other) noexcept;

    enum class set_code {
        ok,
        /**
         * For url, it could be:
         *  - url is longer than 8000000 bytes (7MiB)
         *  - scheme too long (newest libcurl support up to 40 bytes)
         *  - url does not match standard syntax
         *  - lack necessary part, e.g. scheme, host
         *  - contain junk character <= 0x1f || == 0x7f
         */
        malform_input,
        bad_port_number,
        unsupported_scheme,
    };

    auto set_url(const char *url_arg) noexcept -> Ret_except<set_code, std::bad_alloc>;

    /**
     * <scheme>://<user>:<password>@<host>:<port>/<path>;<params>?<query>#<fragment>
     */

    auto set_scheme(const char *scheme) noexcept -> Ret_except<set_code, std::bad_alloc>;
    auto set_options(const char *options) noexcept -> Ret_except<set_code, std::bad_alloc>;
    auto set_query(const char *query) noexcept -> Ret_except<set_code, std::bad_alloc>;

    using string = std::unique_ptr<char, void (*)(void*)>;
    enum class get_code {
        no_scheme, 
        no_user, 
        no_passwd, 
        no_options, 
        no_host, 
        no_port, 
        no_query, 
        no_fragment,
    };
    auto get_url() const noexcept -> Ret_except<string, get_code, std::bad_alloc>;

    auto get_scheme() const noexcept -> Ret_except<string, get_code, std::bad_alloc>;
    auto get_options() const noexcept -> Ret_except<string, get_code, std::bad_alloc>;
    auto get_query() const noexcept -> Ret_except<string, get_code, std::bad_alloc>;

    ~Url();
};

} /* namespace curl */

#endif
