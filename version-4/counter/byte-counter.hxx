#pragma once
#include <cstdint>
#include <ios>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/operations.hpp>

namespace ribomation::weatherdata {
    namespace io = boost::iostreams;

    // Counts the bytes passing through, like io::counter but with 64 bits
    class byte_counter : public io::multichar_input_filter {
        std::uint64_t count_ = 0;

    public:
        template<typename Source>
        auto read(Source& src, char* buffer, std::streamsize n) -> std::streamsize {
            auto result = io::read(src, buffer, n);
            if (result > 0) count_ += static_cast<std::uint64_t>(result);
            return result; // -1 at EOF, passed on unchanged
        }

        [[nodiscard]] auto count() const { return count_; }
    };
}
