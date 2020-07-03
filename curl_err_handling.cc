#include "curl.hpp"
#include <curl/curl.h>

#include <cstdio>
#include <cassert>

namespace curl {
/* For Easy_t */
Easy_t::Exception::Exception(long err_code_arg):
    curl::Exception{""},
    error_code{err_code_arg}
{}
auto Easy_t::Exception::what() const noexcept -> const char*
{
    return curl_easy_strerror(static_cast<CURLcode>(error_code));
}


Easy_t::ProtocolInternal_error::ProtocolInternal_error(long error_code_arg, const char *error_buffer):
    Exception{error_code_arg}
{
    const char *code_name;
    switch (error_code_arg) {
        case CURLE_HTTP2:
            code_name = "CURLE_HTTP2";
            break;

        case CURLE_SSL_CONNECT_ERROR:
            code_name = "CURLE_SSL_CONNECT_ERROR";
            break;

        case CURLE_UNKNOWN_OPTION:
            code_name = "CURLE_UNKNOWN_OPTION";
            break;

        case CURLE_HTTP3:
            code_name = "CURLE_HTTP3";
            break;

        default:
            assert(false);
    }

    std::snprintf(buffer, buffer_size, "%s: %s", code_name, error_buffer);
}

auto Easy_t::ProtocolInternal_error::what() const noexcept -> const char*
{
    return buffer;
}
/* End of Easy_t */
}
