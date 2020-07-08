#ifndef  __curl_cpp_utils_unique_ptr_pair_HPP__
# define __curl_cpp_utils_unique_ptr_pair_HPP__

# include <memory>
# include <utility>

namespace curl::util {
template <class T1, class Deleter1, class T2, class Deleter2 = std::default_delete<T2>>
struct unique_ptr_pair {
    std::unique_ptr<T1, Deleter1> p1;
    std::unique_ptr<T2, Deleter2> p2;
};
} /* namespace curl::util */

#endif
