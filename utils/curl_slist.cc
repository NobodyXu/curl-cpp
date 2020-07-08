#include "curl_slist.hpp"
#include <curl/curl.h>
#include <utility>

namespace curl::utils {
slist::slist(slist &&other) noexcept:
    slist{}
{
    (*this).swap(other);
}
slist& slist::operator = (slist &&other) noexcept
{
    clear();
    (*this).swap(other);

    return *this;
}

void slist::swap(slist &other) noexcept
{
    std::swap(list, other.list);
    std::swap(nodes, other.nodes);
}
void slist::clear() noexcept
{
    if (list)
        curl_slist_free_all(static_cast<struct curl_slist*>(list));
    list = nullptr;
    nodes = 0;
}

slist::~slist()
{
    if (list)
        curl_slist_free_all(static_cast<struct curl_slist*>(list));
}

auto slist::begin() noexcept -> iterator
{
    return {static_cast<struct curl_slist*>(list)};
}
auto slist::end() noexcept -> iterator
{
    return {};
}

auto slist::begin() const noexcept -> const_iterator
{
    return {static_cast<struct curl_slist*>(list)};
}
auto slist::end() const noexcept -> const_iterator
{
    return {};
}

auto slist::cbegin() const noexcept -> const_iterator
{
    return {static_cast<struct curl_slist*>(list)};
}
auto slist::cend() const noexcept -> const_iterator
{
    return {};
}

auto slist::get_underlying_ptr() const noexcept -> void*
{
    return list;
}

bool slist::is_empty() const noexcept
{
    return nodes == 0;
}
std::size_t slist::size() const noexcept
{
    return nodes;
}

auto slist::push_back(const char *str) noexcept -> Ret_except<void, std::bad_alloc>
{
    auto temp = curl_slist_append(static_cast<struct curl_slist*>(list), str);
    if (!temp)
        return {std::bad_alloc{}};

    list = temp;
    ++nodes;

    return {};
}
} /* namespace curl::utils */
