#include "../curl_easy.hpp"
#include "../curl_url.hpp"

#include <cassert>
#include "utility.hpp"

int main(int argc, char* argv[])
{
    curl::curl_t curl{nullptr};
    assert(curl.has_CURLU());

    auto url = curl.create_Url();
    assert(url.get());

    auto url_ref = curl::Url_ref_t{url.get()};
    assert_same(url_ref.set_url("https://www.google.com").get_return_value(), curl::Url_ref_t::set_code::ok);

    auto easy1 = curl.create_easy();
    assert(easy1.p1.get());
    assert(easy1.p2.get());

    curl::Easy_ref_t easy_ref1 = easy1;

    easy_ref1.set_url(url_ref);
    easy_ref1.set_writeback(reinterpret_cast<decltype(easy_ref1)::writeback_t>(std::fwrite), stdout);

    easy_ref1.request_get();
    assert_same(easy_ref1.perform().get_return_value(), curl::Easy_ref_t::code::ok);
    assert_same(easy_ref1.get_response_code(), 200L);

    auto easy2 = curl.dup_easy(easy1);
    assert(easy2.p1.get());
    assert(easy2.p2.get());

    curl::Easy_ref_t easy_ref2{easy2};

    constexpr const char expected_response[] = "<!doctype html>";
    constexpr const auto expected_len = sizeof(expected_response) - 1;

    std::string response;
    response.reserve(expected_len);

    assert_same(easy_ref2.read(response, expected_len).get_return_value(), curl::Easy_ref_t::code::ok);
    assert_same(easy_ref2.get_response_code(), 200L);

    assert_same(response, std::string_view{expected_response});

    return 0;
}
