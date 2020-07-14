#include "../curl_url.hpp"

#include "utility.hpp"
#include <cassert>

using namespace std::literals;
using curl::Url_ref_t;
using set_code = Url_ref_t::set_code;
using get_code = Url_ref_t::get_code;

static constexpr const auto malform_scheme = "1111111111111111111111111111111111111111111111111111111";

int main(int argc, char* argv[])
try {
    curl::curl_t curl{stderr};
    assert(curl.has_CURLU());

    auto url1 = curl.create_Url();
    assert(url1.get());

    auto url_ref1 = curl::Url_ref_t{url1.get()};

    assert_same(url_ref1.set_url("wwww.google.com").get_return_value(), set_code::malform_input);
    assert_same(url_ref1.set_url("").get_return_value(), set_code::malform_input);

    assert_same(url_ref1.set_url("https://localhost:99999999999999").get_return_value(), 
                set_code::bad_port_number);
    assert_same(url_ref1.set_url("random://localhost:99999999999999").get_return_value(), 
                set_code::unsupported_scheme);

    assert_same(url_ref1.set_url("http://wwww.google.com").get_return_value(), set_code::ok);
    assert_same(url_ref1.set_scheme("nonexistent").get_return_value(), set_code::unsupported_scheme);
    assert_same(url_ref1.set_scheme(malform_scheme).get_return_value(), set_code::malform_input);
    assert_same(url_ref1.set_scheme("https").get_return_value(), set_code::ok);

    auto url2 = curl.dup_Url(url1);
    assert(url2.get());

    auto url_ref2 = curl::Url_ref_t{url2.get()};

    assert_same(std::string_view{url_ref2.get_url().get_return_value().get()}, "https://wwww.google.com/"sv);
    assert_same(std::string_view{url_ref2.get_scheme().get_return_value().get()}, "https"sv);

    url_ref2.get_options().Catch([](get_code code) noexcept {
        assert_same(code, get_code::no_options);
    });
    url_ref2.get_query().Catch([](get_code code) noexcept {
        assert_same(code, get_code::no_query);
    });

    assert_same(url_ref2.set_query("a=b").get_return_value(), set_code::ok);
    assert_same(std::string_view{url_ref2.get_url().get_return_value().get()}, "https://wwww.google.com/?a=b"sv);

    assert_same(std::string_view{url_ref2.get_query().get_return_value().get()}, "a=b"sv);

    return 0;
} catch (get_code code) {
    std::cout << code << std::endl;
    std::exit(1);
}
