#ifndef  __curl_cpp_curl_HPP__
# define __curl_cpp_curl_HPP__

# include <cstddef>
# include <cstdint>
# include <cstdio>
# include <stdexcept>
# include <memory>
# include <string>

# include "return-exception/ret-exception.hpp"

namespace curl {
class Exception: public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
class NotSupported_error: public Exception {
public:
    using Exception::Exception;
};

class handle_t;
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
    };

    FILE *debug_stream;
    const Version version;

    /**
     * Since this variable is designed to be static, 
     * it would call errx on error.
     */
    curl_t(FILE *debug_stream_arg) noexcept;

    curl_t(const curl_t&) = delete;
    curl_t(curl_t&&) = delete;

    curl_t& operator = (const curl_t&) = delete;
    curl_t& operator = (curl_t&&) = delete;

    ~curl_t();

    bool has_compression_support() const noexcept;
    bool has_largefile_support() const noexcept;
    bool has_protocol(const char *protocol) const noexcept;

    auto create_handle() noexcept -> Ret_except<handle_t, curl::Exception>;
};

class handle_t {
public:
    /**
     * If return value is less than @param size, then it will singal an err cond to libcurl.
     * This will cause the transfer to get aborted and the libcurl function used will return CURLE_WRITE_ERROR.
	 * 
     * @param buffer not null-terminated
     */
    using writeback_t = std::size_t (*)(char *buffer, std::size_t size, void *data);

private:
    void *curl_easy;

public:
    /**
     * If set to nullptr, then all response content are ignored.
     */
    writeback_t writeback = nullptr;
    void *data = nullptr;

protected:
    static void check_easy(int code, const char *expr);

public:
    class Exception: public curl::Exception {
    public:
        const long error_code;

        Exception(long err_code_arg);
        Exception(const Exception&) = default;

        auto what() const noexcept -> const char*;
    };
    class CannotResolve_error: public Exception {
    public:
        using Exception::Exception;
    };
    class ConnnectionFailed_error: public Exception {
    public:
        using Exception::Exception;
    };
    class ProtocolError: public Exception {
    public:
        const long response_code;

        ProtocolError(long err_code_arg, long response_code_arg);
        ProtocolError(const ProtocolError&) = default;
    };
    class Timeout_error: public Exception {
    public:
        using Exception::Exception;
    };

    /**
     * @param curl must be ret value of curl_easy_init(), must not be nullptr
     */
    handle_t(void *curl) noexcept;

    handle_t(const handle_t&, Ret_except<void, curl::Exception> &e) noexcept;
    /**
     * @param other after mv operation, other is in invalid state and can only be destroyed.
     */
    handle_t(handle_t &&other) noexcept;

    handle_t& operator = (const handle_t&) = delete;
    handle_t& operator = (handle_t&&) = delete;

    /**
     * @param url content of it must not be changed during call to perform(),
     *            but can be changed once it is finished.
     * @param encoding "" for enable all, NULL for disable all (including auto decompression).
     * @exception std::bad_alloc or NotSupported_error
     */
    auto set(const Url &url, const char *useragent, const char *encoding) noexcept -> 
        Ret_except<void, NotSupported_error, std::bad_alloc>;

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
     * @exception NotSupported_error, std::bad_alloc or any exception defined in this class
     */
    void perform();

    auto get_response_code() const noexcept -> Ret_except<long, NotSupported_error>;

    /**
     * @return in bytes
     */
    auto getinfo_sizeof_uploaded() const noexcept -> Ret_except<std::size_t, NotSupported_error>;
    auto getinfo_sizeof_response_header() const noexcept -> Ret_except<std::size_t, NotSupported_error>;
    auto getinfo_sizeof_response_body() const noexcept -> Ret_except<std::size_t, NotSupported_error>;

    /**
     * @return transfer time in ms
     * @exception NotSupported_error
     */
    auto getinfo_transfer_time() const noexcept -> Ret_except<std::size_t, NotSupported_error>;

    ~handle_t();

    // High-level functions

    /**
     * readall() and read() can be used for get or post.
     * They would return the response content as std::string.
     *
     * readall() would return all while read would only return
     * first bytes byte.
     *
     * Can throw any exception that perform would throw.
     */
    std::string readall();
    std::string read(std::size_t bytes);

    /**
     * @exception any exception request_get() and perform() can throw.
     */
    void establish_connection_only();
};

class Url {
    void *url;

protected:
    static void check_url(int code);

public:
    friend auto handle_t::set(const Url &url, const char *useragent, const char *encoding) noexcept ->
        Ret_except<void, NotSupported_error, std::bad_alloc>;

    class Exception: public curl::Exception {
    public:
        const int error_code;

        Exception(int err_code_arg);
        Exception(const Exception&) = default;

        auto what() const noexcept -> const char*;
    };

    /**
     * ctor and assignment can throw std::bad_alloc only.
     * noexcept mv ctor and assignment is noexcept.
     */

    Url();
    Url(const char *url_arg);
    Url(const Url&);
    /**
     * After other is moved, it can only be destroyed or reassigned.
     */
    Url(Url &&other) noexcept;

    Url& operator = (const Url&);
    /**
     * After other is moved, it can only be destroyed or reassigned.
     */
    Url& operator = (Url &&other) noexcept;

    void set_url(const char *url_arg);

    /**
     * <scheme>://<user>:<password>@<host>:<port>/<path>;<params>?<query>#<fragment>
     */
    void set_scheme(const char *scheme);
    void set_options(const char *options);
    void set_query(const char *query);

    using fullurl_str = std::unique_ptr<char, void (*)(void*)>;
    auto get_url() const -> fullurl_str;

    ~Url();
};

} /* namespace curl */

#endif
