#ifndef  __curl_cpp_curl_url_HPP__
# define __curl_cpp_curl_url_HPP__

# include "curl.hpp"

namespace curl {
/**
 * @Precondition for using this class: curl_t::has_CURLU()
 *
 * Why make Url_ref_t RAII-less?
 *
 * Since url can be used by multiple Easy_ref_t, users might want to
 * manage it using std::shared_ptr.
 *
 * It Url_ref_t is RAII-ed, then user would have to write
 * std::shared_ptr<Url_ref_t>, introducing another layer of indirection
 * while it doesn't have to.
 */
class Url_ref_t {
protected:
    static void check_url(int code);

public:
    /**
     * If url == nullptr, then calling any member function has 
     * undefined behavior.
     */
    char *url;

    enum class set_code {
        ok,
        /**
         * For url, it could be:
         *  - url is longer than 8000000 bytes (7MiB)
         *  - scheme too long (newest libcurl support up to 40 bytes)
         *  - url does not match standard syntax
         *  - lack necessary part, e.g. scheme, host
         *  - contain junk character <= 0x1f || == 0x7f
         */
        malform_input,
        bad_port_number,
        unsupported_scheme,
    };

    /**
     * @return all of set_code
     */
    auto set_url(const char *url_arg) noexcept -> Ret_except<set_code, std::bad_alloc>;

    /**
     * <scheme>://<user>:<password>@<host>:<port>/<path>;<params>?<query>#<fragment>
     */

    /**
     * @return only set_code::unsupported_scheme, malform_input, ok
     */
    auto set_scheme(const char *scheme) noexcept -> Ret_except<set_code, std::bad_alloc>;
    /**
     * @return only set_code::malform_input, ok
     */
    auto set_options(const char *options) noexcept -> Ret_except<set_code, std::bad_alloc>;
    /**
     * @return only set_code::malform_input, ok
     */
    auto set_query(const char *query) noexcept -> Ret_except<set_code, std::bad_alloc>;

    struct curl_delete {
        void operator () (char *p) const noexcept;
    };
    using string = std::unique_ptr<char, curl_delete>;

    /**
     * get_code::no_* can be returned by respective get_*() function.
     */
    enum class get_code {
        no_scheme, 
        no_user, 
        no_passwd, 
        no_options, 
        no_host, 
        no_port, 
        no_query, 
        no_fragment,
    };

    /**
     * @return no_scheme or no_host
     */
    auto get_url() const noexcept -> Ret_except<string, get_code, std::bad_alloc>;

    auto get_scheme() const noexcept -> Ret_except<string, get_code, std::bad_alloc>;
    auto get_options() const noexcept -> Ret_except<string, get_code, std::bad_alloc>;
    auto get_query() const noexcept -> Ret_except<string, get_code, std::bad_alloc>;
};
} /* namespace curl */

#endif
