#include "curl.hpp"
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

    running_handles = other.running_handles;

    perform_callback = other.perform_callback;
    data = other.data;
}
Multi_t& Multi_t::operator = (Multi_t &&other) noexcept
{
    if (curl_multi)
        curl_multi_cleanup(curl_multi);
    curl_multi = other.curl_multi;
    other.curl_multi = nullptr;

    running_handles = other.running_handles;

    perform_callback = other.perform_callback;
    data = other.data;

    return *this;
}

bool Multi_t::add_easy(Easy_ref_t &easy) noexcept
{
    curl_easy_setopt(easy.ptrs.first, CURLOPT_PRIVATE, easy.ptrs.second);
    return curl_multi_add_handle(curl_multi, easy.ptrs.first) != CURLM_ADDED_ALREADY;
}
void Multi_t::remove_easy(Easy_ref_t &easy) noexcept
{
    curl_multi_remove_handle(curl_multi, easy.ptrs.first);
}

int Multi_t::get_number_of_running_handles() const noexcept
{
    return running_handles;
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
    Ret_except<int, std::bad_alloc>
{
    int numfds;

    if (curl_multi_poll(curl_multi, extra_fds, extra_nfds, timeout, &numfds) == CURLM_OUT_OF_MEMORY)
        return {std::bad_alloc{}};
    else
        return {numfds};
}

auto Multi_t::check_perform(long code, int running_handles_tmp) noexcept -> 
    Ret_except<int, std::bad_alloc, Exception, libcurl_bug>
{
    if (code == CURLM_OUT_OF_MEMORY)
        return {std::bad_alloc{}};
    else if (code == CURLM_INTERNAL_ERROR)
        return {libcurl_bug{"This can only be returned if libcurl bugs. Please report it to us!"}};

    assert(code == CURLM_OK);

    int msgq = 0;
    for (CURLMsg *m; (m = curl_multi_info_read(curl_multi, &msgq)); )
        if (m->msg == CURLMSG_DONE) {
            char *error_buffer = nullptr;
            curl_easy_getinfo(m->easy_handle, CURLINFO_PRIVATE, &error_buffer);

            auto easy = Easy_ref_t{{static_cast<char*>(m->easy_handle), nullptr}};

            constexpr const auto msg = "In Multi_t::perform or Multi_t::multi_socket_action";
            perform_callback(easy, easy.check_perform(m->data.result, msg), data);

            remove_easy(easy);
        }

    running_handles = running_handles_tmp;
    return {running_handles};
}
auto Multi_t::perform() noexcept -> Ret_except<int, std::bad_alloc, Exception, libcurl_bug>
{
    int running_handles_tmp = 0;

    CURLMcode code;
    while ((code = curl_multi_perform(curl_multi, &running_handles_tmp)) == CURLM_CALL_MULTI_PERFORM);

    return check_perform(code, running_handles_tmp);
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
    Ret_except<void, Recursive_api_call_Exception, std::invalid_argument>
{
    auto code = curl_multi_assign(curl_multi, socketfd, per_sockptr);
    if (code == CURLM_BAD_SOCKET)
        return {std::invalid_argument{"In curl::Multi_t::multi_assign: socketfd is not valid."}};
    else if (code == CURLM_RECURSIVE_API_CALL)
        return {Recursive_api_call_Exception{__PRETTY_FUNCTION__}};

    return {};
}
auto Multi_t::multi_socket_action(curl_socket_t socketfd, int ev_bitmask) noexcept -> 
    Ret_except<int, std::bad_alloc, Exception, libcurl_bug>
{
    int running_handles_tmp;
    auto code = curl_multi_socket_action(curl_multi, socketfd, ev_bitmask, &running_handles_tmp);
    return check_perform(code, running_handles_tmp);
}

Multi_t::~Multi_t()
{
    if (curl_multi)
        curl_multi_cleanup(curl_multi);
}
} /* namespace curl */
