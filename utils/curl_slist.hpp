#ifndef  __curl_cpp_utils_curl_slist_HPP__
# define __curl_cpp_utils_curl_slist_HPP__

# include <cstddef>
# include <memory>
# include <iterator>

# include <curl/curl.h>

# include "../return-exception/ret-exception.hpp"

namespace curl::utils {
class slist {
    void *list = nullptr;

    template <class Pointer, class value_t>
    struct Iterator_template {
        Pointer ptr = nullptr;

        using value_type = value_t;
        using pointer = value_type*;
        using reference = value_type&;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        auto& operator ++ () noexcept
        {
            ptr = ptr->next;
        }
        auto operator ++ (int) noexcept
        {
            auto ret = *this;
            ++(*this);
            return ret;
        }

        auto operator * () const noexcept -> reference
        {
            return ptr->data;
        }

        bool operator == (const Iterator_template &other) const noexcept
        {
            return ptr == other.ptr;
        }

        bool operator != (const Iterator_template &other) const noexcept
        {
            return ptr != other.ptr;
        }
    };

public:
    using value_type = char* const;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using reference = value_type&;
    using const_reference = const value_type&;

    using pointer = value_type*;
    using const_pointer = const value_type*;

    using iterator = Iterator_template<curl_slist*, char * const>;
    using const_iterator = Iterator_template<const curl_slist*, const char * const>;

    slist() = default;

    slist(curl_slist *l) noexcept;

    slist(const slist&) = delete;
    slist(slist &&other) noexcept;

    slist& operator = (const slist&) = delete;
    slist& operator = (slist &&other) noexcept;

    void swap(slist &other) noexcept;
    void clear() noexcept;

    ~slist();

    bool is_empty() const noexcept;

    auto begin() noexcept -> iterator;
    auto end() noexcept -> iterator;

    auto begin() const noexcept -> const_iterator;
    auto end() const noexcept -> const_iterator;

    auto cbegin() const noexcept -> const_iterator;
    auto cend() const noexcept -> const_iterator;

    auto get_underlying_ptr() const noexcept -> void*;

    /**
     * @param str is copied before adding to the list.
     *            Must not be CRLF-terminated for use in curl::Easy_ref_t::set_http_header.
     */
    auto push_back(const char *str) noexcept -> Ret_except<void, std::bad_alloc>;
};
} /* namespace curl::utils */

#endif
