/**
 * Example/test for using Multi_t::multi_socket_action interface and libuv without vector.
 */

#include "../curl_easy.hpp"
#include "../curl_url.hpp"
#include "../curl_multi.hpp"

#include <cassert>
#include "utility.hpp"

#include <err.h>
#include <cerrno>
#include <cinttypes>
#include <uv.h>

using curl::Easy_ref_t;

static constexpr const auto connection_cnt = 20UL;
static constexpr const auto expected_response = "<p>Hello, world!\\n</p>\n";

void perform_callback(Easy_ref_t &easy_ref, Easy_ref_t::perform_ret_t ret, 
                      curl::Multi_t &multi, void*) noexcept
{
    assert_same(ret.get_return_value(), Easy_ref_t::code::ok);
    assert_same(easy_ref.get_response_code(), 200L);
    multi.remove_easy(easy_ref);
}

curl::Multi_t *multi_p;
int main(int argc, char* argv[])
{
    curl::curl_t curl{nullptr};
    assert(curl.has_http2_multiplex_support());
    assert(curl.has_max_concurrent_stream_support());
    assert(curl.has_multi_socket_support());

    auto multi = curl.create_multi().get_return_value();
    multi.set_multiplexing(30);
    multi_p = &multi;

    uv_timer_t timeout;
    uv_loop_t *loop = uv_default_loop();
    assert(loop);
    uv_timer_init(loop, &timeout);

    multi.register_callback([](CURL *curl_easy, 
                               curl_socket_t s,
                               int what,
                               void *userp,
                               void *per_socketp) noexcept
    {
        using curl_context_t = std::pair<uv_poll_t, curl_socket_t>;
        curl_context_t *curl_context;
        int events = 0;

        auto *loop = static_cast<uv_loop_t*>(userp);

        switch(what) {
            case CURL_POLL_IN:
                events = UV_READABLE;
                break;

            case CURL_POLL_OUT:
                events = UV_WRITABLE;
                break;

            case CURL_POLL_INOUT:
                events = UV_READABLE | UV_WRITABLE;
                break;

            case CURL_POLL_REMOVE:
                if(per_socketp) {
                    curl_context = static_cast<curl_context_t *>(per_socketp);

                    uv_poll_stop(&curl_context->first);
                    uv_close(reinterpret_cast<uv_handle_t*>(&curl_context->first), 
                             [](uv_handle_t *handle) noexcept
                    {
                        auto *curl_context = static_cast<curl_context_t*>(handle->data);
                        delete curl_context;
                    });
                    multi_p->multi_assign(s, nullptr);
                }
                return 0;

            default:
                assert(false);
        }

        if (!per_socketp) {
            curl_context = new curl_context_t{{}, s};

            uv_poll_init_socket(loop, &curl_context->first, s);
            curl_context->first.data = curl_context;

            multi_p->multi_assign(s, curl_context);
        } else
            curl_context = static_cast<curl_context_t*>(per_socketp);

        uv_poll_start(&curl_context->first, events, 
                      [](uv_poll_t *req, int status, int events) noexcept
        {
            int flags = 0;
            if(events & UV_READABLE)
              flags |= CURL_CSELECT_IN;
            if(events & UV_WRITABLE)
              flags |= CURL_CSELECT_OUT;

            auto *context = static_cast<curl_context_t*>(req->data);

            multi_p->multi_socket_action(context->second, flags, perform_callback, nullptr);
        });
 
        return 0;
    }, loop, [](CURLM *multi, long timeout_ms, void *userp) noexcept
    {
        auto *timeout = static_cast<uv_timer_t*>(userp);
        if(timeout_ms < 0) {
            uv_timer_stop(timeout);
        } else {
            if(timeout_ms == 0)
                timeout_ms = 1; /* 0 means directly call socket_action, but we'll do it
                               in a bit */
            uv_timer_start(timeout, [](uv_timer_t*) noexcept
            {
                multi_p->multi_socket_action(CURL_SOCKET_TIMEOUT, 0, perform_callback, nullptr);
            }, timeout_ms, 0);
        }
        return 0;
    }, &timeout);

    std::vector<std::pair<curl::Easy_t, std::string>> pool;
    for (auto i = 0UL; i != connection_cnt; ++i) {
        auto easy = curl.create_easy();
        assert(easy);

        pool.emplace_back(std::move(easy), std::string());
    }

    for (auto &pair: pool) {
        auto easy_ref = Easy_ref_t{pair.first.get()};

        easy_ref.request_get();
        easy_ref.set_url("http://localhost:8787/");

        easy_ref.set_readall_writeback(pair.second);

        multi.add_easy(easy_ref);
    }

    assert_same(multi.get_number_of_handles(), connection_cnt);
    assert_same(uv_run(loop, UV_RUN_DEFAULT), 0);
    assert_same(multi.get_number_of_handles(), 0);

    for (auto &[_, str]: pool) {
        assert_same(str, expected_response);
    }

    return 0;
}
