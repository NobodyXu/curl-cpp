#include "../curl_easy.hpp"
#include "../curl_url.hpp"
#include "../curl_multi.hpp"

#include <cassert>
#include "utility.hpp"

#include <err.h>
#include <cerrno>
#include <cinttypes>
#include <event2/event.h>

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

int main(int argc, char* argv[])
{
    curl::curl_t curl{nullptr};
    assert(curl.has_http2_multiplex_support());
    assert(curl.has_max_concurrent_stream_support());
    assert(curl.has_multi_socket_support());

    auto multi = curl.create_multi().get_return_value();
    multi.set_multiplexing(30);

    struct event_base *base = event_base_new();
    assert(base);
    struct event *timeout = evtimer_new(base, [](evutil_socket_t fd, short events, void *arg) noexcept
    {
        auto &multi = *static_cast<curl::Multi_t*>(arg);
        multi.multi_socket_action(CURL_SOCKET_TIMEOUT, 0, perform_callback, nullptr);
    }, &multi);

    using Arg = std::pair<struct event_base*, curl::Multi_t&>;
    Arg arg{base, multi};

    multi.register_callback([](CURL *curl_easy, 
                               curl_socket_t s,
                               int what,
                               void *userp,
                               void *per_socketp) noexcept
    {
        using curl_context_t = std::tuple<struct event*, curl_socket_t, curl::Multi_t&>;
        curl_context_t *curl_context;
        int events = 0;

        auto &arg = *static_cast<Arg*>(userp);
        auto &multi = arg.second;
        auto *base = arg.first;

        //auto *loop = static_cast<uv_loop_t*>(userp);

        switch(what) {
            case CURL_POLL_IN:
                events = EV_READ | EV_PERSIST;
                break;

            case CURL_POLL_OUT:
                events = EV_WRITE | EV_PERSIST;
                break;

            case CURL_POLL_INOUT:
                events = EV_READ | EV_WRITE | EV_PERSIST;
                break;

            case CURL_POLL_REMOVE:
                if(per_socketp) {
                    curl_context = static_cast<curl_context_t*>(per_socketp);

                    auto *event = std::get<0>(*curl_context);
                    event_del(event);
                    event_free(event);
                    delete curl_context;

                    multi.multi_assign(s, nullptr);
                }
                return 0;

            default:
                assert(false);
        }

        auto curl_perform = [](int fd, short event, void *arg) noexcept
        {
            auto &context = *static_cast<curl_context_t*>(arg);
            int flags = 0;

            if(event & EV_READ)
              flags |= CURL_CSELECT_IN;
            if(event & EV_WRITE)
              flags |= CURL_CSELECT_OUT;

            auto &multi = std::get<2>(context);
            auto sockfd = std::get<1>(context);
            multi.multi_socket_action(sockfd, flags, perform_callback, nullptr);
        };

        if (!per_socketp) {
            curl_context = new curl_context_t{{}, s, multi};
            std::get<0>(*curl_context) = event_new(base, s, events, curl_perform, curl_context);
            multi.multi_assign(s, curl_context);
        } else {
            curl_context = static_cast<curl_context_t*>(per_socketp);

            auto *event = std::get<0>(*curl_context);
            auto sockfd = std::get<1>(*curl_context);

            event_del(event);
            event_assign(event, base, sockfd, events, curl_perform, curl_context);
            event_add(event, nullptr);
        }

        return 0;
    }, &arg, [](CURLM *multi, long timeout_ms, void *userp) noexcept
    {
        auto *timeout = static_cast<struct event*>(userp);

        if(timeout_ms < 0) {
            evtimer_del(timeout);
        } else {
            if(timeout_ms == 0)
                timeout_ms = 1; /* 0 means directly call socket_action, but we'll do it
                                   in a bit */
            evtimer_del(timeout);
            struct timeval tv{
                .tv_sec = timeout_ms / 1000,
                .tv_usec = (timeout_ms % 1000) * 1000,
            };
            evtimer_add(timeout, &tv);
        }
 
        return 0;
    }, timeout);

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
    event_base_dispatch(base);
    assert_same(multi.get_number_of_handles(), 0);

    for (auto &[_, str]: pool) {
        assert_same(str, expected_response);
    }

    return 0;
}
