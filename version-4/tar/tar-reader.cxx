

#include "tar-reader.hxx"

namespace ribomation::weatherdata {
    std::streambuf::int_type ContentBuf::underflow() {
        if (unread_count == 0) return traits_type::eof();

        input.read(buf.data(), static_cast<std::streamsize>(std::min(buf.size(), unread_count)));
        auto n = static_cast<std::size_t>(input.gcount());
        if (n == 0) return traits_type::eof();

        unread_count -= n;
        setg(buf.data(), buf.data(), buf.data() + n);

        return traits_type::to_int_type(buf[0]);
    }

    auto TarReader::text(const Block& b, std::size_t offset, std::size_t len) -> std::string {
        auto field = std::string_view(b.data() + offset, len);
        return std::string(field.substr(0, field.find('\0')));
    }

    auto TarReader::octal(const Block& b, std::size_t offset, std::size_t len) -> std::size_t {
        auto field = text(b, offset, len);
        field.erase(field.find_last_not_of(' ') + 1);
        auto value = std::size_t{0};
        auto [end, ec] = std::from_chars(field.data(), field.data() + field.size(), value, 8);
        if (ec != std::errc{} || end != field.data() + field.size()) {
            throw std::runtime_error("tar: invalid size field '" + field + "'");
        }
        return value;
    }

    void TarReader::skip(std::size_t n) const {
        input.ignore(static_cast<std::streamsize>(n));    // works on non-seekable streams
        if (input.gcount() != static_cast<std::streamsize>(n)) {
            throw std::runtime_error("tar: unexpected end of archive");
        }
    }

    auto TarReader::next() -> std::optional<TarEntry> {
        content_buf.reset();
        content.clear();
        skip(unread + padding);
        unread = padding = 0;

        auto header = Block{};
        if (not input.read(header.data(), block_size)) {
            throw std::runtime_error("tar: unexpected end of archive");
        }
        if (std::ranges::all_of(header, [](char c) { return c == '\0'; })) {
            return std::nullopt;
        }

        auto entry = TarEntry{
            .name = text(header, 0, 100),
            .size = octal(header, 124, 12),
            .type = header[156],
        };
        if (entry.type == 'L' || entry.type == 'x' || entry.type == 'g') {
            throw std::runtime_error("tar: GNU long-name / pax extension headers are not supported");
        }
        if (text(header, 257, 5) == "ustar") {                 // ustar adds a path prefix
            if (auto prefix = text(header, 345, 155); !prefix.empty()) {
                entry.name = prefix + "/" + entry.name;
            }
        }

        unread  = entry.size;
        padding = (block_size - entry.size % block_size) % block_size;
        return entry;
    }
}
