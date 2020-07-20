#ifndef  __curl_cpp_curl_easy_HPP__
# define __curl_cpp_curl_easy_HPP__

# include <cstddef>
# include <utility>
# include <string>

# include "curl.hpp"
# include "utils/curl_slist.hpp"

# include <curl/curl.h>

namespace curl {
/**
 * @example curl_easy_get.cc
 *
 * Why make Easy_ref_t RAII-less?
 *
 * It would simplify design of Easy_ref_t, since it doesn't need to
 * implement RAII-part logic.
 *
 * It would also make it easier to use member function of Easy_ref_t in Multi_t,
 * provided that libcurl callback provides CURL* and that option CURLOPT_PRIVATE 
 * which enables storing any object, isn't support until 7.10.3.
 *
 * Easy_ref_t's member function cannot be called in multiple threads simultaneously. 
 *
 * ### PERSISTENT CONNECTIONS
 *
 * Persistent connections means that libcurl can re-use the same connection for several transfers, 
 * if the conditions are right.
 * 
 * libcurl  will  always  attempt  to use persistent connections. It will by default, cache 5 connections.
 *
 * Whenever you use Easy_ref_t::perform or Multi::perform/Multi::socket_action, 
 * libcurl will attempt to use an existing connection to do the transfer, and if none exists it'll
 * open a new one that will be subject for re-use on a possible following call to these functions.
 * 
 * To allow libcurl to take full advantage of persistent connections, you should do 
 * as many of your file transfers as possible using the same handle.
 * 
 * If you use the easy interface, and the Easy_t get destroyed, all the possibly 
 * open connections held by libcurl will be closed and forgotten.
 * 
 * When you've created a multi handle and are using the multi interface, the connection pool is 
 * instead kept in the multi handle so closing and creating new easy handles to do transfers 
 * will not affect them. 
 * Instead all added easy handles can take advantage of the single shared pool.
 *
 * It can also be archieved by using curl::Share_base or curl::Share and 
 * enable_sharing(Share_base::Options::connection_cache).
 */
class Easy_ref_t {
public:
    char *curl_easy = nullptr;

    friend Multi_t;

    /**
     * Base class for any exception thrown via
     * Ret_except in this class -- except for
     * cookie-related function, which can throw
     * curl::NotBuiltIn_error.
     */
    class Exception: public curl::Exception {
    public:
        const long error_code;

        Exception(long err_code_arg);
        Exception(const Exception&) = default;

        auto what() const noexcept -> const char*;
    };
    /**
     * It means that 
     *
     *  > A requested feature, protocol or option was not found built-in in this libcurl 
     *  > due to a build-time decision. 
     *  > This means that a feature or option was not enabled or explicitly disabled 
     *  > when libcurl was built and in order to get it to function you have to get a rebuilt libcurl.
     *
     * From https://curl.haxx.se/libcurl/c/libcurl-errors.html
     */
    class NotBuiltIn_error: public Exception {
    public:
        using Exception::Exception;
    };
    /**
     * Internal error in protocol layer.
     *
     * If you have error buffer set via set_error_buffer,
     * you can check that for an in-detail error message.
     */
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
     * @param buffer either nullptr to disable error buffer,
     *               or at least get_error_buffer_size() big.
     *
     * The error buffer must be kept around until call set_error_buffer again
     * or curl::Easy_t is destroyed.
     *
     * The error buffer is only set if ProtocolInternal_error is thrown.
     */
    void set_error_buffer(char *error_buffer) noexcept;

    /**
     * @pre curl_t::has_private_ptr_support()
     * @param userp any user-defined pointer. Default to nullptr.
     */
    void set_private(void *userp) noexcept;
    /**
     * @pre curl_t::has_private_ptr_support()
     * @return pointer set via set_private or nullptr if not set at all.
     */
    void* get_private() const noexcept;

    /**
     * @param buffer not null-terminated
     * @param size at most CURL_MAX_WRITE_SIZE
     *
     * @ return if less than size, then it will singal an err cond to libcurl.
     *          <br>This will cause the transfer to get aborted and the libcurl function used will return 
     *          code::writeback_error.
     *          <br>If curl_t::has_pause_support() == true, and CURL_WRITEFUNC_PAUSE is returned, 
     *          it will cause transfer to be paused.
     *          See curl_easy_pause for more details.
     *
     * **It would be undefined behavior to call any easy member function in writeback.**
     */
    using writeback_t = std::size_t (*)(char *buffer, std::size_t _, std::size_t size, void *userp);

    /**
     * By default, writeback == std::fwrite, userp == stdout
     */
    void set_writeback(writeback_t writeback, void *userp) noexcept;

