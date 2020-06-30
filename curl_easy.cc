#include "curl.hpp"
#include <curl/curl.h>

#include <utility>
#include <algorithm>

#define CHECK(expr) check_easy((expr), #expr)

namespace curl {
static void check_easy(CURLcode code, const char *expr)
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
        throw std::invalid_argument{expr};
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

handle_t curl_t::create_handle()
{
    CURL *curl = curl_easy_init();
    if (!curl)
        throw curl::Exception("curl_easy_init failed");

    if (debug_stream) {
        curl_easy_setopt(curl, CURLOPT_STDERR, debug_stream);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }

    // Enable TCP_fastopen
    curl_easy_setopt(curl, CURLOPT_TCP_FASTOPEN, 1L);
    // Enable TCP_keepalive
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

    // Fail on HTTP 4xx errors
    check_easy(curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L), "CURLOPT_FAILONERROR");

    // Attempt to optimize buffer size for writeback
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, CURL_MAX_READ_SIZE);

    handle_t handle{curl};
    return handle;
}
handle_t::handle_t(void *p):
    curl_easy{p}
{
    auto write_callback = [](char *buffer, std::size_t _, std::size_t size, void *arg) noexcept {
        handle_t *handle = static_cast<handle_t*>(arg);
        if (handle->writeback)
            return handle->writeback(buffer, size, handle->data);
        else
            return size;
    };

    curl_easy_setopt(curl_easy, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl_easy, CURLOPT_WRITEFUNCTION, write_callback);
}
handle_t::handle_t(const handle_t &other):
    curl_easy{curl_easy_duphandle(other.curl_easy)},
    writeback{other.writeback},
    data{other.data}
{
    if (!curl_easy)
        throw curl::Exception("curl_easy_duphandle failed");
    curl_easy_setopt(curl_easy, CURLOPT_WRITEDATA, this);
}

void handle_t::set(const Url &url, const char *useragent, const char *encoding)
{
    // Set url
    CHECK(curl_easy_setopt(curl_easy, CURLOPT_CURLU, url.url));
    // Set useragent
    CHECK(curl_easy_setopt(curl_easy, CURLOPT_USERAGENT, useragent));
    // Set encoding
    CHECK(curl_easy_setopt(curl_easy, CURLOPT_ACCEPT_ENCODING, encoding));
}

void handle_t::request_get()
{
    CHECK(curl_easy_setopt(curl_easy, CURLOPT_HTTPGET, 1L));
}
void handle_t::request_post(const void *data, std::size_t len)
{
    CHECK(curl_easy_setopt(curl_easy, CURLOPT_POSTFIELDSIZE_LARGE, len));
    curl_easy_setopt(curl_easy, CURLOPT_POSTFIELDS, data);
}

void handle_t::perform()
{
    auto code = curl_easy_perform(curl_easy);

    if (code == CURLE_QUOTE_ERROR)
        throw handle_t::ProtocolError{code, get_response_code()};
    else if (code == CURLE_HTTP_RETURNED_ERROR)
        throw handle_t::ProtocolError{code, get_response_code()};
    else
        check_easy(code, "curl_easy_perform(curl_easy)");
}

long handle_t::get_response_code() const
{
    long response_code;
    curl_easy_getinfo(curl_easy, CURLINFO_RESPONSE_CODE, &response_code);
    return response_code;
}

std::size_t handle_t::getinfo_sizeof_uploaded() const
{
    curl_off_t ul;
    CHECK(curl_easy_getinfo(curl_easy, CURLINFO_SIZE_UPLOAD_T, &ul));
    return ul;
}
std::size_t handle_t::getinfo_sizeof_response_header() const
{
    long size;
    CHECK(curl_easy_getinfo(curl_easy, CURLINFO_HEADER_SIZE, &size));
    return size;
}
std::size_t handle_t::getinfo_sizeof_response_body() const
{
    curl_off_t dl;
    CHECK(curl_easy_getinfo(curl_easy, CURLINFO_SIZE_DOWNLOAD_T, &dl));
    return dl;
}

handle_t::~handle_t()
{
    if (curl_easy)
        curl_easy_cleanup(curl_easy);
}

std::string handle_t::readall()
{
    std::string response;

    writeback = [](char *buffer, std::size_t size, void *data) {
        std::string &response = *static_cast<std::string*>(data);
        response.append(buffer, buffer + size);
        return size;
    };
    data = &response;

    perform();

    return response;
}
std::string handle_t::read(std::size_t bytes)
{
    std::string response;
    response.reserve(bytes);

    writeback = [](char *buffer, std::size_t size, void *data) {
        std::string &response = *static_cast<std::string*>(data);

        auto str_size = response.size();
        auto str_cap = response.capacity();
        if (str_size < str_cap)
            response.append(buffer, buffer + std::min(size, str_cap - str_size));

        return size;
    };
    data = &response;

    perform();

    return response;
}
void handle_t::establish_connection_only()
{
    CHECK(curl_easy_setopt(curl_easy, CURLOPT_HTTPGET, 1L));
    curl_easy_setopt(curl_easy, CURLOPT_BUFFERSIZE, CURL_MAX_READ_SIZE);

    curl_easy_setopt(curl_easy, CURLOPT_NOBODY, 1);
    perform();
    curl_easy_setopt(curl_easy, CURLOPT_NOBODY, 0);
}
} /* namespace curl */
