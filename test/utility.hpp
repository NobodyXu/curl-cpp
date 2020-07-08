#ifndef  __curl_test_utility_HPP__
# define __curl_test_utility_HPP__

# include <cstdlib>
# include <iostream>
# include <string_view>

# include "../curl_url.hpp"

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


template <class T>
void assert_same_impl(const T &v1, const char *expr1, const T &v2, const char *expr2) noexcept
{
    if (v1 != v2) {
        std::cerr << expr1 << " != " << expr2 << ": " 
                  << expr1 << " = " << v1 << ", " << expr2 << " = " << v2 << std::endl;
        std::exit(1);
    }
}

#define assert_same(expr1, expr2) assert_same_impl((expr1), # expr1, (expr2), # expr2)

#endif
