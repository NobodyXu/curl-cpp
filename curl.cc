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
std::uint8_t curl_t::Version::get_major() const noexcept
{
    return (num >> 16) & mask;
}
std::uint8_t curl_t::Version::get_minor() const noexcept
{
    return (num >> 8) & mask;
}
std::uint8_t curl_t::Version::get_patch() const noexcept
{
    return num & mask;
}

bool curl_t::Version::operator < (const Version &other) const noexcept
{
    return num < other.num;
}
bool curl_t::Version::operator <= (const Version &other) const noexcept
{
    return num <= other.num;
}
bool curl_t::Version::operator > (const Version &other) const noexcept
{
    return num > other.num;
}
bool curl_t::Version::operator >= (const Version &other) const noexcept
{
    return num >= other.num;
}
bool curl_t::Version::operator == (const Version &other) const noexcept
{
    return num == other.num;
}
bool curl_t::Version::operator != (const Version &other) const noexcept
{
    return num != other.num;
}

std::size_t curl_t::Version::to_string(char buffer[12]) const noexcept
{
    return std::snprintf(buffer, 12, "%" PRIu8 ".%" PRIu8 ".%" PRIu8, get_major(), get_minor(), get_patch());
}

curl_t::curl_t(FILE *debug_stream_arg) noexcept:
    debug_stream{debug_stream_arg},
    version_info{curl_version_info(CURLVERSION_NOW)},
    version{static_cast<const curl_version_info_data*>(version_info)->version_num},
    version_str{static_cast<const curl_version_info_data*>(version_info)->version}
{
    if (version < Version::from(7, 10, 8))
        errx(1, "CURLINFO_RESPONSE_CODE isn't supported in this version: %s", version_str);

    auto code = curl_global_init(CURL_GLOBAL_ALL);
    if (code != CURLE_OK)
        errx(1, "curl_global_init(CURL_GLOBAL_ALL) failed with %s", curl_easy_strerror(code));
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

bool curl_t::has_sizeof_upload_support() const noexcept
{
    return version >= Version::from(7, 55, 0);
}
bool curl_t::has_sizeof_response_header_support() const noexcept
{
    return version >= Version::from(7, 4, 1);
}

curl_t::~curl_t()
{
    curl_global_cleanup();
}
} /* namespace curl */
