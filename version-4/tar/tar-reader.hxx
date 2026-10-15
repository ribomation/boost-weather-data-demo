#pragma once
#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <istream>
#include <streambuf>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

// A minimal reader for *.tar archives. Boost has no tar support, but the
// format is simple: a sequence of 512-byte blocks, where each entry is a
// header block followed by the content, padded to a multiple of 512 bytes.
// The archive ends with (at least) two all-zero blocks.

namespace ribomation::weatherdata {
    using std::string;
    using std::string_view;

    struct TarEntry {
        std::string name;
        std::size_t size = 0;
        char type = '0'; // '0' or NUL = regular file, '5' = directory
        auto is_file() const -> bool {
            return type == '0' || type == '\0';
        }
    };

    /**
     * Exposes the content of the current entry as a stream that ends where the
     * entry ends, and keeps count of how much has been taken from the archive.
     */
    class ContentBuf : public std::streambuf {
        std::istream& input;
        std::size_t& unread_count;
        std::array<char, 4096> buf{};

    protected:
        auto underflow() -> int_type override;

    public:
        ContentBuf(std::istream& in, std::size_t& unread)
            : input(in), unread_count(unread) { }
        void reset() { setg(nullptr, nullptr, nullptr); }
    };

    class TarReader {
        static constexpr auto block_size = std::size_t{512};
        using Block = std::array<char, block_size>;

        std::istream& input;
        std::size_t   unread  = 0;     // bytes of the current entry's content not yet taken from in_
        std::size_t   padding = 0;     // padding after the current entry's content
        ContentBuf    content_buf{input, unread};
        std::istream  content{&content_buf};

        void skip(std::size_t n) const;
        static auto text(const Block& b, std::size_t offset, std::size_t len) -> std::string ;
        static auto octal(const Block& b, std::size_t offset, std::size_t len) -> std::size_t ;

    public:
        explicit TarReader(std::istream& in) : input(in) {}

        TarReader(const TarReader&) = delete;
        auto operator=(const TarReader&) -> TarReader& = delete;

        auto content_stream() -> std::istream& { return content; }
        auto next() -> std::optional<TarEntry> ;
    };

}
