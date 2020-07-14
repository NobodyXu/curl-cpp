#include "../curl_easy.hpp"
#include "../curl_url.hpp"
#include "../curl_share.hpp"

#include <cassert>
#include "utility.hpp"

auto create_share(curl::curl_t &curl) noexcept
{
    auto share = curl.create_share();
    assert(share);
    return curl::Share<>{std::move(share)};
}

using Options = curl::Share_base::Options;

int main(int argc, char* argv[])
{
    curl::curl_t curl{nullptr};

    auto easy1 = curl.create_easy();
    assert(easy1);

    curl::Easy_ref_t easy_ref{easy1.get()};

    auto share = create_share(curl);

    share.enable_multithreaded_share();

    assert_same(share.enable_sharing(Options::dns).get_return_value(), 1);
    assert_same(share.enable_sharing(Options::ssl_session).get_return_value(), 1);
    assert_same(share.enable_sharing(Options::connection_cache).get_return_value(), 1);
    assert_same(share.enable_sharing(Options::psl).get_return_value(), 1);

    share.add_easy(easy_ref);

    easy_ref.set_url("http://en.cppreference.com/");

    easy_ref.request_get();
    assert_same(easy_ref.perform().get_return_value(), curl::Easy_ref_t::code::ok);
    assert_same(easy_ref.get_response_code(), 302L);

    share.remove_easy(easy_ref);

    return 0;
}
