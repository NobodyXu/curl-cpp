#ifndef  __curl_cpp_curl_easy_HPP__
# define __curl_cpp_curl_easy_HPP__

# include <cstddef>
# include <utility>
# include <string>

# include <curl/curl.h>

# include "curl.hpp"
# include "utils/curl_slist.hpp"

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
public:
    char *curl_easy = nullptr;
    char *error_buffer = nullptr;

    Easy_ref_t(const curl_t::Easy_t &e) noexcept;

    Easy_ref_t() = default;

    Easy_ref_t(const Easy_ref_t&) = default;
    Easy_ref_t(Easy_ref_t&&) = default;

    Easy_ref_t& operator = (const Easy_ref_t&) = default;
    Easy_ref_t& operator = (Easy_ref_t&&) = default;

    ~Easy_ref_t() = default;

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
     * @param stderr_stream_arg if not null, enable verbose mode and
     *                          print them onto stderr_stream_arg.
     */
    void set_verbose(FILE *stderr_stream_arg) noexcept;

    /**
     * If return value is less than @param size, then it will singal an err cond to libcurl.
     * This will cause the transfer to get aborted and the libcurl function used will return CURLE_WRITE_ERROR.
     *
     * It would be undefined behavior to call any easy member function in writeback.
	 * 
     * @param buffer not null-terminated
     */
    using writeback_t = std::size_t (*)(char *buffer, std::size_t _, std::size_t size, void *userp);

    /**
     * By default, writeback == std::fwrite, userp == stdout
     */
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
     * @Precondition curl_t::has_protocol("http")
     * @param redir set to 0 to disable redirection.
     *              set to -1 to allow infinite number of redirections.
     *              Other number enables redir number of redirections.
     */
    void set_follow_location(long redir) noexcept;

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
     * @param enable if true, then it would not request body data to be transfered;
     *               if false, then a normal request (default).
     */
    void set_nobody(bool enable) noexcept;

    /**
     * @param value can be ipv4 or ipv6 address/hostname/interface.
     *              If it is nullptr, then set to whatever TCP stack find available (default).
     */
    auto set_interface(const char *value) noexcept -> Ret_except<void, std::bad_alloc>;

    /**
     * @Precondition curl_t::has_set_ip_addr_only_support()
     * @param ip_addr ipv4/ipv6 address
     */
    auto set_ip_addr_only(const char *ip_addr) noexcept -> Ret_except<void, std::bad_alloc>;

    /**
     * @param timeout in milliseconds. Set to 0 to disable;
     *                should be less than std::numeric_limits<long>::max().
     */
    void set_timeout(unsigned long timeout) noexcept;

    enum class header_option {
        /**
         * If unspecified is passed to set_http_header, then the
         * previous header_option (or default) will be used.
         *
         * If curl_t::has_header_option_support() == false, then
         * it is default to unified.
         *
         * Before 7.42.1, default is unified.
         * After 7.42.1, default is separate.
         */
        unspecified,
        /**
         * The following options only take effect when 
         * curl_t::has_header_option_support() == true.
         */
        unified, // header specified with set_http_header will also be used for proxy
        separate, // reverse of unified
    };
    /**
     * @Precondition curl_t::has_protocol("http")
     * @param l will not be copied, thus it is required to be kept
     *          around until another set_http_header is issued or 
     *          this Easy_t is destroyed.
     *
     *          Must not be CRLF-terminated.
     * @param option control whether header set here will also sent to
     *               proxy
     *
     * Example:
     *  - Replace hedaer 'Accept:'
     *    
     *      utils::slist l;
     *      l.push_back("Accept: deflate");
     *      easy.set_http_header(l);
     *
     *  - Remove header 'Accept:'
     *      
     *      easy.set_http_header(utils::slist{});
     *
     * Starting in 7.58.0, libcurl will specifically prevent 
     * "Authorization:" headers from being sent to hosts other 
     * than the first used one, unless specifically permitted 
     * with the CURLOPT_UNRESTRICTED_AUTH option.
     * 
     * Starting in 7.64.0, libcurl will specifically prevent 
     * "Cookie:" headers from being sent to hosts other than 
     * the first used one, unless specifically permitted with 
     * the CURLOPT_UNRESTRICTED_AUTH option.
     */
    void set_http_header(const utils::slist &l, header_option option = header_option::unspecified) noexcept;

    /**
     * @Precondition: curl_t::has_protocol("http")
     * This is the default for http.
     */
    void request_get() noexcept;
    /**
     * The data pointed to is NOT copied by the library: as a consequence, it must be preserved by 
     * the calling application until the associated transfer finishes. 
     *
     * @Precondition: curl_t::has_protocol("http"))
     * @param len if set to -1, then libcurl would strlen(data) to determine its length.
     */
    void request_post(const void *data, std::size_t len) noexcept;

    /**
     * The length of buffer is size * nitems.
     *
     * @return bytes writen to the buffer.
     *               0 to signal end-of-file to the library and cause it to stop the current transfer.
     *               CURL_READFUNC_ABORT (requires curl_t::has_readfunc_abort_support()) to 
     *               stop immediately, result code::aborted_by_callback.
     *
     * If you stop the current transfer by returning 0 "pre-maturely" 
     * (i.e before the server expected it, like when you've said you will 
     * upload N bytes and you upload less than N bytes), you may experience 
     * that the server "hangs" waiting for the rest of the data that won't come.
     */
    using readback_t = std::size_t (*)(char *buffer, std::size_t size, std::size_t nitems, void *userp);

    /**
     * @param len optional. Set to -1 means length of data is not known ahead of time.
     */
    void request_post(readback_t readback, void *userp, std::size_t len = -1) noexcept;

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
        upload_failure, // Failed starting the upload
        timedout,
        aborted_by_callback, // If readback return CURL_READFUNC_ABORT.
        too_many_redirects, 
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

    /**
     * @Precondition curl_t::has_redirect_url_support()
     * @return null-terminated string, freeing not required.
     *
     * If you disable redirection or CURLOPT_MAXREDIRS limit 
     * prevented a redirect to happen (since 7.54.1), 
     * a would-be redirect-to url is returned.
     *
     * This function is only meaningful for http(s) protocol.
     */
    auto getinfo_redirect_url() const noexcept -> const char*;

    /**
     * @Precondition curl_t::has_get_active_socket_support()
     * @return CURL_SOCKET_BAD if no valid socket or not supported
     *
     * The return value can be used in Multi_t::multi_assign()
     */
    auto get_active_socket() const noexcept -> curl_socket_t;

    // High-level functions

    /**
     * set_readall_callback() can be used for get or post.
     */
    template <class String = std::string>
    auto set_readall_callback(String &response) noexcept
    {
        set_writeback([](char *buffer, std::size_t _, std::size_t size, void *ptr) {
            auto &response = *static_cast<String*>(ptr);
            response.append(buffer, buffer + size);
            return size;
        }, &response);
    }

    /**
     * set_read_callback() can be used for get or post.
     * @param len read in len bytes
     */
    template <class String = std::string, class size_type = typename String::size_type>
    auto set_read_callback(std::pair<String, size_type> arg) noexcept
    {
        set_writeback([](char *buffer, std::size_t _, std::size_t size, void *ptr) {
            auto &args = *static_cast<std::pair<String, size_type>*>(ptr);
            auto &response = args.first;
            auto &requested_len = args.second;

            auto str_size = response.size();
            if (str_size < requested_len)
                response.append(buffer, buffer + std::min(size, requested_len - str_size));

            return size;
        }, &arg);
    }

    auto establish_connection_only() noexcept -> perform_ret_t;

protected:
    auto check_perform(long code, const char *fname) noexcept -> perform_ret_t;
};
} /* namespace curl */

#endif
