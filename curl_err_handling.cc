#include "curl.hpp"
#include <curl/curl.h>

namespace curl {
/* For handle_t */
void handle_t::check_easy(int code, const char *expr)
{
    if (code == CURLE_OUT_OF_MEMORY)
        throw std::bad_alloc{};
    else if (code == CURLE_BAD_FUNCTION_ARGUMENT)
        throw std::invalid_argument{expr};
    else if (code == CURLE_NOT_BUILT_IN)
        throw NotSupported_error{"A feature, protocol or option wasn't in this libcurl at compilation."};
    else if (code == CURLE_COULDNT_RESOLVE_PROXY)
        throw handle_t::CannotResolve_error{code};
    else if (code == CURLE_COULDNT_RESOLVE_HOST)
        throw handle_t::CannotResolve_error{code};
    else if (code == CURLE_COULDNT_CONNECT)
        throw handle_t::ConnnectionFailed_error{code};
    else if (code == CURLE_OPERATION_TIMEDOUT)
        throw handle_t::Timeout_error{code};
    else if (code == CURLE_UNKNOWN_OPTION)
        throw NotSupported_error{expr};
    else if (code != CURLE_OK)
        throw handle_t::Exception{code};
}
handle_t::Exception::Exception(long err_code_arg):
    curl::Exception{""},
    error_code{err_code_arg}
{}
auto handle_t::Exception::what() const noexcept -> const char*
{
    return curl_easy_strerror(static_cast<CURLcode>(error_code));
}

handle_t::ProtocolError::ProtocolError(long err_code_arg, long response_code_arg):
    handle_t::Exception{err_code_arg},
    response_code{response_code_arg}
{}
/* End of handle_t */


/* For Url */
Url::Exception::Exception(int err_code_arg):
    curl::Exception{""},
    error_code{err_code_arg}
{}
auto Url::Exception::what() const noexcept -> const char*
{
    switch (error_code) {
        case CURLUE_BAD_HANDLE:
            return "An argument that should be a CURLU pointer was passed in as a NULL.";
        case CURLUE_BAD_PARTPOINTER:
            return "A NULL pointer was passed to the 'part' argument of curl_url_get.";
        case CURLUE_MALFORMED_INPUT:
            return "A malformed input was passed to a URL API function.";
        case CURLUE_BAD_PORT_NUMBER:
            return "The port number was not a decimal number between 0 and 65535.";
        case CURLUE_UNSUPPORTED_SCHEME:
            return "This libcurl build doesn't support the given URL scheme.";
        case CURLUE_URLDECODE:
            return "URL decode error, most likely because of rubbish in the input.";
        case CURLUE_OUT_OF_MEMORY:
            return "SHOULD THROW std::bad_alloc instead: A memory function failed.";
        case CURLUE_USER_NOT_ALLOWED:
            return "Credentials was passed in the URL when prohibited.";
        case CURLUE_UNKNOWN_PART:
            return "An unknown part ID was passed to a URL API function.";
        case CURLUE_NO_SCHEME:
            return "There is no scheme part in the URL.";
        case CURLUE_NO_USER:
            return "There is no user part in the URL.";
        case CURLUE_NO_PASSWORD:
            return "There is no password part in the URL.";
        case CURLUE_NO_OPTIONS:
            return "There is no options part in the URL.";
        case CURLUE_NO_HOST:
            return "There is no host part in the URL.";
        case CURLUE_NO_PORT:
            return "There is no port part in the URL.";
        case CURLUE_NO_QUERY:
            return "There is no query part in the URL.";
        case CURLUE_NO_FRAGMENT:
            return "There is no fragment part in the URL.";

        default:
            return "Unknown/unsupported error code.";
    }

}

void Url::check_url(int code)
{
    if (code == CURLUE_OUT_OF_MEMORY)
        throw std::bad_alloc{};
    else if (code == CURLUE_BAD_HANDLE)
        throw std::invalid_argument{"An argument that should be a CURLU pointer was passed in as a NULL."};
    else if (code == CURLUE_BAD_PARTPOINTER)
        throw std::invalid_argument{"A NULL pointer was passed to the 'part' argument of curl_url_get."};
    else if (code == CURLUE_MALFORMED_INPUT)
        throw std::invalid_argument{"A malformed input was passed to a URL API function."};
    else if (code == CURLUE_BAD_PORT_NUMBER)
        throw std::domain_error{"The port number was not a decimal number between 0 and 65535."};
    else if (code == CURLUE_UNSUPPORTED_SCHEME)
        throw NotSupported_error{"This libcurl build doesn't support the given URL scheme."};
    else if (code != CURLUE_OK)
        throw Url::Exception{code};
}
/* End of Url */
}
