#include "curl_easy.hpp"
#include "curl_url.hpp"

#include <new>
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
        return {nullptr};

    if (stderr_stream) {
        curl_easy_setopt(curl, CURLOPT_STDERR, stderr_stream);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }

    // Enable TCP_keepalive
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

    if (buffer_size)
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, buffer_size);

    if (disable_signal_handling_v)
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    using Easy_ptr = std::unique_ptr<char, Easy_deleter>;
    return {Easy_ptr{static_cast<char*>(curl)}};
}
auto curl_t::dup_easy(const Easy_t &e, std::size_t buffer_size) noexcept -> Easy_t
{
    CURL *curl = curl_easy_duphandle(e.get());
    if (!curl)
        return {nullptr};

    if (stderr_stream) {
        curl_easy_setopt(curl, CURLOPT_STDERR, stderr_stream);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }

    if (buffer_size)
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, buffer_size);

    if (disable_signal_handling_v)
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    using Easy_ptr = std::unique_ptr<char, Easy_deleter>;
    return {Easy_ptr{static_cast<char*>(curl)}};
}

void Easy_ref_t::set_verbose(FILE *stderr_stream_arg) noexcept
{
    if (stderr_stream_arg) {
        curl_easy_setopt(curl_easy, CURLOPT_STDERR, stderr_stream_arg);
        curl_easy_setopt(curl_easy, CURLOPT_VERBOSE, 1L);
    }
}
std::size_t Easy_ref_t::get_error_buffer_size() noexcept
{
    return CURL_ERROR_SIZE;
}
void Easy_ref_t::set_error_buffer(char *error_buffer) noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_ERRORBUFFER, error_buffer);
}

void Easy_ref_t::set_private(void *userp) noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_PRIVATE, userp);
}
void* Easy_ref_t::get_private() const noexcept
{
    char *userp;
    curl_easy_getinfo(curl_easy, CURLINFO_PRIVATE, &userp);
    return userp;
}

void Easy_ref_t::set_writeback(writeback_t writeback, void *userp) noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_WRITEFUNCTION, writeback);
    curl_easy_setopt(curl_easy, CURLOPT_WRITEDATA, userp);
}

void Easy_ref_t::set_url(const Url_ref_t &url) noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_CURLU, url.url);
}
auto Easy_ref_t::set_url(const char *url) noexcept -> Ret_except<void, std::bad_alloc>
{
    CHECK_OOM(curl_easy_setopt(curl_easy, CURLOPT_URL, url));
    return {};
}

auto Easy_ref_t::set_cookie(const char *cookies) noexcept -> 
    Ret_except<void, std::bad_alloc, curl::NotBuiltIn_error>
{
    auto code = curl_easy_setopt(curl_easy, CURLOPT_COOKIE, cookies);
    if (code == CURLE_UNKNOWN_OPTION)
        return curl::NotBuiltIn_error{"cookies not supported"};
    else if (code == CURLE_OUT_OF_MEMORY)
        return {std::bad_alloc{}};

    return {};
}

void Easy_ref_t::set_follow_location(long redir) noexcept
{
    if (redir != 0) {
        curl_easy_setopt(curl_easy, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_easy, CURLOPT_MAXREDIRS, redir);
    } else
        curl_easy_setopt(curl_easy, CURLOPT_FOLLOWLOCATION, 0L);
}

auto Easy_ref_t::set_useragent(const char *useragent) noexcept -> Ret_except<void, std::bad_alloc>
{
    CHECK_OOM(curl_easy_setopt(curl_easy, CURLOPT_USERAGENT, useragent));
    return {};
}
auto Easy_ref_t::set_encoding(const char *encoding) noexcept -> Ret_except<void, std::bad_alloc>
{
    CHECK_OOM(curl_easy_setopt(curl_easy, CURLOPT_ACCEPT_ENCODING, encoding));
    return {};
}

