#include "curl_easy.hpp"
#include "curl_multi.hpp"

#include <curl/curl.h>

#include <cstdio>
#include <cassert>

namespace curl {
/* For Easy_t */
Easy_ref_t::Exception::Exception(long err_code_arg):
    curl::Exception{""},
    error_code{err_code_arg}
{}
auto Easy_ref_t::Exception::what() const noexcept -> const char*
{
    return curl_easy_strerror(static_cast<CURLcode>(error_code));
}


Easy_ref_t::ProtocolInternal_error::ProtocolInternal_error(long error_code_arg, const char *error_buffer):
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

auto Easy_ref_t::ProtocolInternal_error::what() const noexcept -> const char*
{
    return buffer;
}

auto Easy_ref_t::check_perform(long code, const char *fname) noexcept -> perform_ret_t
{
    switch (code) {
        case CURLE_OK:
            return {code::ok};

        case CURLE_URL_MALFORMAT:
            return {code::url_malformat};

        case CURLE_NOT_BUILT_IN:
            return {NotBuiltIn_error{code}};

        case CURLE_COULDNT_RESOLVE_PROXY:
            return {code::cannot_resolve_proxy};

        case CURLE_COULDNT_RESOLVE_HOST:
        case CURLE_FTP_CANT_GET_HOST:
            return {code::cannot_resolve_host};

        case CURLE_COULDNT_CONNECT:
            return {code::cannot_connect};

        case CURLE_REMOTE_ACCESS_DENIED:
            return {code::remote_access_denied};

        case CURLE_WRITE_ERROR:
            return {code::writeback_error};

        case CURLE_UPLOAD_FAILED:
            return {code::upload_failure};

        case CURLE_ABORTED_BY_CALLBACK:
            return {code::aborted_by_callback};

        case CURLE_OUT_OF_MEMORY:
            return {std::bad_alloc{}};

        case CURLE_OPERATION_TIMEDOUT:
            return {code::timedout};

        case CURLE_BAD_FUNCTION_ARGUMENT:
            return std::invalid_argument{"A function was called with a bad parameter."};

        case CURLE_TOO_MANY_REDIRECTS:
            return {code::too_many_redirects};

        case CURLE_RECURSIVE_API_CALL:
            return {Recursive_api_call_Exception{fname}};

        default:
            return {Exception{code}};

        case CURLE_HTTP2:
        case CURLE_SSL_CONNECT_ERROR:
        case CURLE_UNKNOWN_OPTION:
        case CURLE_HTTP3:
            return {ProtocolInternal_error{code, error_buffer}};
    }
}
/* End of Easy_t */

/* For Multi_t */
Multi_t::Exception::Exception(long err_code_arg):
    curl::Exception{""},
    error_code{err_code_arg}
{}
auto Multi_t::Exception::what() const noexcept -> const char*
{
    return curl_multi_strerror(static_cast<CURLMcode>(error_code));
}
/* End of Multi_t */
}
