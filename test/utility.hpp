#ifndef  __curl_test_utility_HPP__
# define __curl_test_utility_HPP__

# include <cstdlib>
# include <iostream>
# include <string_view>

# include "../curl_url.hpp"
# include "../curl_easy.hpp"

auto& operator << (std::ostream &os, curl::Url_ref_t::set_code code)
{
    using set_code = curl::Url_ref_t::set_code;
    switch (code) {
        case set_code::ok:
            return os << "ok";
        case set_code::malform_input:
            return os << "malform_input";
        case set_code::bad_port_number:
            return os << "bad_port_number";
        case set_code::unsupported_scheme:
            return os << "unsupported_scheme";
        default:
            return os << "Unknown set_code";
    }
}
auto& operator << (std::ostream &os, curl::Url_ref_t::get_code code)
{
    using get_code = curl::Url_ref_t::get_code;
    switch (code) {
        case get_code::no_scheme:
            return os << "no_scheme";
        case get_code::no_user:
            return os << "no_user";
        case get_code::no_passwd:
            return os << "no_passwd";
        case get_code::no_options:
            return os << "no_options";
        case get_code::no_host:
            return os << "no_host";
        case get_code::no_port:
            return os << "no_port";
        case get_code::no_query:
            return os << "no_query";
        case get_code::no_fragment:
            return os << "no_fragment";
        default:
            return os << "Unknown get_code";
    }
}
auto& operator << (std::ostream &os, curl::Easy_ref_t::code code)
{
    using code_t = curl::Easy_ref_t::code;
    switch (code) {
        case code_t::ok:
            return os << "ok";
        case code_t::url_malformat:
            return os << "url_malformat";
        case code_t::cannot_resolve_proxy:
            return os << "cannot_resolve_proxy";
        case code_t::cannot_resolve_host:
            return os << "cannot_resolve_host";
        case code_t::cannot_connect:
            return os << "cannot_connect";
        case code_t::remote_access_denied:
            return os << "remote_access_denied";
        case code_t::writeback_error:
            return os << "writeback_error";
        case code_t::upload_failure:
            return os << "upload_failure";
        case code_t::timedout:
            return os << "timedout";
        case code_t::aborted_by_callback:
            return os << "aborted_by_callback";
        default:
            return os << "Unknown curl::Easy_ref_t::code";
    }
}

namespace impl {
template <class ...Ts>
void print(std::ostream &os, Ts &&...args)
{
    ((os << std::forward<Ts>(args)), ...);
}

constexpr void print(std::ostream &os) noexcept
{}
} /* namespace impl */

template <class T1, class T2, class ...Ts>
void assert_same_impl(const T1 &v1, const char *expr1, const T2 &v2, const char *expr2,
                      Ts &&...args) noexcept
{
    if (v1 != v2) {
        impl::print(std::cerr, std::forward<Ts>(args)...);
        std::cerr << expr1 << " != " << expr2 << ": " 
                  << expr1 << " = " << v1 << ", " << expr2 << " = " << v2 << std::endl;
        std::exit(1);
    }
}

#define assert_same(expr1, expr2, ...) \
    assert_same_impl((expr1), # expr1, (expr2), # expr2, ## __VA_ARGS__)

#endif
