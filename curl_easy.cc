#include "curl.hpp"
#include <curl/curl.h>

#include <utility>
#include <algorithm>

#define CHECK(expr) check_easy((expr), #expr)

#define CURL_EASY_SETOPT(curl, opt, val)                    \
    ({                                                      \
        auto code = curl_easy_setopt((curl), (opt), (val)); \
        if (code == CURLE_UNKNOWN_OPTION)                   \
            return {NotSupported_error(# opt)};             \
        code;                                               \
    })
#define CHECK_OOM(code)                \
    if ((code) == CURLE_OUT_OF_MEMORY) \
            return {std::bad_alloc{}}

namespace curl {
auto curl_t::create_handle() noexcept -> Ret_except<handle_t, curl::Exception>
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

    return {std::in_place_type<handle_t>, curl};
}
handle_t::handle_t(void *curl) noexcept:
    curl_easy{curl}
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
handle_t::handle_t(const handle_t &other, Ret_except<void, curl::Exception> &e) noexcept:
    curl_easy{curl_easy_duphandle(other.curl_easy)},
    writeback{other.writeback},
    data{other.data}
{
    if (!curl_easy)
        e.set_exception<curl::Exception>("curl_easy_duphandle failed");
    curl_easy_setopt(curl_easy, CURLOPT_WRITEDATA, this);
}
handle_t::handle_t(handle_t &&other) noexcept:
    curl_easy{other.curl_easy}
{
    other.curl_easy = nullptr;
    curl_easy_setopt(curl_easy, CURLOPT_WRITEDATA, this);
}

auto handle_t::set(const Url &url, const char *useragent, const char *encoding) noexcept -> 
    Ret_except<void, NotSupported_error, std::bad_alloc>
{
    CURL_EASY_SETOPT(curl_easy, CURLOPT_CURLU, url.url);
    CHECK_OOM(CURL_EASY_SETOPT(curl_easy, CURLOPT_USERAGENT, useragent));
    CHECK_OOM(CURL_EASY_SETOPT(curl_easy, CURLOPT_ACCEPT_ENCODING, encoding));

    return {};
}

void handle_t::request_get() noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_HTTPGET, 1L);
}
void handle_t::request_post(const void *data, std::uint32_t len) noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_POSTFIELDSIZE, len);
    curl_easy_setopt(curl_easy, CURLOPT_POSTFIELDS, data);
}
void handle_t::request_post_large(const void *data, std::size_t len) noexcept
{
    curl_easy_setopt(curl_easy, CURLOPT_POSTFIELDSIZE_LARGE, len);
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
    check_easy(curl_easy_getinfo(curl_easy, CURLINFO_RESPONSE_CODE, &response_code), "CURLINFO_RESPONSE_CODE");
    return response_code;
}

std::size_t handle_t::getinfo_sizeof_uploaded() const
{
    curl_off_t ul;
    check_easy(curl_easy_getinfo(curl_easy, CURLINFO_SIZE_UPLOAD_T, &ul), "CURLINFO_SIZE_UPLOAD_T");
    return ul;
}
std::size_t handle_t::getinfo_sizeof_response_header() const
{
    long size;
    check_easy(curl_easy_getinfo(curl_easy, CURLINFO_HEADER_SIZE, &size), "CURLINFO_HEADER_SIZE");
    return size;
}
std::size_t handle_t::getinfo_sizeof_response_body() const
{
    curl_off_t dl;
    check_easy(curl_easy_getinfo(curl_easy, CURLINFO_SIZE_DOWNLOAD_T, &dl), "CURLINFO_SIZE_DOWNLOAD_T");
    return dl;
}
std::size_t handle_t::getinfo_transfer_time() const
{
    curl_off_t total;
    check_easy(curl_easy_getinfo(curl_easy, CURLINFO_TOTAL_TIME_T, &total), "CURLINFO_TOTAL_TIME_T");
    return total;
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
    request_get();

    curl_easy_setopt(curl_easy, CURLOPT_NOBODY, 1);
    perform();
    curl_easy_setopt(curl_easy, CURLOPT_NOBODY, 0);
}
} /* namespace curl */
