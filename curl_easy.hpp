#ifndef  __curl_cpp_curl_easy_HPP__
# define __curl_cpp_curl_easy_HPP__

# include <cstddef>
# include <utility>
# include <string>

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
 *
 * PERSISTENT CONNECTIONS
 *     Persistent connections means that libcurl can re-use the same connection for several transfers, 
 *     if the conditions are right.
 * 
 *     libcurl  will  always  attempt  to use persistent connections. 
 *     Whenever you use curl_easy_perform or curl_multi_perform/curl_multi_socket_action etc, 
 *     libcurl will attempt to use an existing connection to do the transfer, and if none exists it'll
 *     open a new one that will be subject for re-use on a possible following call curl_easy_perform or 
 *     curl_multi_perform/curl_multi_socket_action.
 * 
 *     To allow libcurl to take full advantage of persistent connections, you should do 
 *     as many of your file transfers as possible using the same handle.
 * 
 *     If you use the easy interface, and you call curl_easy_cleanup(3), all the possibly 
 *     open connections held by libcurl will be closed and forgotten.
 * 
 *     When you've created a multi handle and are using the multi interface, the connection pool is 
 *     instead kept in the multi handle so closing and creating new easy handles to do transfers 
 *     will not affect them. 
 *     Instead all added easy handles can take advantage of the single shared pool.
 */
