/**
 * Example/test for using Multi_t::multi_socket_action interface with epoll and vector.
 */

#include "../curl_easy.hpp"
#include "../curl_url.hpp"
#include "../curl_multi.hpp"

#include <cassert>
#include "utility.hpp"

#include <err.h>
#include <cerrno>
#include <cinttypes>
#include <sys/epoll.h>

using curl::Easy_ref_t;

static constexpr const auto connection_cnt = 20UL;
static constexpr const auto expected_response = "<p>Hello, world!\\n</p>\n";

int main(int argc, char* argv[])
{
    curl::curl_t curl{nullptr};
    assert(curl.has_http2_multiplex_support());
    assert(curl.has_max_concurrent_stream_support());
    assert(curl.has_multi_socket_support());

    auto multi = curl.create_multi().get_return_value();
    multi.set_multiplexing(30);

    int timeout; // in milliseconds (ms)
    auto epfd = epoll_create1(0);
    if (epfd == -1)
        err(1, "epoll_create1 failed");

    std::pair<curl::Multi_t&, int> args{multi, epfd};
    multi.register_callback([](CURL *curl_easy, 
                               curl_socket_t s,
                               int what,
                               void *userp,
                               void *per_socketp) noexcept
    {
        auto &args = *static_cast<std::pair<curl::Multi_t&, int>*>(userp);
        auto &multi = args.first;
        auto epfd = args.second;

        int op;
        if (!per_socketp)
            op = EPOLL_CTL_ADD;
        else
            op = EPOLL_CTL_MOD;

        struct epoll_event event;
        switch (what) {
            case CURL_POLL_IN:
                event.events = EPOLLIN;
                break;

            case CURL_POLL_OUT:
                event.events = EPOLLOUT;
                break;

            case CURL_POLL_INOUT:
                event.events = EPOLLIN | EPOLLOUT;
                break;

            case CURL_POLL_REMOVE:
                op = EPOLL_CTL_DEL;
        }

        int socketfd = static_cast<int>(s);
        event.data.fd = socketfd;
        if (epoll_ctl(epfd, op, socketfd, &event) == -1)
            err(1, "epoll_ctl(%d, %d, %d, event.events = %" PRIu32 ") failed", 
                    epfd, op, static_cast<int>(socketfd), event.events);

        if (what == CURL_POLL_REMOVE)
            multi.multi_assign(s, nullptr);
        else
            multi.multi_assign(s, curl_easy);

        return 0;
    }, &args, [](CURLM *multi, long timeout_ms, void *userp) noexcept
    {
        *static_cast<int*>(userp) = timeout_ms;
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

    constexpr const std::size_t maxevents = 20;
    struct epoll_event events[maxevents];

    auto perform_callback = [](Easy_ref_t &easy_ref, Easy_ref_t::perform_ret_t ret, 
                               curl::Multi_t &multi, void*) noexcept
    {
        assert_same(ret.get_return_value(), Easy_ref_t::code::ok);
        assert_same(easy_ref.get_response_code(), 200L);
        multi.remove_easy(easy_ref);
    };

    multi.multi_socket_action(CURL_SOCKET_TIMEOUT, 0, perform_callback, nullptr);
    while (multi.get_number_of_handles()) {
        auto ret = epoll_wait(epfd, events, maxevents, timeout);
        if (ret == -1) {
            if (errno == EINTR)
                multi.multi_socket_action(CURL_SOCKET_TIMEOUT, 0, perform_callback, nullptr);
            else
                err(1, "epoll_wait(%d, %p, %zu, %d) failed", epfd, events, maxevents, timeout);
        } else {
            for (std::size_t i = 0; i != ret; ++i) {
                int event = 0;
                if (events[i].events & EPOLLIN)
                    event |= CURL_CSELECT_IN;
                if (events[i].events & EPOLLOUT)
                    event |= CURL_CSELECT_OUT;
                if (events[i].events * EPOLLERR)
                    event |= CURL_CSELECT_ERR;
                multi.multi_socket_action(events[i].data.fd, event, perform_callback, nullptr);
            }
        }
    }

    for (auto &[_, str]: pool) {
        assert_same(str, expected_response);
    }

    return 0;
}
