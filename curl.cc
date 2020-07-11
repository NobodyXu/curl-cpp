#include "curl.hpp"
#include <curl/curl.h>

#include <err.h>
#include <cstring>
#include <cinttypes>
#include <limits>

namespace curl {
constexpr auto curl_t::Version::from(std::uint8_t major, std::uint8_t minor, std::uint8_t patch) noexcept -> 
    Version
{
    return {static_cast<std::uint32_t>(major << 16) | 
            static_cast<std::uint32_t>(minor << 8)  | 
            static_cast<std::uint32_t>(patch)};
}

static constexpr const auto mask = std::numeric_limits<std::uint8_t>::max();
constexpr std::uint8_t curl_t::Version::get_major() const noexcept
{
    return (num >> 16) & mask;
}
constexpr std::uint8_t curl_t::Version::get_minor() const noexcept
{
    return (num >> 8) & mask;
}
constexpr std::uint8_t curl_t::Version::get_patch() const noexcept
{
    return num & mask;
}

constexpr bool curl_t::Version::operator < (const Version &other) const noexcept
{
    return num < other.num;
}
constexpr bool curl_t::Version::operator <= (const Version &other) const noexcept
{
    return num <= other.num;
}
constexpr bool curl_t::Version::operator > (const Version &other) const noexcept
{
    return num > other.num;
}
constexpr bool curl_t::Version::operator >= (const Version &other) const noexcept
{
    return num >= other.num;
}
constexpr bool curl_t::Version::operator == (const Version &other) const noexcept
{
    return num == other.num;
}
constexpr bool curl_t::Version::operator != (const Version &other) const noexcept
{
    return num != other.num;
}

std::size_t curl_t::Version::to_string(char buffer[12]) const noexcept
{
    return std::snprintf(buffer, 12, "%" PRIu8 ".%" PRIu8 ".%" PRIu8, get_major(), get_minor(), get_patch());
}

static auto initialize_and_return_arg(FILE *stderr_stream_arg) noexcept
{
    auto code = curl_global_init(CURL_GLOBAL_ALL);
    if (code != CURLE_OK)
        errx(1, "curl_global_init(CURL_GLOBAL_ALL) failed with %s", curl_easy_strerror(code));

    return stderr_stream_arg;
}
curl_t::curl_t(FILE *stderr_stream_arg) noexcept:
    stderr_stream{initialize_and_return_arg(stderr_stream_arg)},
    version_info {curl_version_info(CURLVERSION_NOW)},
    version      {static_cast<const curl_version_info_data*>(version_info)->version_num},
    version_str  {static_cast<const curl_version_info_data*>(version_info)->version}
{
    if (version < Version::from(7, 4, 1))
        errx(1, "CURLINFO_RESPONSE_CODE isn't supported in this version: %s, %" PRIu32, 
                 version_str, version.num);
}

static auto initialize_and_return_last_arg(curl_t::malloc_callback_t  malloc_callback,
                                           curl_t::free_callback_t    free_callback,
                                           curl_t::realloc_callback_t realloc_callback,
                                           curl_t::strdup_callback_t  strdup_callback,
                                           curl_t::calloc_callback_t  calloc_callback,
                                           FILE *stderr_stream_arg) noexcept
{
    auto code = curl_global_init_mem(CURL_GLOBAL_ALL,
                                     malloc_callback,
                                     free_callback,
                                     realloc_callback,
                                     strdup_callback,
                                     calloc_callback);
    if (code != CURLE_OK)
        errx(1, "curl_global_init_mem(CURL_GLOBAL_ALL, %p, %p, %p, %p, %p) failed with %s", 
                 malloc_callback, 
                 free_callback, 
                 realloc_callback, 
                 strdup_callback, 
                 calloc_callback, 
                 curl_easy_strerror(code));

    return stderr_stream_arg;
}
curl_t::curl_t(FILE *stderr_stream_arg, 
               malloc_callback_t  malloc_callback,
               free_callback_t    free_callback,
               realloc_callback_t realloc_callback,
               strdup_callback_t  strdup_callback,
               calloc_callback_t  calloc_callback) noexcept:
    stderr_stream{initialize_and_return_last_arg(malloc_callback,
                                                 free_callback,
                                                 realloc_callback,
                                                 strdup_callback,
                                                 calloc_callback,
                                                 stderr_stream_arg)},
    version_info {curl_version_info(CURLVERSION_NOW)},
    version      {static_cast<const curl_version_info_data*>(version_info)->version_num},
    version_str  {static_cast<const curl_version_info_data*>(version_info)->version}
{
    if (version < Version::from(7, 4, 1))
        errx(1, "CURLINFO_RESPONSE_CODE isn't supported in this version: %s, %" PRIu32, 
                 version_str, version.num);
}


bool curl_t::has_compression_support() const noexcept
{
    auto *info = static_cast<const curl_version_info_data*>(version_info);
    return info->features & CURL_VERSION_LIBZ;
}
bool curl_t::has_largefile_support() const noexcept
{
    auto *info = static_cast<const curl_version_info_data*>(version_info);
    return info->features & CURL_VERSION_LARGEFILE;
}
bool curl_t::has_protocol(const char *protocol) const noexcept
{
    auto *info = static_cast<const curl_version_info_data*>(version_info);
    auto *protocols = info->protocols;

    for (std::size_t i = 0; protocols[i]; ++i)
        if (std::strcmp(protocols[i], protocol) == 0)
            return true;

    return false;
}
bool curl_t::has_ssl_support() const noexcept
{
    auto *info = static_cast<const curl_version_info_data*>(version_info);
    return info->features & CURL_VERSION_SSL;
}
bool curl_t::has_ipv6_support() const noexcept
{
    auto *info = static_cast<const curl_version_info_data*>(version_info);
    return info->features & CURL_VERSION_IPV6;
}

bool curl_t::has_erase_all_cookies_in_mem_support() const noexcept
{
    return version >= Version::from(7, 14, 1);
}
bool curl_t::has_erase_all_session_cookies_in_mem_support() const noexcept
{
    return version >= Version::from(7, 15, 4);
}

bool curl_t::has_disable_signal_handling_support() const noexcept
{
    return version >= Version::from(7, 10, 0);
}

bool curl_t::has_private_ptr_support() const noexcept
{
    return version >= Version::from(7, 10, 3);
}

bool curl_t::has_readfunc_abort_support() const noexcept
{
    return version >= Version::from(7, 12, 1);
}
bool curl_t::has_header_option_support() const noexcept
{
    return version >= Version::from(7, 37, 0);
}
bool curl_t::has_set_ip_addr_only_support() const noexcept
{
    return version >= Version::from(7, 24, 0);
}

bool curl_t::has_CURLU() const noexcept
{
    return version >= Version::from(7, 63, 0);
}

bool curl_t::has_sizeof_upload_support() const noexcept
{
    return version >= Version::from(7, 55, 0);
}
bool curl_t::has_sizeof_response_header_support() const noexcept
{
    return version >= Version::from(7, 4, 1);
}
bool curl_t::has_sizeof_response_body_support() const noexcept
{
    return version >= Version::from(7, 55, 0);
}
bool curl_t::has_transfer_time_support() const noexcept
{
    return version >= Version::from(7, 61, 0);
}
bool curl_t::has_redirect_url_support() const noexcept
{
    return version >= Version::from(7, 18, 2);
}

bool curl_t::has_buffer_size_tuning_support() const noexcept
{
    return version >= Version::from(7, 10, 0);
}
bool curl_t::has_buffer_size_growing_support() const noexcept
{
    return version >= Version::from(7, 53, 0);
}

bool curl_t::has_get_active_socket_support() const noexcept
{
    return version >= Version::from(7, 45, 0);
}

bool curl_t::has_multi_poll_support() const noexcept
{
    return version >= Version::from(7, 66, 0);
}
bool curl_t::has_multi_socket_support() const noexcept
{
    return version >= Version::from(7, 16, 0);
}

bool curl_t::has_http2_multiplex_support() const noexcept
{
    return version >= Version::from(7, 43, 0);
}
bool curl_t::has_max_concurrent_stream_support() const noexcept
{
    return version >= Version::from(7, 67, 0);
}


bool curl_t::has_ssl_session_sharing_support() const noexcept
{
    return version >= Version::from(7, 23, 0) && has_ssl_support();
}
bool curl_t::has_connection_cache_sharing_support() const noexcept
{
    return version >= Version::from(7, 57, 0);
}
bool curl_t::has_psl_sharing_support() const noexcept
{
    auto *info = static_cast<const curl_version_info_data*>(version_info);
    return version >= Version::from(7, 61, 0) && info->features & CURL_VERSION_PSL;
}

curl_t::~curl_t()
{
    curl_global_cleanup();
}
} /* namespace curl */
