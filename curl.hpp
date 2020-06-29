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
    ~curl_t();

    bool has_compression_support() const noexcept;
    bool has_http2_support() const noexcept;

    /**
     * @param encoding "" for enable all, NULL for disable all (including auto decompression).
     * @param url must exist until perform() returns.
     * @exception curl::Exception and curl::handle_t::Exception.
     */
    handle_t create_conn(const Url &url, const char *useragent = NULL, const char *encoding = "");
};

class handle_t {
public:
    /**
     * If return value is less than @param size, then it will singal an err cond to libcurl.
     * This will cause the transfer to get aborted and the libcurl function used will return CURLE_WRITE_ERROR.
	 * 
     * @param buffer not null-terminated
     */
    using download_callback_t = std::size_t (*)(char *buffer, std::size_t size, void *data);

private:
    void *curl_easy;

    download_callback_t download_callback;
    void *data;

protected:
    handle_t(void *p);

public:
    friend handle_t curl_t::create_conn(const Url &url, const char *useragent, const char *encoding);

    class Exception: public curl::Exception {
    public:
        const int error_code;

        Exception(int err_code_arg);
        Exception(const Exception&) = default;
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
     */
    void set(const Url &url, const char *useragent, const char *encoding);

    void request_get(download_callback_t callback, void *data);
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
    std::string readall();
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
