#include "curl.hpp"
#include <curl/curl.h>
#include <cassert>

namespace curl {
Url::Url(Ret_except<void, std::bad_alloc> &e) noexcept:
    url{curl_url()}
{
    if (url == nullptr)
        e.set_exception<std::bad_alloc>();
}
Url::Url(const Url &other, Ret_except<void, std::bad_alloc> &e) noexcept:
    url{(curl_url_dup(static_cast<CURLU*>(other.url)))}
{
    if (url == nullptr)
        e.set_exception<std::bad_alloc>();
}
Url::Url(Url &&other) noexcept:
    url{other.url}
{
    other.url = nullptr;
}

auto Url::operator = (const Url &other) noexcept -> Ret_except<void, std::bad_alloc>
{
    url = curl_url_dup(static_cast<CURLU*>(other.url));
    if (url == nullptr)
        return {std::bad_alloc{}};
    else
        return {};
}
Url& Url::operator = (Url &&other) noexcept
{
    if (url)
        curl_url_cleanup(static_cast<CURLU*>(url));
    url = other.url;
    other.url = nullptr;
    return *this;
}

static auto curl_urlset_wrapper(void *url, CURLUPart part, const char *arg) noexcept -> 
    Ret_except<Url::set_code, std::bad_alloc>
{
    auto code = curl_url_set(static_cast<CURLU*>(url), part, arg, 0);

    assert(code != CURLUE_BAD_HANDLE);
    assert(code !=  CURLUE_BAD_PARTPOINTER);

    switch (code) {
        case CURLUE_MALFORMED_INPUT:
            return {Url::set_code::malform_input};
        case CURLUE_BAD_PORT_NUMBER:
            return {Url::set_code::bad_port_number};
        case CURLUE_UNSUPPORTED_SCHEME:
            return {Url::set_code::unsupported_scheme};
        case CURLUE_OUT_OF_MEMORY:
            return {std::bad_alloc{}};
        case CURLUE_OK:
            return {Url::set_code::ok};

        default:
            assert(false);
            break;
    }
}
auto Url::set_url(const char *url_arg) noexcept -> Ret_except<set_code, std::bad_alloc>
{
    return curl_urlset_wrapper(url, CURLUPART_URL, url_arg);
}
auto Url::set_scheme(const char *scheme) noexcept -> Ret_except<set_code, std::bad_alloc>
{
    return curl_urlset_wrapper(url, CURLUPART_SCHEME, scheme);
}
auto Url::set_options(const char *options) noexcept -> Ret_except<set_code, std::bad_alloc>
{
    return curl_urlset_wrapper(url, CURLUPART_OPTIONS, options);
}
auto Url::set_query(const char *query) noexcept -> Ret_except<set_code, std::bad_alloc>
{
    return curl_urlset_wrapper(url, CURLUPART_QUERY, query);
}

auto Url::get_url() const -> string
{
    char *fullurl;
    check_url(curl_url_get(static_cast<CURLU*>(url), CURLUPART_URL, &fullurl, 0));
    return string(fullurl, &curl_free);
}

Url::~Url()
{
    if (url)
        curl_url_cleanup(static_cast<CURLU*>(url));
}
} /* namespace curl */
