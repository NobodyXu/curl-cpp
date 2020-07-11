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
    assert(curl.has_http2_multiplex_support());
    assert(curl.has_max_concurrent_stream_support());
    assert(curl.has_multi_poll_support());

    auto multi = curl.create_multi().get_return_value();
    multi.set_multiplexing(30);

    std::vector<std::pair<curl::Easy_t, std::string>> pool;
    for (auto i = 0UL; i != connection_cnt; ++i) {
        auto easy = curl.create_easy();
        assert(easy.p1);
        assert(easy.p2);

        pool.emplace_back(std::move(easy), std::string());
    }

    for (auto &pair: pool) {
        auto easy_ref = Easy_ref_t{pair.first};

        easy_ref.request_get();
        easy_ref.set_url("http://localhost:8787/");

        easy_ref.set_readall_writeback(pair.second);

        multi.add_easy(easy_ref);
    }

    assert_same(multi.get_number_of_handles(), connection_cnt);

    do {
        multi.perform([](Easy_ref_t &easy_ref, Easy_ref_t::perform_ret_t ret, void*) noexcept
        {
            assert_same(ret.get_return_value(), Easy_ref_t::code::ok);
            assert_same(easy_ref.get_response_code(), 200L);
        }, nullptr);
    } while (multi.break_or_poll().get_return_value() != -1);

    assert_same(multi.get_number_of_handles(), 0);

    for (auto &[_, str]: pool) {
        assert_same(str, expected_response);
    }

    return 0;
}
