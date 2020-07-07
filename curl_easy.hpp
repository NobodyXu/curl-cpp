#ifndef  __curl_cpp_curl_easy_HPP__
# define __curl_cpp_curl_easy_HPP__

# include <cstddef>
# include <utility>

# include <curl/curl.h>

# include "curl.hpp"

namespace curl {
/**
 * Why make Easy_ref_t RAII-less?
 *
 * It would simplify design of Easy_ref_t, since it doesn't need to
 * implement RAII-part logic.
 *
 * It would also make it easier to use member function of Easy_ref_t in Multi_t,
 * provided that libcurl callback provides CURL* and that option CURLOPT_PRIVATE 
 * which enables storing any object, isn't support until 7.10.3.
 */
class Easy_ref_t {
protected:
    static std::size_t write_callback(char *buffer, std::size_t _, std::size_t size, void *arg) noexcept;

public:
    std::pair<char* /* actually is pointer to CURL */, char*> ptrs;

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
     * If return value is less than @param size, then it will singal an err cond to libcurl.
     * This will cause the transfer to get aborted and the libcurl function used will return CURLE_WRITE_ERROR.
     *
     * It would be undefined behavior to call any easy member function in writeback.
	 * 
     * @param buffer not null-terminated
     */
    using writeback_t = std::size_t (*)(char *buffer, std::size_t _, std::size_t size, void *userp);
    void set_writeback(writeback_t writeback, void *userp) noexcept;

    /**
     * @Precondition curl_t::has_CURLU()
     * @param url content of it must not be changed during call to perform(),
     *            but can be changed once it is finished.
     */
    void set_url(const Url_ref_t &url) noexcept;
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
     * @param len if set to -1, then libcurl would strlen(data) to determine its length.
     */
    void request_post(const void *data, std::uint32_t len) noexcept;
    /**
     * The data pointed to is NOT copied by the library: as a consequence, it must be preserved by 
     * the calling application until the associated transfer finishes. 
     *
     * @Precondition: curl_t::has_protocol("http"))
     * @param len if set to -1, then libcurl would strlen(data) to determine its length.
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
        writeback_error, // Check writeback_exception_thrown
        upload_failure,
        timedout,
    };
    using perform_ret_t = Ret_except<code, std::bad_alloc, std::invalid_argument, std::length_error, 
                                     Exception, Recursive_api_call_Exception, NotBuiltIn_error, 
                                     ProtocolInternal_error>;

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

    // High-level functions

    /**
     * readall() can be used for get or post.
     */
    template <class String = std::string>
    auto readall(String &response) noexcept -> perform_ret_t
    {
        set_writeback([](char *buffer, std::size_t _, std::size_t size, void *ptr) {
            auto &response = *static_cast<String*>(ptr);
            response.append(buffer, buffer + size);
            return size;
        }, &response);

        return perform();
    }

    /**
     * read() can be used for get or post.
     * Read in response.capacity() bytes.
     */
    template <class String = std::string>
    auto read(String &response) noexcept -> perform_ret_t
    {
        set_writeback([](char *buffer, std::size_t _, std::size_t size, void *ptr) {
            auto &response = *static_cast<String*>(ptr);

            auto str_size = response.size();
            auto str_cap = response.capacity();
            if (str_size < str_cap)
                response.append(buffer, buffer + std::min(size, str_cap - str_size));

            return size;
        }, &response);

        return perform();
    }

    auto establish_connection_only() noexcept -> perform_ret_t;

protected:
    auto check_perform(long code, const char *fname) noexcept -> perform_ret_t;
};
} /* namespace curl */

#endif