    /**
     * @pre curl_t::has_CURLU()
     * @param url content of it must not be changed during call to perform(),
     *            but can be changed once it is finished.
     */
    void set_url(const Url_ref_t &url) noexcept;
    /**
     * @param url would be dupped, thus it can be freed up after this function call.
     */
    auto set_url(const char *url) noexcept -> Ret_except<void, std::bad_alloc>;

    /**
     * @pre url is set to use http(s) && curl_t::has_protocol("http")
     * @param cookies null-terminated string, in format "name1=content1; name2=content2;"
     *                <br>This string will be strdup-ed and override previous call.
     *                <br>This is defaulted to nullptr.
     * @return note that libcurl can be built with cookies disabled, thus this library
     *         can return exception curl::NotBuiltIn_error.
     *
     * This option sets the cookie header explicitly in the outgoing request(s). 
     * <br>If multiple requests are done due to authentication, followed redirections or similar, 
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
     * <br>To set a cookie that is instead held by the cookie engine and can be modified by the 
     * server use set_cookielist.
     *
     * This option will not enable the cookie engine. 
     * Use set_cookiefile or set_cookiejar to enable parsing and sending cookies automatically.
     */
    auto set_cookie(const char *cookies) noexcept -> 
        Ret_except<void, std::bad_alloc, curl::NotBuiltIn_error>;

    /**
     * @pre url is set to use http(s) && curl_t::has_protocol("http")
     * @param cookie_filename null-terminte string for the filename of the cookie file;
     *                        <br>"" to enable cookie engine without any initial cookies;
     *                        <br>"-" to read the cookie from stdin;
     *                        <br>Does not have to keep around after this call.
     *                        <br>This is default to nullptr.
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
     * <br>To make libcurl write cookies to file, see set_cookiejar.
     * 
     * Exercise caution if you are using this option and multiple transfers may occur
     * due to redirect: 
     *
     * If you use the Set-Cookie format: "name1=content1; name2=content2;" and don't specify a domain 
     * then 
     *   the cookie is sent for any domain (even after redirects are followed) 
     *   and cannot be modified by a server-set cookie. 
     *
     *   If a server sets a cookie of the same name then both will be sent 
     *   on a future transfer to that server, likely not what you intended. 
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
     * @pre url is set to use http(s) && curl_t::has_protocol("http")
     * @param cookie_filename null-terminated string;
     *                        <br>"-" write cookies to stdout;
     *                        <br>Does not have to keep around after this call.
     *                        <br>This is default to nullptr.
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
     * <br>If you want to read cookies from a file, use set_cookiefile.
     * 
     * If the cookie jar file can't be created or written to, libcurl will not 
     * and cannot report an error for this.
     * <br>Using set_verbose, set curl::stderr_stream to non-null before creating curl::Easy_t 
     * or CURLOPT_DEBUGFUNCTION will get a warning to display, 
     * but that is the only visible feedback you get about this possibly lethal situation.
     * 
     * Since 7.43.0 cookies that were imported in the Set-Cookie format without 
     * a domain name are not exported by this function.
     */
    auto set_cookiejar(const char *cookie_filename) noexcept -> 
        Ret_except<void, std::bad_alloc, curl::NotBuiltIn_error>;

    /**
     * @pre url is set to use http(s) && curl_t::has_protocol("http")
     * @param cookie_filename null-terminte string for the filename of the cookie file;
     *                        <br>Does not have to keep around after this call.
     *                        <br>This is default to nullptr.
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
     *   the cookie is sent for any domain (even after redirects are followed) 
     *   and cannot be modified by a server-set cookie. 
     *
     *   If a server sets a cookie of the same name then both will be sent 
     *   on a future transfer to that server, likely not what you intended. 
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
     * @pre url is set to use http(s) && curl_t::has_protocol("http")
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
     * @pre url is set to use http(s) && curl_t::has_protocol("http") &&
     *      curl_t::has_erase_all_cookies_in_mem_support().
     * @return note that libcurl can be built with cookies disabled, thus this library
     *         can return exception curl::NotBuiltIn_error.
     */
    auto erase_all_cookies_in_mem() noexcept ->
        Ret_except<void, std::bad_alloc, curl::NotBuiltIn_error>;

    /**
     * @pre url is set to use http(s) && curl_t::has_protocol("http") &&
     *      curl_t::has_erase_all_session_cookies_in_mem_support()
     * @return note that libcurl can be built with cookies disabled, thus this library
     *         can return exception curl::NotBuiltIn_error.
     *
     * Session cookies are cookies without expiry date and they are meant 
     * to be alive and existing for this "session" only.
     *
     * A "session" is usually defined in browser land for as long as 
     * you have your browser up, more or less.
     */
    auto erase_all_session_cookies_in_mem() noexcept ->
        Ret_except<void, std::bad_alloc, curl::NotBuiltIn_error>;

