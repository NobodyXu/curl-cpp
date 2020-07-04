#include "curl.hpp"

#include <curl/curl.h>
#include <arpa/inet.h>
#include <utility>

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

    curl_easy_setopt(curl_easy, CURLOPT_PRIVATE, this);

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
    curl_easy_setopt(curl_easy, CURLOPT_PRIVATE, this);
    curl_easy_setopt(curl_easy, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl_easy, CURLOPT_ERRORBUFFER, error_buffer);
}
Easy_t::Easy_t(Easy_t &&other) noexcept:
    curl_easy{other.curl_easy}
{
    other.curl_easy = nullptr;
    curl_easy_setopt(curl_easy, CURLOPT_PRIVATE, this);
    curl_easy_setopt(curl_easy, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl_easy, CURLOPT_ERRORBUFFER, error_buffer);
}
Easy_t& Easy_t::operator = (Easy_t &&other) noexcept
{
    if (curl_easy)
        curl_easy_cleanup(curl_easy);
    curl_easy = other.curl_easy;
    other.curl_easy = nullptr;

    writeback = other.writeback;
    data = other.data;

    curl_easy_setopt(curl_easy, CURLOPT_PRIVATE, this);
    curl_easy_setopt(curl_easy, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl_easy, CURLOPT_ERRORBUFFER, error_buffer);

    return *this;
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

void Easy_t::set_timeout(unsigned long timeout) noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_TIMEOUT_MS, timeout);
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

auto Easy_t::perform() noexcept -> perform_ret_t
{
    return check_perform(curl_easy_perform(curl_easy));
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
    writeback = [](char *buffer, std::size_t size, Data_t &data) {
        std::string &response = *static_cast<std::string*>(data.ptr);
        response.append(buffer, buffer + size);
        return size;
    };
    data.ptr = &response;

    return perform();
}
auto Easy_t::read(std::string &response) -> perform_ret_t
{
    writeback = [](char *buffer, std::size_t size, Data_t &data) {
        std::string &response = *static_cast<std::string*>(data.ptr);

        auto str_size = response.size();
        auto str_cap = response.capacity();
        if (str_size < str_cap)
            response.append(buffer, buffer + std::min(size, str_cap - str_size));

        return size;
    };
    data.ptr = &response;

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

Easy_t& Easy_t::get_easy(void *curl_easy) noexcept
{
    void *pointer;
    curl_easy_getinfo(curl_easy, CURLINFO_PRIVATE, &pointer);
    return *static_cast<Easy_t*>(pointer);
}
} /* namespace curl */
