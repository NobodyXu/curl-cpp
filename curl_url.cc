#include "curl.hpp"
#include <curl/curl.h>
#include <cassert>

namespace curl {
void curl_t::Url_deleter::operator () (CURLU *p) const noexcept
{
    if (p)
        curl_url_cleanup(p);
}
auto curl_t::create_Url() noexcept -> Url_t
{
    return Url_t{curl_url()};
}

void Url_ref_t::curl_delete::operator () (char *p) const noexcept
{
    curl_free(p);
}

static auto curl_urlset_wrapper(void *url, CURLUPart part, const char *arg) noexcept -> 
    Ret_except<Url_ref_t::set_code, std::bad_alloc>
{
    auto code = curl_url_set(static_cast<CURLU*>(url), part, arg, 0);

    assert(code != CURLUE_BAD_HANDLE);
    assert(code !=  CURLUE_BAD_PARTPOINTER);

    switch (code) {
        case CURLUE_MALFORMED_INPUT:
            return {Url_ref_t::set_code::malform_input};
        case CURLUE_BAD_PORT_NUMBER:
            return {Url_ref_t::set_code::bad_port_number};
        case CURLUE_UNSUPPORTED_SCHEME:
            return {Url_ref_t::set_code::unsupported_scheme};
        case CURLUE_OUT_OF_MEMORY:
            return {std::bad_alloc{}};
        case CURLUE_OK:
            return {Url_ref_t::set_code::ok};

        default:
            assert(false);
            break;
    }
}
auto Url_ref_t::set_url(const char *url_arg) noexcept -> Ret_except<set_code, std::bad_alloc>
{
    return curl_urlset_wrapper(url, CURLUPART_URL, url_arg);
}
auto Url_ref_t::set_scheme(const char *scheme) noexcept -> Ret_except<set_code, std::bad_alloc>
{
    return curl_urlset_wrapper(url, CURLUPART_SCHEME, scheme);
}
auto Url_ref_t::set_options(const char *options) noexcept -> Ret_except<set_code, std::bad_alloc>
{
    return curl_urlset_wrapper(url, CURLUPART_OPTIONS, options);
}
auto Url_ref_t::set_query(const char *query) noexcept -> Ret_except<set_code, std::bad_alloc>
{
    return curl_urlset_wrapper(url, CURLUPART_QUERY, query);
}

static auto curl_urlget_wrapper(void *url, CURLUPart part) noexcept -> 
    Ret_except<Url_ref_t::string, Url_ref_t::get_code, std::bad_alloc>
{
    char *result;
    auto code = curl_url_get(static_cast<CURLU*>(url), part, &result, 0);

    assert(code != CURLUE_BAD_HANDLE);
    assert(code != CURLUE_BAD_PARTPOINTER);

    switch (code) {
        case CURLUE_OK:
            return {Url_ref_t::string(result)};

        case CURLUE_OUT_OF_MEMORY:
            return {std::bad_alloc{}};

        case CURLUE_NO_SCHEME:
            return {Url_ref_t::get_code::no_scheme};
        case CURLUE_NO_USER:
            return {Url_ref_t::get_code::no_user};
        case CURLUE_NO_PASSWORD:
            return {Url_ref_t::get_code::no_passwd};
        case CURLUE_NO_OPTIONS:
            return {Url_ref_t::get_code::no_options};
        case CURLUE_NO_HOST:
            return {Url_ref_t::get_code::no_host};
        case CURLUE_NO_PORT:
            return {Url_ref_t::get_code::no_port};
        case CURLUE_NO_QUERY:
            return {Url_ref_t::get_code::no_query};
        case CURLUE_NO_FRAGMENT:
            return {Url_ref_t::get_code::no_fragment};

        default:
            assert(false);
            break;
    }
}
auto Url_ref_t::get_url() const noexcept -> Ret_except<string, get_code, std::bad_alloc>
{
    return curl_urlget_wrapper(static_cast<CURLU*>(url), CURLUPART_URL);
}
auto Url_ref_t::get_scheme() const noexcept -> Ret_except<string, get_code, std::bad_alloc>
{
    return curl_urlget_wrapper(static_cast<CURLU*>(url), CURLUPART_SCHEME);
}
auto Url_ref_t::get_options() const noexcept -> Ret_except<string, get_code, std::bad_alloc>
{
    return curl_urlget_wrapper(static_cast<CURLU*>(url), CURLUPART_OPTIONS);
}
auto Url_ref_t::get_query() const noexcept -> Ret_except<string, get_code, std::bad_alloc>
{
    return curl_urlget_wrapper(static_cast<CURLU*>(url), CURLUPART_QUERY);
}
} /* namespace curl */