    /**
     * @pre url is set to use http(s) && curl_t::has_protocol("http") &&
     *      curl_t::has_flush_cookies_to_jar()
     * @return note that libcurl can be built with cookies disabled, thus this library
     *         can return exception curl::NotBuiltIn_error.
     *
     * writes all known cookies to the file specified by set_cookiejar.
     */
    auto flush_cookies_to_jar() noexcept ->
        Ret_except<void, std::bad_alloc, curl::NotBuiltIn_error>;

    /**
     * @pre url is set to use http(s) && curl_t::has_protocol("http") &&
     *      curl_t::has_reload_cookies_from_file()
     * @return note that libcurl can be built with cookies disabled, thus this library
     *         can return exception curl::NotBuiltIn_error.
     *
     * loads all cookies from the files specified by set_cookiefile.
     */
     auto reload_cookies_from_file() noexcept ->
        Ret_except<void, std::bad_alloc, curl::NotBuiltIn_error>;

    /**
     * @pre url is set to use http(s) && curl_t::has_protocol("http")
     * @param redir set to 0 to disable redirection.
     *              set to -1 to allow infinite number of redirections.
     *              Other number enables redir number of redirections.
     */
    void set_follow_location(long redir) noexcept;

    /**
     * @pre url is set to use http(s) && curl_t::has_protocol("http")
     * @param useragent pass nullptr for no useragent (default)
     */
    auto set_useragent(const char *useragent) noexcept -> Ret_except<void, std::bad_alloc>;
    /**
     * @pre url is set to use http(s) && curl_t::has_protocol("http")
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
     * @pre curl_t::has_set_ip_addr_only_support()
     * @param ip_addr ipv4/ipv6 address
     *                If it is nullptr, then set to whatever TCP stack find available (default).
     */
    auto set_ip_addr_only(const char *ip_addr) noexcept -> Ret_except<void, std::bad_alloc>;

    /**
     * @param timeout in milliseconds. Set to 0 to disable (default);
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
     * @pre url is set to use http(s)
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
     * @pre url is set to use http(s) && curl_t::has_protocol("http")
     *
     * This is the default for http, and would also set_nobody(false).
     */
    void request_get() noexcept;
    /**
     * @pre url is set to use http(s) && curl_t::has_protocol("http")
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
     *         <br>0 to signal end-of-file to the library and cause it to stop the current transfer.
     *         <br>CURL_READFUNC_ABORT (requires curl_t::has_readfunc_abort_support()) to 
     *         stop immediately, result code::aborted_by_callback.
     *         <br>If curl_t::has_pause_support() == true, and CURL_READFUNC_PAUSE is returned,
     *         it would cause reading from this connection to pause.
     *         See curl_easy_pause for further details.
     *         <br>Bugs: when doing TFTP uploads, you must return the exact amount of data that 
     *         the callback wants, or it will be considered the final packet by the server end and 
     *         the transfer will end there.
     *
     * If you stop the current transfer by returning 0 "pre-maturely" 
     * (i.e before the server expected it, like when you've said you will 
     * upload N bytes and you upload less than N bytes), you may experience 
     * that the server "hangs" waiting for the rest of the data that won't come.
     */
    using readback_t = std::size_t (*)(char *buffer, std::size_t size, std::size_t nitems, void *userp);

    /**
     * @pre url is set to use http(s) && curl_t::has_protocol("http")
     * @param len optional. Set to -1 means length of data is not known ahead of time.
     */
    void request_post(readback_t readback, void *userp, std::size_t len = -1) noexcept;

    /**
     * @pre curl_t::has_protocol(protocol you use in url)
     * @exception NotSupported_error, std::bad_alloc or any exception defined in this class
     */
    enum class code {
        ok = 0,
        unsupported_protocol,
        url_malformat,
        cannot_resolve_proxy,
        cannot_resolve_host,
        cannot_connect, // Cannot connect to host or proxy
        remote_access_denied,
        writeback_error,
        upload_failure, // Failed starting the upload
        timedout,
        aborted_by_callback, // If readback return CURL_READFUNC_ABORT.
        too_many_redirects, 
    };
    using perform_ret_t = Ret_except<code, std::bad_alloc, std::invalid_argument, std::length_error, 
                                     Exception, Recursive_api_call_Exception, NotBuiltIn_error, 
                                     ProtocolInternal_error>;

    auto perform() noexcept -> perform_ret_t;

    enum class PauseOptions {
        /**
         * Pause receiving data. 
         * <br>There will be no data received on this connection until this function is called again without this bit set. 
         * <br>Thus, the writeback won't be called.
         */
        recv = 1 << 0,
        /**
         * Pause sending data. 
         * <br>There will be no data sent on this connection until this function is called again without this bit set. 
         * <br>Thus, the readback won't be called/the data registered with request_post won't be copied.
         */
        send = 1 << 2, // Make value of pause_send the same as stock libcurl
        /**
         * Convenience define that pauses both directions.
         */
        all = recv | send, 
        /**
         * Convenience define that unpauses both directions.
         */
        cont = 0,
    };

