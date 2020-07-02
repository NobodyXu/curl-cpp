#include "curl.hpp"
#include <curl/curl.h>
#include <cstring>

namespace curl {
curl_t::curl_t(FILE *debug_stream_arg):
    debug_stream{debug_stream_arg}
{
    auto code = curl_global_init(CURL_GLOBAL_ALL);
    if (code != CURLE_OK)
        throw Exception{curl_easy_strerror(code)};
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
