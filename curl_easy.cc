#include "curl_easy.hpp"
#include "curl_url.hpp"

#include <cstdlib>
#include <curl/curl.h>
#include <arpa/inet.h>
#include <utility>

#define CHECK_OOM(code)                \
    if ((code) == CURLE_OUT_OF_MEMORY) \
        return {std::bad_alloc{}}

namespace curl {
void curl_t::Easy_deleter::operator () (void *p) const noexcept
{
    if (p)
        curl_easy_cleanup(p);
}
auto curl_t::create_easy(std::size_t buffer_size) noexcept -> Easy_t
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return {nullptr, nullptr};

    if (debug_stream) {
        curl_easy_setopt(curl, CURLOPT_STDERR, debug_stream);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }

    // Enable TCP_keepalive
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, buffer_size);

    if (disable_signal_handling_v)
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    using Easy_ptr = std::unique_ptr<char, Easy_deleter>;
    using cstr_ptr = std::unique_ptr<char[]>;

    auto *error_buffer = static_cast<char*>(std::malloc(CURL_ERROR_SIZE));
    if (error_buffer) {
        error_buffer[0] = '\0';
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
    }

    return {Easy_ptr{static_cast<char*>(curl)}, cstr_ptr{error_buffer}};
}

void Easy_ref_t::set_writeback(writeback_t writeback, void *userp) noexcept
{
    curl_easy_setopt(ptrs.first, CURLOPT_WRITEFUNCTION, writeback);
    curl_easy_setopt(ptrs.first, CURLOPT_WRITEDATA, userp);
}

void Easy_ref_t::set_url(const Url_ref_t &url) noexcept
{
    curl_easy_setopt(ptrs.first, CURLOPT_CURLU, url.url);
}
auto Easy_ref_t::set_url(const char *url) noexcept -> Ret_except<void, std::bad_alloc>
{
    CHECK_OOM(curl_easy_setopt(ptrs.first, CURLOPT_URL, url));
    return {};
}

auto Easy_ref_t::set_useragent(const char *useragent) noexcept -> Ret_except<void, std::bad_alloc>
{
    CHECK_OOM(curl_easy_setopt(ptrs.first, CURLOPT_USERAGENT, useragent));
    return {};
}
auto Easy_ref_t::set_encoding(const char *encoding) noexcept -> Ret_except<void, std::bad_alloc>
{
    CHECK_OOM(curl_easy_setopt(ptrs.first, CURLOPT_ACCEPT_ENCODING, encoding));
    return {};
}

auto Easy_ref_t::set_source_ip(const char *ip_addr) noexcept -> Ret_except<void, std::bad_alloc>
{
    CHECK_OOM(curl_easy_setopt(ptrs.first, CURLOPT_INTERFACE, ip_addr));
    return {};
}

void Easy_ref_t::set_timeout(unsigned long timeout) noexcept
{
    curl_easy_setopt(ptrs.first, CURLOPT_TIMEOUT_MS, timeout);
}

void Easy_ref_t::set_http_header(const utils::slist &l, header_option option) noexcept
{
    curl_easy_setopt(ptrs.first, CURLOPT_HTTPHEADER, static_cast<struct curl_slist*>(l.get_underlying_ptr()));
    if (option != header_option::unspecified) {
        long value;
        if (option == header_option::separate)
            value = CURLHEADER_SEPARATE;
        else
            value = CURLHEADER_UNIFIED;

        curl_easy_setopt(ptrs.first, CURLOPT_HEADEROPT, value);
    }
}

void Easy_ref_t::request_get() noexcept
{
    curl_easy_setopt(ptrs.first, CURLOPT_HTTPGET, 1L);
}
void Easy_ref_t::request_post(const void *data, std::size_t len) noexcept
{
    curl_easy_setopt(ptrs.first, CURLOPT_POSTFIELDSIZE_LARGE, len);
    curl_easy_setopt(ptrs.first, CURLOPT_POSTFIELDS, data);
}
void Easy_ref_t::request_post(readback_t readback, void *userp, std::size_t len) noexcept
{
    request_post(nullptr, len);

    curl_easy_setopt(ptrs.first, CURLOPT_READFUNCTION, readback);
    curl_easy_setopt(ptrs.first, CURLOPT_READDATA, userp);
}

auto Easy_ref_t::perform() noexcept -> perform_ret_t
{
    return check_perform(curl_easy_perform(ptrs.first), "curl::Easy_ref_t::perform");
}

long Easy_ref_t::get_response_code() const noexcept
{
    long response_code;
    curl_easy_getinfo(ptrs.first, CURLINFO_RESPONSE_CODE, &response_code);
    return response_code;
}

std::size_t Easy_ref_t::getinfo_sizeof_uploaded() const noexcept
{
    curl_off_t ul;
    curl_easy_getinfo(ptrs.first, CURLINFO_SIZE_UPLOAD_T, &ul);
    return ul;
}
std::size_t Easy_ref_t::getinfo_sizeof_response_header() const noexcept
{
    long size;
    curl_easy_getinfo(ptrs.first, CURLINFO_HEADER_SIZE, &size);
    return size;
}
std::size_t Easy_ref_t::getinfo_sizeof_response_body() const noexcept
{
    curl_off_t dl;
    curl_easy_getinfo(ptrs.first, CURLINFO_SIZE_DOWNLOAD_T, &dl);
    return dl;
}
std::size_t Easy_ref_t::getinfo_transfer_time() const noexcept
{
    curl_off_t total;
    curl_easy_getinfo(ptrs.first, CURLINFO_TOTAL_TIME_T, &total);
    return total;
}

auto Easy_ref_t::get_active_socket() const noexcept -> curl_socket_t
{
    curl_socket_t socketfd = CURL_SOCKET_BAD;

    curl_easy_getinfo(ptrs.first, CURLINFO_ACTIVESOCKET, &socketfd);;

    return socketfd;
}

auto Easy_ref_t::establish_connection_only() noexcept -> perform_ret_t
{
    request_get();

    curl_easy_setopt(ptrs.first, CURLOPT_NOBODY, 1);
    auto ret = perform();
    curl_easy_setopt(ptrs.first, CURLOPT_NOBODY, 0);

    return std::move(ret);
}
} /* namespace curl */
