#include "curl_share.hpp"
#include "curl_easy.hpp"
#include <type_traits>

namespace curl {
Share_base::Share_base(Ret_except<void, Exception> &e) noexcept:
    curl_share{curl_share_init()}
{
    if (!curl_share)
        e.set_exception<Exception>("curl_share_init() failed");
}

Share_base::~Share_base()
{
    curl_share_cleanup(curl_share);
}

void Share_base::add_lock(lock_function_t lock_func, unlock_function_t unlock_func, void *userptr) noexcept
{
    curl_share_setopt(curl_share, CURLSHOPT_LOCKFUNC, lock_func);
    curl_share_setopt(curl_share, CURLSHOPT_UNLOCKFUNC, unlock_func);
    curl_share_setopt(curl_share, CURLSHOPT_USERDATA, userptr);
}

void Share_base::enable_sharing(Options option) noexcept
{
    curl_share_setopt(curl_share, CURLSHOPT_SHARE, static_cast<curl_lock_data>(option));
}
void Share_base::disable_sharing(Options option) noexcept
{
    curl_share_setopt(curl_share, CURLSHOPT_UNSHARE, static_cast<curl_lock_data>(option));
}

void Share_base::add_easy(Easy_t &easy) noexcept
{
    curl_easy_setopt(easy.curl_easy, CURLOPT_SHARE, curl_share);
}
void Share_base::remove_easy(Easy_t &easy) noexcept
{
    curl_easy_setopt(easy.curl_easy, CURLOPT_SHARE, NULL);
}
} /* namespae curl */
