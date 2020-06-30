#ifndef  __curl_cpp_curl_HPP__
# define __curl_cpp_curl_HPP__

# include <cstddef>
# include <cstdio>
# include <stdexcept>
# include <memory>
# include <string>

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
    FILE *debug_stream;

public:
    curl_t(FILE *debug_stream_arg);

    curl_t(const curl_t&) = delete;
    curl_t(curl_t&&) = delete;

    curl_t& operator = (const curl_t&) = delete;
    curl_t& operator = (curl_t&&) = delete;

    ~curl_t();

    bool has_compression_support() const noexcept;
    bool has_http2_support() const noexcept;

    /**
     * @exception curl::NotSupported_error.
     */
    handle_t create_handle();
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
    handle_t(void *p);

public:
    friend handle_t curl_t::create_handle();

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
     * After calling copy constructor, you must call request_* again before calling perform.
     */
    handle_t(const handle_t&);
    handle_t(handle_t &&other) = delete;

    handle_t& operator = (const handle_t&) = delete;
    handle_t& operator = (handle_t&&) = delete;

    /**
     * @param url must exist until perform() returns.
     * @param encoding "" for enable all, NULL for disable all (including auto decompression).
     */
    void set(const Url &url, const char *useragent, const char *encoding);

    void request_get();
    /**
     * The data pointed to is NOT copied by the library: as a consequence, it must be preserved by 
     * the calling application until the associated transfer finishes. 
     */
    void request_post(const void *data, std::size_t len);

    void perform();

    long get_response_code() const;

    std::size_t getinfo_sizeof_uploaded() const;
    std::size_t getinfo_sizeof_response_header() const;
    std::size_t getinfo_sizeof_response_body() const;

    ~handle_t();

    // High-level functions

    /**
     * readall() and read() can be used for get or post.
     * They would return the response content as std::string.
     *
     * readall() would return all while read would only return
     * first bytes byte.
     */
    std::string readall();
    std::string read(std::size_t bytes);

    void establish_connection_only();
};

class Url {
    void *url;

public:
    friend void handle_t::set(const Url &url, const char *useragent, const char *encoding);

    class Exception: public curl::Exception {
    public:
        const int error_code;

        Exception(int err_code_arg);
        Exception(const Exception&) = default;

        auto what() const noexcept -> const char*;
    };

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
    void set_scheme(const char *scheme);
    void set_options(const char *options);

    using fullurl_str = std::unique_ptr<char, void (*)(void*)>;
    auto get_url() const -> fullurl_str;

    ~Url();
};

} /* namespace curl */

#endif
