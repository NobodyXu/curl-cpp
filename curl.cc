#include "curl.hpp"
#include <curl/curl.h>

#include <err.h>
#include <cstring>
#include <limits>

namespace curl {
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

curl_t::curl_t(FILE *debug_stream_arg) noexcept:
    debug_stream{debug_stream_arg},
    version{curl_version_info(CURLVERSION_NOW)->version_num}
{
    auto code = curl_global_init(CURL_GLOBAL_ALL);
    if (code != CURLE_OK)
        errx(1, "curl_global_init(CURL_GLOBAL_ALL) failed with %s", curl_easy_strerror(code));
}

bool curl_t::has_compression_support() const noexcept
{
    auto *info = curl_version_info(CURLVERSION_NOW);
    return info->features & CURL_VERSION_LIBZ;
}
bool curl_t::has_largefile_support() const noexcept
{
    auto *info = curl_version_info(CURLVERSION_NOW);
    return info->features & CURL_VERSION_LARGEFILE;
}
bool curl_t::has_protocol(const char *protocol) const noexcept
{
    auto *protocols = curl_version_info(CURLVERSION_NOW)->protocols;

    for (std::size_t i = 0; protocols[i]; ++i)
        if (std::strcmp(protocols[i], protocol) == 0)
            return true;

    return false;
}

curl_t::~curl_t()
{
    curl_global_cleanup();
}
} /* namespace curl */
