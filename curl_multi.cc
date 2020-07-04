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
}
Multi_t& Multi_t::operator = (Multi_t &&other) noexcept
{
    if (curl_multi)
        curl_multi_cleanup(curl_multi);
    curl_multi = other.curl_multi;
    other.curl_multi = nullptr;
    return *this;
}

bool Multi_t::add_easy(Easy_t &easy) noexcept
{
    return curl_multi_add_handle(curl_multi, easy.curl_easy) != CURLM_ADDED_ALREADY;
}
void Multi_t::remove_easy(Easy_t &easy) noexcept
{
    curl_multi_remove_handle(curl_multi, easy.curl_easy);
}

auto Multi_t::poll(curl_waitfd *extra_fds, unsigned extra_nfds, int timeout) noexcept -> 
    Ret_except<int, std::bad_alloc>
{
    int numfds;

    if (curl_multi_poll(curl_multi, extra_fds, extra_nfds, timeout, &numfds) == CURLM_OUT_OF_MEMORY)
        return {std::bad_alloc{}};
    else
        return {numfds};
}

auto Multi_t::perform() noexcept -> Ret_except<int, std::bad_alloc, Exception, libcurl_bug>
{
    int running_handles_tmp = 0;

    CURLMcode code;
    while ((code = curl_multi_perform(curl_multi, &running_handles_tmp)) == CURLM_CALL_MULTI_PERFORM);

    if (code == CURLM_OUT_OF_MEMORY)
        return {std::bad_alloc{}};
    else if (code == CURLM_INTERNAL_ERROR)
        return {libcurl_bug{"This can only be returned if libcurl bugs. Please report it to us!"}};

    assert(code == CURLM_OK);

    if (running_handles != running_handles_tmp) {
        int msgq = 0;
        for (CURLMsg *m; (m = curl_multi_info_read(curl_multi, &msgq)); )
            if (m->msg == CURLMSG_DONE) {
                auto &easy = Easy_t::get_easy(m->easy_handle);
                perform_callback(easy, easy.check_perform(m->data.result), data);
                remove_easy(easy);
            }
    }

    running_handles = running_handles_tmp;
    return {running_handles};
}

Multi_t::~Multi_t()
{
    if (curl_multi)
        curl_multi_cleanup(curl_multi);
}
} /* namespace curl */
