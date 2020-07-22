#ifndef  __curl_cpp_curl_multi_HPP__
# define __curl_cpp_curl_multi_HPP__

# include "curl_easy.hpp"
# include <curl/curl.h>

namespace curl {
/**
 * @example curl_multi_poll.cc
 * @example curl_multi_poll2.cc
 * @example curl_multi_socket_action_epoll.cc
 * @example curl_multi_socket_action_event.cc
 * @example curl_multi_socket_action_uv.cc
 *
 * Multi_t enables user to simultaneously do multiple request
 * in the same thread, using any polling interface they prefer.
 *
 * This class contains poll() interface (wrapping curl_multi_poll, doesn't
 * add additional space to use it), and multi_socket_action interface 
 * that enables using any interface. They are incompatible and should not be 
 * mixed.
 *
 * Multi_t's member function cannot be called in multiple threads simultaneously. 
 *
 * All easy handles must be removed from this class before it can be 
 * properly destroyed.
 * Failure to do so invokes undefined behavior.
 */
class Multi_t {
protected:
    void *curl_multi;
    std::size_t handles = 0;

public:
    /**
     * Base class for any exceptions thrown via Ret_except in 
     * this class.
     */
    class Exception: public curl::Exception {
    public:
        const long error_code;

        Exception(long err_code_arg);
        Exception(const Exception&) = default;

        auto what() const noexcept -> const char*;
    };

    Multi_t(void *multi) noexcept;

    Multi_t(const Multi_t&) = delete;
    /**
     * @param other after mv operation, other is in unusable state and can only be destroyed
     *              or move assign another value.
     */
    Multi_t(Multi_t&&) noexcept;

    Multi_t& operator = (const Multi_t&) = delete;
    /**
     * @param other after mv operation, other is in unusable state and can only be destroyed
     *              or move assign another value.
     */
    Multi_t& operator = (Multi_t&&) noexcept;

    /**
     * @return true if this object is usable, false otherwise
     */
    operator bool () const noexcept;

    /**
     * @param easy must be in valid state
     * @return true if not yet added;
     *         <br>false if already added.
     */
    bool add_easy(Easy_ref_t &easy) noexcept;
    /**
     * Undefined behavior if easy is not valid or not added to this multi.
     */
    void remove_easy(Easy_ref_t &easy) noexcept;

    std::size_t get_number_of_handles() const noexcept;

    /**
     * HTTP2 multiplexing configuration.
     *
     * @pre curl_t::has_http2_multiplex_support()
     * @param max_concurrent_stream max concurrent stream for a given connection.
     *                              <br>Should be between [1, 2 ^ 31 - 1].
     *                              <br>Set it to 1 or 0 disable multiplexing.
     *
     * Since curl_t::version >= 7.62.0 (version released way after http2 multiplex support)
     * , multiplex is turned on by default.
     *
     * NOTE that libcurl not always accept max_concurrent_stream tuning.
     * <br>Check curl_t::has_max_concurrent_stream_support().
     *
     * If libcurl does not support tuning, this option will be only used
     * for turning on and off the http2 multiplex.
     */
    void set_multiplexing(long max_concurrent_stream) noexcept;

    /* Interface for poll + perform - multi_poll interface */

    /**
     * @pre curl_t::has_multi_poll_support()
     * @param timeout Must be >= 0, in ms. Pass 0 for infinite.
     * @return number of fd on which interested events occured.
     *
     * poll can return if interesting events has happened or timeout ms
     * has passed and nothing has heppend.
     *
     * It also can return due to pending internal timeout that 
     * has a shorter expiry time than timeout_ms.
     */
    auto poll(curl_waitfd *extra_fds = nullptr, unsigned extra_nfds = 0U, int timeout = 0) noexcept -> 
        Ret_except<int, std::bad_alloc, libcurl_bug>;

    /**
     * @pre curl_t::has_multi_poll_support()
     * @param timeout Must be >= 0, in ms. Pass 0 for infinite.
     * @return -1 when get_number_of_handles() == 0;
     *         Otherwise number of fd on which interested events occured, can be 0.
     *
     * Behavior is same as poll.
     */
    auto break_or_poll(curl_waitfd *extra_fds = nullptr, unsigned extra_nfds = 0U, int timeout = 0) noexcept -> 
        Ret_except<int, std::bad_alloc, libcurl_bug>;

