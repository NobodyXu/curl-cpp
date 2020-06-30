#include "curl.hpp"
#include <curl/curl.h>
#include <cassert>

namespace curl {
Url::Url():
    url{curl_url()}
{
    if (url == nullptr)
        throw std::bad_alloc{};
}
Url::Url(const char *url_arg):
    Url{}
{
    set_url(url_arg);
}
Url::Url(const Url &other):
    url{curl_url_dup(static_cast<CURLU*>(other.url))}
{
    if (url == nullptr)
        throw std::bad_alloc{};
}
Url::Url(Url &&other) noexcept:
    url{other.url}
{
    other.url = nullptr;
}

Url& Url::operator = (const Url &other)
{
    url = curl_url_dup(static_cast<CURLU*>(other.url));
    if (url == nullptr)
        throw std::bad_alloc{};
    return *this;
}
Url& Url::operator = (Url &&other) noexcept
{
    if (url)
        curl_url_cleanup(static_cast<CURLU*>(url));
    url = other.url;
    other.url = nullptr;
    return *this;
}

static auto curl_urlset_wrapper(void *url, CURLUPart part, const char *arg)
{
    auto code = curl_url_set(static_cast<CURLU*>(url), part, arg, 0);
    assert(code != CURLUE_BAD_HANDLE);
    assert(code !=  CURLUE_BAD_PARTPOINTER);
    return code;
}
void Url::set_url(const char *url_arg)
{
    check_url(curl_urlset_wrapper(url, CURLUPART_URL, url_arg));
}
void Url::set_scheme(const char *scheme)
{
    check_url(curl_urlset_wrapper(url, CURLUPART_SCHEME, scheme));
}
void Url::set_options(const char *options)
{
    check_url(curl_urlset_wrapper(url, CURLUPART_OPTIONS, options));
}
void Url::set_query(const char *query)
{
    check_url(curl_urlset_wrapper(url, CURLUPART_QUERY, query));
}

auto Url::get_url() const -> fullurl_str
{
    char *fullurl;
    check_url(curl_url_get(static_cast<CURLU*>(url), CURLUPART_URL, &fullurl, 0));
    return fullurl_str(fullurl, &curl_free);
}

Url::~Url()
{
    if (url)
        curl_url_cleanup(static_cast<CURLU*>(url));
}
} /* namespace curl */
