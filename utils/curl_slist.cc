#include "curl_slist.hpp"
#include <curl/curl.h>
#include <utility>

namespace curl::utils {
slist::slist(void *l) noexcept:
    list{l}
{}
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
}
void slist::clear() noexcept
{
    if (list)
        curl_slist_free_all(static_cast<struct curl_slist*>(list));
    list = nullptr;
}

slist::~slist()
{
    if (list)
        curl_slist_free_all(static_cast<struct curl_slist*>(list));
}

auto slist::const_iterator::operator ++ () noexcept -> const_iterator&
{
    list_ptr = static_cast<const curl_slist*>(list_ptr)->next;
    return *this;
}
auto slist::const_iterator::operator ++ (int) noexcept -> const_iterator
{
    auto ret = *this;
    ++(*this);
    return ret;
}
auto slist::const_iterator::operator * () const noexcept -> value_type
{
    return static_cast<const curl_slist*>(list_ptr)->data;
}
bool operator == (const slist::const_iterator &x, const slist::const_iterator &y) noexcept
{
    return x.list_ptr == y.list_ptr;
}
bool operator != (const slist::const_iterator &x, const slist::const_iterator &y) noexcept
{
    return x.list_ptr != y.list_ptr;
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
    return list == nullptr;
}

auto slist::push_back(const char *str) noexcept -> Ret_except<void, std::bad_alloc>
{
    auto temp = curl_slist_append(static_cast<struct curl_slist*>(list), str);
    if (!temp)
        return {std::bad_alloc{}};

    list = temp;

    return {};
}
} /* namespace curl::utils */