    /**
     * perform_callback can call arbitary member functions on easy, but probably
     * not a good idea to call easy.perform().
     *
     * If you want to destroy and free easy.curl_easy, you must first 
     * multi.remove_easy(easy_ref) it.
     *
     * If easy_ref isn't removed from Multi, then the same transfer will happen again
     * in the next call to Multi_t::perform or 
     * Multi_t::multi_socket_action(socketfd of this easy handler, events, ...).
     */
    using perform_callback_t = void (*)(Easy_ref_t &easy, Easy_ref_t::perform_ret_t ret, 
                                        Multi_t &multi, void *arg);
    /**
     * @pre perform_callback is set.
     * @return number of running handles
     *
     * perform() is called only if poll is used.
     *
     * After perform, perform_callback will be called for each completed
     * easy.
     *
     * **YOU MUST CALL perform() after to start the transfer, then poll**
     *
     * Using libcurl version >= 7.10.3 can provide better error message
     * if Easy_ref_t::ProtocolInternal_error is thrown.
     */
    auto perform(perform_callback_t perform_callback, void *arg) noexcept -> 
        Ret_except<int, std::bad_alloc, Exception, Recursive_api_call_Exception, libcurl_bug>;

    /** 
     * @return should be 0
     *
     * Interface for using arbitary event-based interface - multi_socket interface 
     *
     * Precondition for using this interface:
     *  - curl_t::has_multi_socket_support()
     */
    using socket_callback_t = int (*)(CURL *curl_easy, 
                                      curl_socket_t s,
                                      /**
                                       * Possible value for what:
                                       *  - CURL_POLL_IN,
                                       *  - CURL_POLL_OUT,
                                       *  - CURL_POLL_INOUT,
                                       *  - CURL_POLL_REMOVE
                                       */
                                      int what,
                                      void *userp,
                                      void *per_socketp);

    /**
     * @param timeout_ms -1 means you should delete the timer. 
     *                   All other values are valid expire times in number of milliseconds.
     * @return should be 0 on success, 
     *                   -1 on failure.
     *
     * Your callback function timer_callback should install a non-repeating timer with an interval of timeout_ms.
     * <br>When time that timer fires, call multi_socket_action().
     *
     * The timer_callback will only be called when the timeout expire time is changed.
     */
    using timer_callback_t = int (*)(CURLM *multi, long timeout_ms, void *userp);

    /**
     * @param socket_callback, timer_callback setting them to nullptr would
     *                                        disable multi_socket_action interface.
     *
     * You must call this function with non-NULL socket_callback and timer_callback
     * before adding any easy handles.
     */
    void register_callback(socket_callback_t socket_callback, void *socket_data,
                           timer_callback_t timer_callback, void *timer_data) noexcept;

    /**
     * @pre socketfd must be valid
     * @return std::invalild_argument if socketfd is not valid.
     *
     * By default, per_sockptr == nullptr.
     *
     * You can call this function from socket_callback.
     */
    auto multi_assign(curl_socket_t socketfd, void *per_sockptr) noexcept -> 
        Ret_except<void, std::invalid_argument>;

    /**
     * @pre enable_multi_socket_interface() is called,
     *      perform_callback, socket_callback, timer_callback is set.
     *
     * @param socketfd fd to be notified;
     *                 <br>CURL_SOCKET_TIMEOUT on timeout or to initiate the whole process.
     * @param ev_bitmask Or-ed with 0 or one of the value below:
     *    - CURL_CSELECT_IN,
     *    - CURL_CSELECT_OUT,
     *    - CURL_CSELECT_ERR,
     *
     * **YOU MUST CALL multi_socket_action(CURL_SOCKET_TIMEOUT, 0) to start the transfer, 
     * then call waitever poll interface you use**
     *
     * After multi_socket_action, perform_callback will be called for each completed
     * easy.
     *
     * Using libcurl version >= 7.10.3 can provide better error message
     * if Easy_ref_t::ProtocolInternal_error is thrown.
     */
    auto multi_socket_action(curl_socket_t socketfd, int ev_bitmask, 
                             perform_callback_t perform_callback, void *arg) noexcept -> 
        Ret_except<int, std::bad_alloc, Exception, Recursive_api_call_Exception, libcurl_bug>;

    /**
     * @pre get_number_of_handles() == 0
     */
    ~Multi_t();

protected:
    auto check_perform(long code, int running_handles_tmp, const char *fname,
                       perform_callback_t perform_callback, void *arg) noexcept -> 
        Ret_except<int, std::bad_alloc, Exception, Recursive_api_call_Exception, libcurl_bug>;
};
} /* namespace curl */

#endif
