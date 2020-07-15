#include "curl_multi.hpp"
#include <curl/curl.h>

#include <cassert>

namespace curl {
auto curl_t::create_multi() noexcept -> Ret_except<Multi_t, curl::Exception>
{
    auto *multi = curl_multi_init();
    if (!multi)
        return {curl::Exception{"curl_multi_init failed"}};
    return {std::in_place_type<Multi_t>, multi};
}

Multi_t::Multi_t(void *multi) noexcept:
    curl_multi{multi}
{}

Multi_t::Multi_t(Multi_t &&other) noexcept:
    curl_multi{other.curl_multi}
{
    other.curl_multi = nullptr;

    handles = other.handles;
}
Multi_t& Multi_t::operator = (Multi_t &&other) noexcept
{
    if (curl_multi)
        curl_multi_cleanup(curl_multi);
    curl_multi = other.curl_multi;
    other.curl_multi = nullptr;

    handles = other.handles;

    return *this;
}

Multi_t::operator bool () const noexcept
{
    return curl_multi != nullptr;
}

Multi_t::~Multi_t()
{
    if (curl_multi)
        curl_multi_cleanup(curl_multi);
}

bool Multi_t::add_easy(Easy_ref_t &easy) noexcept
{
    bool success = curl_multi_add_handle(curl_multi, easy.curl_easy) != CURLM_ADDED_ALREADY;

    handles += success;

    return success;
}
void Multi_t::remove_easy(Easy_ref_t &easy) noexcept
{
    --handles;
    curl_multi_remove_handle(curl_multi, easy.curl_easy);
}

std::size_t Multi_t::get_number_of_handles() const noexcept
{
    return handles;
}

void Multi_t::set_multiplexing(long max_concurrent_stream) noexcept
{
    long bitmask = max_concurrent_stream > 1 ? CURLPIPE_MULTIPLEX : CURLPIPE_NOTHING;
    curl_multi_setopt(curl_multi, CURLMOPT_PIPELINING, bitmask);

    if (max_concurrent_stream > 1)
        curl_multi_setopt(curl_multi, CURLMOPT_MAX_CONCURRENT_STREAMS, max_concurrent_stream);
}

/* Interface for poll + perform - multi_poll interface */
auto Multi_t::poll(curl_waitfd *extra_fds, unsigned extra_nfds, int timeout) noexcept -> 
    Ret_except<int, std::bad_alloc, libcurl_bug>
{
    int numfds;

    auto code = curl_multi_poll(curl_multi, extra_fds, extra_nfds, timeout, &numfds);
    if (code == CURLM_OUT_OF_MEMORY)
        return {std::bad_alloc{}};
    else if (code == CURLM_INTERNAL_ERROR)
        return {libcurl_bug{"Bug in curl_multi_poll!"}};

    assert(code == CURLM_OK);
    return {numfds};
}

auto Multi_t::break_or_poll(curl_waitfd *extra_fds, unsigned extra_nfds, int timeout) noexcept -> 
    Ret_except<int, std::bad_alloc, libcurl_bug>
{
    if (get_number_of_handles() == 0)
        return {-1};
    else
        return poll(extra_fds, extra_nfds, timeout);
}

auto Multi_t::check_perform(long code, int running_handles, const char *fname,
                            perform_callback_t perform_callback, void *arg) noexcept -> 
    Ret_except<int, std::bad_alloc, Exception, Recursive_api_call_Exception, libcurl_bug>
{
    if (code == CURLM_OUT_OF_MEMORY)
        return {std::bad_alloc{}};
    else if (code == CURLM_INTERNAL_ERROR)
        return {libcurl_bug{fname}};
    else if (code == CURLM_RECURSIVE_API_CALL)
        return {Recursive_api_call_Exception{fname}};

    assert(code == CURLM_OK);

    int msgq = 0;
    for (CURLMsg *m; (m = curl_multi_info_read(curl_multi, &msgq)); )
        if (m->msg == CURLMSG_DONE) {
            Easy_ref_t easy{static_cast<char*>(m->easy_handle)};
            perform_callback(easy, easy.check_perform(m->data.result, fname), *this, arg);
        }

    return {running_handles};
}
auto Multi_t::perform(perform_callback_t perform_callback, void *arg) noexcept -> 
    Ret_except<int, std::bad_alloc, Exception, Recursive_api_call_Exception, libcurl_bug>
{
    int running_handles = 0;

    CURLMcode code;
    while ((code = curl_multi_perform(curl_multi, &running_handles)) == CURLM_CALL_MULTI_PERFORM);

    return check_perform(code, running_handles, "In curl_multi_perform", perform_callback, arg);
}

/* Interface for using arbitary event-based interface - multi_socket interface */
void Multi_t::register_callback(socket_callback_t socket_callback, void *socket_data,
                                timer_callback_t timer_callback, void *timer_data) noexcept
{
    curl_multi_setopt(curl_multi, CURLMOPT_SOCKETFUNCTION, socket_callback);
    curl_multi_setopt(curl_multi, CURLMOPT_SOCKETDATA, socket_data);

    curl_multi_setopt(curl_multi, CURLMOPT_TIMERFUNCTION, timer_callback);
    curl_multi_setopt(curl_multi, CURLMOPT_TIMERDATA, timer_data);
}

auto Multi_t::multi_assign(curl_socket_t socketfd, void *per_sockptr) noexcept -> 
    Ret_except<void, std::invalid_argument>
{
    auto code = curl_multi_assign(curl_multi, socketfd, per_sockptr);
    if (code == CURLM_BAD_SOCKET)
        return {std::invalid_argument{"In curl::Multi_t::multi_assign: socketfd is not valid."}};

    return {};
}
auto Multi_t::multi_socket_action(curl_socket_t socketfd, int ev_bitmask, 
                                  perform_callback_t perform_callback, void *arg) noexcept -> 
    Ret_except<int, std::bad_alloc, Exception, Recursive_api_call_Exception, libcurl_bug>
{
    int running_handles;

    CURLMcode code;
    while ((code = curl_multi_socket_action(curl_multi, socketfd, ev_bitmask, &running_handles)) == CURLM_CALL_MULTI_PERFORM);

    return check_perform(code, running_handles, "In curl_multi_socket_action", perform_callback, arg);
}
} /* namespace curl */
