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
    assert_same(url_ref.set_url("http://en.cppreference.com/").get_return_value(), 
                curl::Url_ref_t::set_code::ok);

    auto easy1 = curl.create_easy();
    assert(easy1);

    curl::Easy_ref_t easy_ref1{easy1.get()};

    easy_ref1.set_url(url_ref);

    easy_ref1.setup_establish_connection_only();
    easy_ref1.perform();

    easy_ref1.request_get();
    easy_ref1.set_writeback(reinterpret_cast<decltype(easy_ref1)::writeback_t>(std::fwrite), stdout);

    assert_same(easy_ref1.perform().get_return_value(), curl::Easy_ref_t::code::ok);
    assert_same(easy_ref1.get_response_code(), 302L);

    easy_ref1.set_url("http://en.cppreference.com/");
    assert_same(easy_ref1.perform().get_return_value(), curl::Easy_ref_t::code::ok);
    assert_same(easy_ref1.get_response_code(), 302L);

    auto easy2 = curl.dup_easy(easy1);
    assert(easy2);

    curl::Easy_ref_t easy_ref2{easy2.get()};

    //constexpr const char expected_response[] = "<!DOCTYPE HTML PUBLIC ";
    //constexpr const auto expected_len = sizeof(expected_response) - 1;

    //std::string response;
    //response.reserve(expected_len);

    //easy_ref2.set_read_writeback(std::pair{response, expected_len});
    //assert_same(easy_ref2.perform().get_return_value(), curl::Easy_ref_t::code::ok);
    //assert_same(easy_ref2.get_response_code(), 302L);

    //assert_same(response, std::string_view{expected_response});

    return 0;
}