class Easy_ref_t {
public:
    char *curl_easy = nullptr;

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
    public:
        using Exception::Exception;
    };

    /**
     * @param stderr_stream_arg if not null, enable verbose mode and
     *                          print them onto stderr_stream_arg.
     */
    void set_verbose(FILE *stderr_stream_arg) noexcept;

    /**
     * Minimum length for error buffer.
     */
    static std::size_t get_error_buffer_size() noexcept;

    /**
     * @para buffer either nullptr to disable error buffer,
     *              or at least get_error_buffer_size() big.
     *
     * The error buffer must be kept around until call set_error_buffer again
     * or curl::Easy_t is destroyed.
     *
     * The error buffer is only set if ProtocolInternal_error is thrown.
     */
    void set_error_buffer(char *error_buffer) noexcept;

    /**
     * @Precondition
     */
    void set_private(void *userp) noexcept;
    void* get_private() const noexcept;

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
     * @Precondition url is set to use http(s) && curl_t::has_protocol("http")
     * @param cookies null-terminated string, in format "name1=content1; name2=content2;"
     *                This string will be strdup-ed and override previous call.
     *                This is defaulted to nullptr.
     * @return note that libcurl can be built with cookies disabled, thus this library
     *         can return exception curl::NotBuiltIn_error.
     *
     * This option sets the cookie header explicitly in the outgoing request(s). 
     * If multiple requests are done due to authentication, followed redirections or similar, 
     * they will all get this cookie passed on.
     *
     * The cookies set by this option are separate from the internal cookie storage held 
     * by the cookie engine and will not be modified by it. 
     *
     * If you enable the cookie engine and either you've imported a cookie of the same name 
     * (e.g. 'foo') or the server has set one, it will have no effect on the cookies you set here. 
     *
     * A request to the server will send both the 'foo' held by the cookie engine and 
     * the 'foo' held by this option. 
     * To set a cookie that is instead held by the cookie engine and can be modified by the 
     * server use set_cookielist.
     *
     * This option will not enable the cookie engine. 
     * Use set_cookiefile or set_cookiejar to enable parsing and sending cookies automatically.
     */
    auto set_cookie(const char *cookies) noexcept -> 
        Ret_except<void, std::bad_alloc, curl::NotBuiltIn_error>;

    /**
     * @Precondition url is set to use http(s) && curl_t::has_protocol("http")
     * @param cookie_filename null-terminte string for the filename of the cookie file;
     *                        "" to enable cookie engine without any initial cookies;
     *                        "-" to read the cookie from stdin;
     *                        Does not have to keep around after this call.
     *                        This is default to nullptr.
     * @return note that libcurl can be built with cookies disabled, thus this library
     *         can return exception curl::NotBuiltIn_error.
     *
     * The cookie data can be in either the old Netscape / Mozilla cookie data format or 
     * just regular HTTP headers (Set-Cookie style) dumped to a file.
     *
     * It enables the cookie engine, making libcurl parse response and send cookies on 
     * subsequent requests with this handle.
     *
     * This function can be used with set_cookielist.
     * 
     * It only reads cookies right before a transfer is started. 
     * To make libcurl write cookies to file, see set_cookiejar.
     * 
     * Exercise caution if you are using this option and multiple transfers may occur
     * due to redirect: 
     *
     * If you use the Set-Cookie format: "name1=content1; name2=content2;" and don't specify a domain 
     * then 
     *     the cookie is sent for any domain (even after redirects are followed) 
     *     and cannot be modified by a server-set cookie. 
     *
     *     If a server sets a cookie of the same name then both will be sent 
     *     on a future transfer to that server, likely not what you intended. 
     * To address these issues set a domain in Set-Cookie HTTP header
     * (doing that will include sub-domains) or use the Netscape format:
     *
     *     char *my_cookie =
     *       "example.com"    // Hostname
     *       "\t" "FALSE"     // Include subdomains
     *       "\t" "/"         // Path
     *       "\t" "FALSE"     // Secure
     *       "\t" "0"         // Expiry in epoch time format. 0 == Session
     *       "\t" "foo"       // Name
     *       "\t" "bar";      // Value
     * 
     * If you call this function multiple times, you just add more files to read. 
     * Subsequent files will add more cookies.
     */
    auto set_cookiefile(const char *cookie_filename) noexcept -> 
        Ret_except<void, curl::NotBuiltIn_error>;

    /**
     * @Precondition url is set to use http(s) && curl_t::has_protocol("http")
     * @param cookie_filename null-terminated string;
     *                        "-" write cookies to stdout;
     *                        Does not have to keep around after this call.
     *                        This is default to nullptr.
     * @return note that libcurl can be built with cookies disabled, thus this library
     *         can return exception curl::NotBuiltIn_error.
     *
     * This will make libcurl write all internally known cookies to 
     * the specified file when curl::Easy_t is destroyed. 
     *
     * If no cookies, then no file will be created. 
     *
     * Using this option also enables cookies for this session, so if you 
     * for example follow a location it will make matching cookies get sent accordingly.
     * 
     * Note that libcurl doesn't read any cookies from the cookie jar. 
     * If you want to read cookies from a file, use set_cookiefile.
     * 
     * If the cookie jar file can't be created or written to, libcurl will not 
     * and cannot report an error for this.
     * Using set_verbose, set curl::stderr_stream to non-null before creating curl::Easy_t 
     * or CURLOPT_DEBUGFUNCTION will get a warning to display, 
     * but that is the only visible feedback you get about this possibly lethal situation.
     * 
     * Since 7.43.0 cookies that were imported in the Set-Cookie format without 
     * a domain name are not exported by this function.
     */
    auto set_cookiejar(const char *cookie_filename) noexcept -> 
        Ret_except<void, std::bad_alloc, curl::NotBuiltIn_error>;

    /**
     * @Precondition url is set to use http(s) && curl_t::has_protocol("http")
     * @param cookie_filename null-terminte string for the filename of the cookie file;
     *                        Does not have to keep around after this call.
     *                        This is default to nullptr.
     * @return note that libcurl can be built with cookies disabled, thus this library
     *         can return exception curl::NotBuiltIn_error.
     *
     * The cookie will be immediately loaded, and this function can be mixed with
     * set_cookiefile.
     *
     * Such a cookie can be either a single line in Netscape / Mozilla format or 
     * just regular HTTP-style header (Set-Cookie: ...) format. 
     *
     * This will also enable the cookie engine and adds that single cookie 
     * to the internal cookie store.
     *
     * Exercise caution if you are using this option and multiple transfers may occur
     * due to redirect: 
     *
     * If you use the Set-Cookie format: "name1=content1; name2=content2;" and don't specify a domain 
     * then 
     *     the cookie is sent for any domain (even after redirects are followed) 
     *     and cannot be modified by a server-set cookie. 
     *
     *     If a server sets a cookie of the same name then both will be sent 
     *     on a future transfer to that server, likely not what you intended. 
     * To address these issues set a domain in Set-Cookie HTTP header
     * (doing that will include sub-domains) or use the Netscape format:
     *
     *     char *my_cookie =
     *       "example.com"    // Hostname
     *       "\t" "FALSE"     // Include subdomains
     *       "\t" "/"         // Path
     *       "\t" "FALSE"     // Secure
     *       "\t" "0"         // Expiry in epoch time format. 0 == Session
     *       "\t" "foo"       // Name
     *       "\t" "bar";      // Value
     * 
     */
    auto set_cookielist(const char *cookie) noexcept -> 
        Ret_except<void, std::bad_alloc, curl::NotBuiltIn_error>;

    /**
     * @Precondition url is set to use http(s) && curl_t::has_protocol("http")
     *
     * It will force libcurl to ignore all cookies it is about to load that 
     * are "session cookies" from the previous session. 
     *
     * Session cookies are cookies without expiry date and they are meant 
     * to be alive and existing for this "session" only.
     *
     * A "session" is usually defined in browser land for as long as 
     * you have your browser up, more or less.
     *
     * By default, libcurl always stores and loads all cookies, independent 
     * if they are session cookies or not. 
     *
     * NOTE that cookie support can be removed in compile time of libcurl, there is
     * no guarantee this would work.
     */
    void start_new_cookie_session() noexcept;

    /**
     * @Precondition url is set to use http(s) && curl_t::has_protocol("http")
     * @param redir set to 0 to disable redirection.
     *              set to -1 to allow infinite number of redirections.
     *              Other number enables redir number of redirections.
     */
    void set_follow_location(long redir) noexcept;

    /**
     * @Precondition url is set to use http(s) && curl_t::has_protocol("http")
     * @param useragent pass nullptr for no useragent (default)
     */
    auto set_useragent(const char *useragent) noexcept -> Ret_except<void, std::bad_alloc>;
    /**
     * @Precondition url is set to use http(s) && curl_t::has_protocol("http")
     * @param encoding "" for enable all (default);
     *                 nullptr for disable all (including auto decompression).
     */
    auto set_encoding(const char *encoding) noexcept -> Ret_except<void, std::bad_alloc>;

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
     * @Precondition url is set to use http(s)
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
     * @param enable if true, then it would not request body data to be transfered;
     *               if false, then a normal request (default).
     */
    void set_nobody(bool enable) noexcept;

    /**
     * @Precondition url is set to use http(s) && curl_t::has_protocol("http")
     *
     * This is the default for http, and would also set_nobody(false).
     */
    void request_get() noexcept;
    /**
     * @Precondition url is set to use http(s) && curl_t::has_protocol("http")
     * @param len if set to -1, then libcurl would strlen(data) to determine its length.
     *
     * The data pointed to is NOT copied by the library: as a consequence, it must be preserved by 
     * the calling application until the associated transfer finishes. 
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
     * @Precondition url is set to use http(s) && curl_t::has_protocol("http")
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
     * @Precondition curl_t::has_redirect_url_support() && 
     *               url is set to use http(s) && curl_t::has_protocol("http")
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
    auto set_readall_writeback(String &response) noexcept
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
    auto set_read_writeback(std::pair<String, size_type> arg) noexcept
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

    /**
     * After this call, Easy_ref_t::perform/Multi_t::perform or multi_socket_action must be 
     * called to establish the connection.
     *
     * To use the established connection, call set_nobody(false) or request_*()
     * to disable nobody.
     *
     * Example usage:
     *
     * int main(int argc, char* argv[]) {
     *     curl::curl_t curl{nullptr};
     *
     *     auto easy = curl.create_easy();
     *     assert(easy.p1 && easy.p2);
     *
     *     auto easy_ref = curl::Easy_ref_t{easy};
     *     easy_ref.set_url("https://www.google.com");
     *
     *     setup_establish_connection_only();
     *     easy_ref.perform(); // Establish the connection
     *
     *     request_get();
     *     easy_ref.perform(); // Now result is writen to stdout.
     * }
     */
    void setup_establish_connection_only() noexcept;

protected:
    static auto check_perform(long code, const char *fname) noexcept -> perform_ret_t;
};
} /* namespace curl */

#endif
