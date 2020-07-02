#include "curl.hpp"
#include <curl/curl.h>

#include <arpa/inet.h>

#include <utility>
#include <algorithm>

#define CHECK(expr) check_easy((expr), #expr)

#define CHECK_OOM(code)                \
    if ((code) == CURLE_OUT_OF_MEMORY) \
        return {std::bad_alloc{}}

namespace curl {
auto curl_t::create_easy() noexcept -> Ret_except<Easy_t, curl::Exception>
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return {curl::Exception{"curl_easy_init failed"}};

    if (debug_stream) {
        curl_easy_setopt(curl, CURLOPT_STDERR, debug_stream);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }

    // Enable TCP_fastopen
    curl_easy_setopt(curl, CURLOPT_TCP_FASTOPEN, 1L);
    // Enable TCP_keepalive
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

    // Attempt to optimize buffer size for writeback
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, CURL_MAX_READ_SIZE);

    return {std::in_place_type<Easy_t>, curl};
}
Easy_t::Easy_t(void *curl) noexcept:
    curl_easy{curl}
{
    auto write_callback = [](char *buffer, std::size_t _, std::size_t size, void *arg) noexcept {
        Easy_t *handle = static_cast<Easy_t*>(arg);
        if (handle->writeback)
            return handle->writeback(buffer, size, handle->data);
        else
            return size;
    };

    curl_easy_setopt(curl_easy, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl_easy, CURLOPT_WRITEFUNCTION, write_callback);

    curl_easy_setopt(curl_easy, CURLOPT_ERRORBUFFER, error_buffer);
}
Easy_t::Easy_t(const Easy_t &other, Ret_except<void, curl::Exception> &e) noexcept:
    curl_easy{curl_easy_duphandle(other.curl_easy)},
    writeback{other.writeback},
    data{other.data}
{
    if (!curl_easy)
        e.set_exception<curl::Exception>("curl_easy_duphandle failed");
    curl_easy_setopt(curl_easy, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl_easy, CURLOPT_ERRORBUFFER, error_buffer);
}
Easy_t::Easy_t(Easy_t &&other) noexcept:
    curl_easy{other.curl_easy}
{
    other.curl_easy = nullptr;
    curl_easy_setopt(curl_easy, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl_easy, CURLOPT_ERRORBUFFER, error_buffer);
}

void Easy_t::set_url(const Url &url) noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_CURLU, url.url);
}
auto Easy_t::set_url(const char *url) noexcept -> Ret_except<void, std::bad_alloc>
{
    CHECK_OOM(curl_easy_setopt(curl_easy, CURLOPT_URL, url));
    return {};
}

auto Easy_t::set_useragent(const char *useragent) noexcept -> Ret_except<void, std::bad_alloc>
{
    CHECK_OOM(curl_easy_setopt(curl_easy, CURLOPT_USERAGENT, useragent));
    return {};
}
auto Easy_t::set_encoding(const char *encoding) noexcept -> Ret_except<void, std::bad_alloc>
{
    CHECK_OOM(curl_easy_setopt(curl_easy, CURLOPT_ACCEPT_ENCODING, encoding));
    return {};
}

auto Easy_t::set_source_ip(const char *ip_addr) noexcept -> Ret_except<void, std::bad_alloc>
{
    CHECK_OOM(curl_easy_setopt(curl_easy, CURLOPT_INTERFACE, ip_addr));
    return {};
}

void Easy_t::request_get() noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_HTTPGET, 1L);
}
void Easy_t::request_post(const void *data, std::uint32_t len) noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_POSTFIELDSIZE, len);
    curl_easy_setopt(curl_easy, CURLOPT_POSTFIELDS, data);
}
void Easy_t::request_post_large(const void *data, std::size_t len) noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_POSTFIELDSIZE_LARGE, len);
    curl_easy_setopt(curl_easy, CURLOPT_POSTFIELDS, data);
}

auto Easy_t::perform() noexcept -> 
    Ret_except<code, std::bad_alloc, std::invalid_argument, std::length_error, Exception, NotBuiltIn_error, 
               ProtocolInternal_error>
{
    auto code = curl_easy_perform(curl_easy);
    switch (code) {
        case CURLE_OK:
            return {};

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

        case CURLE_OUT_OF_MEMORY:
            return {std::bad_alloc{}};

        case CURLE_OPERATION_TIMEDOUT:
            return {code::timedout};

        case CURLE_BAD_FUNCTION_ARGUMENT:
            return std::invalid_argument{"A function was called with a bad parameter."};

        case CURLE_RECURSIVE_API_CALL:
            return {code::recursive_api_call};

        default:
            return {Exception{code}};

        case CURLE_HTTP2:
        case CURLE_SSL_CONNECT_ERROR:
        case CURLE_UNKNOWN_OPTION:
        case CURLE_HTTP3:
            return {ProtocolInternal_error{code, error_buffer}};
    }
}

long Easy_t::get_response_code() const noexcept
{
    long response_code;
    curl_easy_getinfo(curl_easy, CURLINFO_RESPONSE_CODE, &response_code);
    return response_code;
}

std::size_t Easy_t::getinfo_sizeof_uploaded() const noexcept
{
    curl_off_t ul;
    curl_easy_getinfo(curl_easy, CURLINFO_SIZE_UPLOAD_T, &ul);
    return ul;
}
std::size_t Easy_t::getinfo_sizeof_response_header() const noexcept
{
    long size;
    curl_easy_getinfo(curl_easy, CURLINFO_HEADER_SIZE, &size);
    return size;
}
std::size_t Easy_t::getinfo_sizeof_response_body() const noexcept
{
    curl_off_t dl;
    curl_easy_getinfo(curl_easy, CURLINFO_SIZE_DOWNLOAD_T, &dl);
    return dl;
}
std::size_t Easy_t::getinfo_transfer_time() const noexcept
{
    curl_off_t total;
    curl_easy_getinfo(curl_easy, CURLINFO_TOTAL_TIME_T, &total);
    return total;
}

Easy_t::~Easy_t()
{
    if (curl_easy)
        curl_easy_cleanup(curl_easy);
}

auto Easy_t::readall(std::string &response) -> perform_ret_t
{
    writeback = [](char *buffer, std::size_t size, void *data) {
        std::string &response = *static_cast<std::string*>(data);
        response.append(buffer, buffer + size);
        return size;
    };
    data = &response;

    return perform();
}
auto Easy_t::read(std::string &response) -> perform_ret_t
{
    writeback = [](char *buffer, std::size_t size, void *data) {
        std::string &response = *static_cast<std::string*>(data);

        auto str_size = response.size();
        auto str_cap = response.capacity();
        if (str_size < str_cap)
            response.append(buffer, buffer + std::min(size, str_cap - str_size));

        return size;
    };
    data = &response;

    return perform();
}
auto Easy_t::establish_connection_only() noexcept -> perform_ret_t
{
    request_get();

    curl_easy_setopt(curl_easy, CURLOPT_NOBODY, 1);
    auto ret = perform();
    curl_easy_setopt(curl_easy, CURLOPT_NOBODY, 0);

    return std::move(ret);
}
} /* namespace curl */
