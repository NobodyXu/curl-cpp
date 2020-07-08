#ifndef  __curl_test_utility_HPP__
# define __curl_test_utility_HPP__

# include <cstdlib>
# include <iostream>
# include <string_view>

template <class T>
void assert_same_impl(const T &v1, const char *expr1, const T &v2, const char *expr2) noexcept
{
    if (v1 != v2) {
        std::cerr << expr1 << " != " << expr2 << ": " 
                  << expr1 << " = " << v1 << ", " << expr2 << " = " << v2 << std::endl;
        std::exit(1);
    }
}

#define assert_same(expr1, expr2) assert_same_impl((expr1), # expr1, (expr2), # expr2)

#endif