    /**
     * @pre curl_t::has_pause_support() and there's an ongoing transfer
     * @return If no exception is thrown, then it is either code::ok or code::writeback_error.
     *
     * **The pausing of transfers does not work with protocols that work without network connectivity, like FILE://.
     * Trying to pause such a transfer, in any direction, will cause problems in the worst case or an error in the best case.**
     *
     * ### Use of set_pause with multi_socket_action interface
     *
     * Before libcurl 7.32.0, when a specific handle was unpaused with this function, there was no particular forced rechecking 
     * or similar of the socket's state, which made the continuation of the transfer get delayed until next 
     * multi-socket call invoke or even longer. 
     * Alternatively, the user could forcibly call for example curl_multi_socket_all - with a rather hefty performance penalty.
     * 
     * Starting in libcurl 7.32.0, unpausing a transfer will schedule a timeout trigger for that handle 
     * 1 millisecond into the future, so that a curl_multi_socket_action( ... CURL_SOCKET_TIMEOUT) can be used 
     * immediately afterwards to get the transfer going again as desired.
     * 
     * If you use multi interface, you can use multi_socket_action to have a more in-detail
     * control of pausing the easy.
     *
     * ### MEMORY USE
     *
     * When pausing a read by returning the magic return code from a write callback, the read data 
     * is already in libcurl's internal buffers so it'll have to keep it in an allocated buffer 
     * until the reading is again unpaused using this function.
     *
     * If the downloaded data is compressed and is asked to get uncompressed automatically on download, 
     * libcurl will continue to uncompress the entire downloaded chunk and it will cache the data uncompressed. 
     * This has the side-effect that if you download something that is compressed a lot, it can result in a 
     * very large amount of data required to be allocated to be kept around during the pause. 
     *
     * This said, you should probably consider not using paused reading if you allow libcurl to uncompress data automatically.
     */
    auto set_pause(PauseOptions option) noexcept -> Ret_except<code, std::bad_alloc, Exception>;

    long get_response_code() const noexcept;

    /**
     * @pre curl_t::has_sizeof_upload_support()
     * @return in bytes
     */
    std::size_t getinfo_sizeof_uploaded() const noexcept;
    /**
     * @pre curl_t::has_sizeof_response_header_support()
     * @return in bytes
     */
    std::size_t getinfo_sizeof_response_header() const noexcept;
    /**
     * @pre curl_t::has_sizeof_response_body_support()
     * @return in bytes
     */
    std::size_t getinfo_sizeof_response_body() const noexcept;

    /**
     * @pre curl_t::has_transfer_time_support()
     * @return transfer time in ms
     * 
     * What transfer time really measures:
     *
     *     |
     *     |--NAMELOOKUP
     *     |--|--CONNECT
     *     |--|--|--APPCONNECT
     *     |--|--|--|--PRETRANSFER
     *     |--|--|--|--|--STARTTRANSFER
     *     |--|--|--|--|--|--transfer time
     *     |--|--|--|--|--|--REDIRECT
     */
    std::size_t getinfo_transfer_time() const noexcept;

    /**
     * @pre curl_t::has_redirect_url_support() && 
     *      url is set to use http(s) && curl_t::has_protocol("http")
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
     * @pre url is set to use http(s) && curl_t::has_protocol("http") &&
     *      curl_t::has_getinfo_cookie_list_support()
     *
     * @return note that libcurl can be built with cookies disabled, thus this library
     *         can return exception curl::NotBuiltIn_error.
     *         If utils::slist is empty, it might be due to std::bad_alloc, or
     *         no cookie is present(cookie engine not enabled or no cookie has
     *         received).
     *
     * Since 7.43.0 cookies that were imported in the Set-Cookie format 
     * without a domain name are not exported by this option.
     */
    auto getinfo_cookie_list() const noexcept ->
        Ret_except<utils::slist, curl::NotBuiltIn_error>;

    /**
     * @pre curl_t::has_get_active_socket_support()
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
     *     int main(int argc, char* argv[]) {
     *         curl::curl_t curl{nullptr};
     *
     *         auto easy = curl.create_easy();
     *         assert(easy.p1 && easy.p2);
     *
     *         auto easy_ref = curl::Easy_ref_t{easy};
     *         easy_ref.set_url("https://www.google.com");
     *
     *         setup_establish_connection_only();
     *         easy_ref.perform(); // Establish the connection
     *
     *         request_get();
     *         easy_ref.perform(); // Now result is writen to stdout.
     *     }
     */
    void setup_establish_connection_only() noexcept;

protected:
    static auto check_perform(long code, const char *fname) noexcept -> perform_ret_t;
};
} /* namespace curl */

#endif
