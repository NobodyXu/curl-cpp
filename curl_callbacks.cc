#include "curl.hpp"

namespace curl {
std::size_t Easy_t::write_callback(char *buffer, std::size_t _, std::size_t size, void *arg) noexcept
{
    auto &handle = *static_cast<Easy_t*>(arg);
    if (handle.writeback) {
        try {
            return handle.writeback(buffer, size, handle.data, handle.writeback_exception_thrown);
        } catch (...) {
            handle.writeback_exception_thrown = std::current_exception();
            return 0;
        }
    } else
        return size;
}
} /* namespace curl */
