/**
 * Example/test for using Multi_t::poll interface without vector.
 */

#include "../curl_easy.hpp"
#include "../curl_url.hpp"
#include "../curl_multi.hpp"

#include <cassert>
#include "utility.hpp"

using curl::Easy_ref_t;

static constexpr const auto connection_cnt = 20UL;
static constexpr const auto expected_response = "<p>Hello, world!\\n</p>\n";

int main(int argc, char* argv[])
{
    curl::curl_t curl{nullptr};
    assert(curl.has_private_ptr_support());
    assert(curl.has_http2_multiplex_support());
    assert(curl.has_max_concurrent_stream_support());
    assert(curl.has_multi_poll_support());

    auto multi = curl.create_multi().get_return_value();
    multi.set_multiplexing(30);

    for (auto i = 0UL; i != connection_cnt; ++i) {
        auto easy = curl.create_easy();
        assert(easy);

        auto easy_ref = Easy_ref_t{easy.release()};

        easy_ref.request_get();
        easy_ref.set_url("http://localhost:8787/");

        auto *str = new std::string;
        easy_ref.set_readall_writeback(*str);
        easy_ref.set_private(str);

        multi.add_easy(easy_ref);
    }

    assert_same(multi.get_number_of_handles(), connection_cnt);

    do {
        multi.perform([](Easy_ref_t &easy_ref, Easy_ref_t::perform_ret_t ret, curl::Multi_t &multi, void*) noexcept
        {
            assert_same(ret.get_return_value(), Easy_ref_t::code::ok);
            assert_same(easy_ref.get_response_code(), 200L);

            multi.remove_easy(easy_ref);

            curl::Easy_t easy{easy_ref.curl_easy};

            auto *str = static_cast<std::string*>(easy_ref.get_private());
            assert_same(*str, expected_response);
            delete str;
        }, nullptr);
    } while (multi.break_or_poll().get_return_value() != -1);

    assert_same(multi.get_number_of_handles(), 0);

    return 0;
}
