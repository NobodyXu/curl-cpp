#include "curl_share.hpp"
#include "curl_easy.hpp"

namespace curl {
void curl_t::Share_deleter::operator () (char *p) const noexcept
{
    if (p)
        curl_share_cleanup(p);
}
auto curl_t::create_share() noexcept -> Share_t
{
    return Share_t{static_cast<char*>(static_cast<void*>(curl_share_init()))};
}

Share_base::Share_base(curl_t::Share_t &&share) noexcept:
    curl_share{std::move(share)}
{}

Share_base::Share_base(Share_base &&other) noexcept:
    curl_share{std::move(other.curl_share)}
{}
Share_base& Share_base::operator = (Share_base &&other) noexcept
{
    curl_share = std::move(other.curl_share);
    return *this;
}

Share_base::operator bool () const noexcept
{
    return bool(curl_share);
}

void Share_base::add_lock(lock_function_t lock_func, unlock_function_t unlock_func, void *userptr) noexcept
{
    curl_share_setopt(curl_share.get(), CURLSHOPT_LOCKFUNC, lock_func);
    curl_share_setopt(curl_share.get(), CURLSHOPT_UNLOCKFUNC, unlock_func);
    curl_share_setopt(curl_share.get(), CURLSHOPT_USERDATA, userptr);
}

auto Share_base::enable_sharing(Options option) noexcept -> Ret_except<int, std::bad_alloc>
{
    auto code = curl_share_setopt(curl_share.get(), CURLSHOPT_SHARE, static_cast<curl_lock_data>(option));
    if (code == CURLSHE_NOMEM)
        return {std::bad_alloc{}};
    else if (code == CURLSHE_NOT_BUILT_IN)
        return {0};

    return {1};
}
void Share_base::disable_sharing(Options option) noexcept
{
    curl_share_setopt(curl_share.get(), CURLSHOPT_UNSHARE, static_cast<curl_lock_data>(option));
}

void Share_base::add_easy(Easy_ref_t &easy) noexcept
{
    curl_easy_setopt(easy.curl_easy, CURLOPT_SHARE, curl_share.get());
}
void Share_base::remove_easy(Easy_ref_t &easy) noexcept
{
    curl_easy_setopt(easy.curl_easy, CURLOPT_SHARE, NULL);
}
} /* namespae curl */
