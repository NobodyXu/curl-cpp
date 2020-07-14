#include "../curl.hpp"
#include "../curl.cc"

#include "utility.hpp"

using Version = curl::curl_t::Version;

template <std::uint8_t major, std::uint8_t minor, std::uint8_t patch>
constexpr void test_Version_from_and_get() noexcept
{
    constexpr auto ver1 = Version::from(major, minor, patch);
    static_assert(ver1.get_major() == major);
    static_assert(ver1.get_minor() == minor);
    static_assert(ver1.get_patch() == patch);
}

#define test_less_bigger_than(v1, v2) \
    static_assert(v1 < v2);           \
    static_assert(v2 > v1)

#define test_to_string(major, minor, patch) \
    do {                                    \
        char buffer[12];                    \
        char ver_s[] = # major "." # minor "." # patch; \
        assert_same((Version::from(major, minor, patch).to_string(buffer)), sizeof(ver_s) - 1); \
        assert_same(std::string_view{buffer}, std::string_view{ver_s}); \
    } while (0)

int main(int argc, char* argv[])
{
    test_Version_from_and_get<7, 10, 3>();
    test_Version_from_and_get<7, 12, 3>();
    test_Version_from_and_get<8, 12, 3>();
    test_Version_from_and_get<8, 12, 9>();
    test_Version_from_and_get<8, 200, 9>();

    test_less_bigger_than(Version::from(7, 10, 3), Version::from(7, 10, 4));
    test_less_bigger_than(Version::from(7, 10, 3), Version::from(7, 11, 0));
    test_less_bigger_than(Version::from(7, 10, 3), Version::from(8, 0, 0));
    test_less_bigger_than(Version::from(8, 0, 0), Version::from(8, 10, 3));

    static_assert(Version::from(7, 10, 3) <= Version::from(7, 10, 3));
    static_assert(Version::from(7, 10, 3) >= Version::from(7, 10, 3));
    static_assert(Version::from(7, 10, 3) == Version::from(7, 10, 3));
    static_assert(Version::from(7, 10, 4) != Version::from(7, 10, 3));

    test_to_string(7, 10, 3);
    test_to_string(7, 12, 3);
    test_to_string(0, 12, 3);

    return 0;
}