auto Easy_ref_t::set_interface(const char *value) noexcept -> Ret_except<void, std::bad_alloc>
{
    CHECK_OOM(curl_easy_setopt(curl_easy, CURLOPT_INTERFACE, value));
    return {};
}
auto Easy_ref_t::set_ip_addr_only(const char *ip_addr) noexcept -> Ret_except<void, std::bad_alloc>
{
    constexpr const auto buffer_size = 5 + INET6_ADDRSTRLEN + 1;
    char buffer[buffer_size];
    std::snprintf(buffer, buffer_size, "host!%s", ip_addr);

    return set_interface(buffer);
}

void Easy_ref_t::set_timeout(unsigned long timeout) noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_TIMEOUT_MS, timeout);
}

void Easy_ref_t::set_http_header(const utils::slist &l, header_option option) noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_HTTPHEADER, static_cast<struct curl_slist*>(l.get_underlying_ptr()));
    if (option != header_option::unspecified) {
        long value;
        if (option == header_option::separate)
            value = CURLHEADER_SEPARATE;
        else
            value = CURLHEADER_UNIFIED;

        curl_easy_setopt(curl_easy, CURLOPT_HEADEROPT, value);
    }
}

void Easy_ref_t::set_nobody(bool enable) noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_NOBODY, enable);
}

void Easy_ref_t::request_get() noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_HTTPGET, 1L);
}
void Easy_ref_t::request_post(const void *data, std::size_t len) noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_POSTFIELDSIZE_LARGE, len);
    curl_easy_setopt(curl_easy, CURLOPT_POSTFIELDS, data);
}
void Easy_ref_t::request_post(readback_t readback, void *userp, std::size_t len) noexcept
{
    request_post(nullptr, len);

    curl_easy_setopt(curl_easy, CURLOPT_READFUNCTION, readback);
    curl_easy_setopt(curl_easy, CURLOPT_READDATA, userp);
}

auto Easy_ref_t::perform() noexcept -> perform_ret_t
{
    return check_perform(curl_easy_perform(curl_easy), "curl::Easy_ref_t::perform");
}

long Easy_ref_t::get_response_code() const noexcept
{
    long response_code;
    curl_easy_getinfo(curl_easy, CURLINFO_RESPONSE_CODE, &response_code);
    return response_code;
}

std::size_t Easy_ref_t::getinfo_sizeof_uploaded() const noexcept
{
    curl_off_t ul;
    curl_easy_getinfo(curl_easy, CURLINFO_SIZE_UPLOAD_T, &ul);
    return ul;
}
std::size_t Easy_ref_t::getinfo_sizeof_response_header() const noexcept
{
    long size;
    curl_easy_getinfo(curl_easy, CURLINFO_HEADER_SIZE, &size);
    return size;
}
std::size_t Easy_ref_t::getinfo_sizeof_response_body() const noexcept
{
    curl_off_t dl;
    curl_easy_getinfo(curl_easy, CURLINFO_SIZE_DOWNLOAD_T, &dl);
    return dl;
}
std::size_t Easy_ref_t::getinfo_transfer_time() const noexcept
{
    curl_off_t total;
    curl_easy_getinfo(curl_easy, CURLINFO_TOTAL_TIME_T, &total);
    return total;
}
auto Easy_ref_t::getinfo_redirect_url() const noexcept -> const char*
{
    char *url = nullptr;
    curl_easy_getinfo(curl_easy, CURLINFO_REDIRECT_URL, &url);
    return url;
}

auto Easy_ref_t::get_active_socket() const noexcept -> curl_socket_t
{
    curl_socket_t socketfd = CURL_SOCKET_BAD;
    curl_easy_getinfo(curl_easy, CURLINFO_ACTIVESOCKET, &socketfd);
    return socketfd;
}

void Easy_ref_t::setup_establish_connection_only() noexcept
{
    request_get();
    curl_easy_setopt(curl_easy, CURLOPT_NOBODY, 1);
}
} /* namespace curl */
