#include <array>
#include <cstdint>
#include <iterator>
#include <string>
#include <type_traits>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <catch2/catch_test_macros.hpp>
#include "byte-counter.hxx"

using ribomation::weatherdata::byte_counter;
namespace io = boost::iostreams;
using namespace std::string_literals;

namespace {
    using std::string;

    // array_source is a direct device (no read() member), so byte_counter::read()
    // cannot call it directly. A stream_buffer turns it into an ordinary source.
    using ArraySource = io::stream_buffer<io::array_source>;

    auto source_of(string const& s) -> ArraySource {
        return ArraySource{io::array_source{s.data(), s.size()}};
    }

    auto read_all(std::istream& in) -> string {
        return string{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
    }

    // reads src through the counter, in chunks of chunk_size, until EOF
    auto drain(byte_counter& counter, ArraySource& src, std::streamsize chunk_size) -> string {
        auto result = string{};
        auto buffer = string(static_cast<std::size_t>(chunk_size), '\0');
        for (std::streamsize n; (n = counter.read(src, buffer.data(), chunk_size)) > 0;) {
            result.append(buffer.data(), static_cast<std::size_t>(n));
        }
        return result;
    }

    auto sample_text() -> string {
        auto text = string{};
        for (auto k = 0; k < 500; ++k) {
            text += "\"03864099999\",\"1929-10-01\",\"  55.0\",\" 4\",\"  44.7\",\" 4\",\"1015.3\"\n";
        }
        return text;
    }
}

TEST_CASE("byte-counter") {
    SECTION("count is 64 bits") {
        STATIC_CHECK(std::is_same_v<decltype(byte_counter{}.count()), std::uint64_t>);
    }

    SECTION("a new counter is zero") {
        CHECK(byte_counter{}.count() == 0);
    }

    SECTION("array_source") {
        auto const text = "Hello from the byte counter"s;
        auto src = source_of(text);
        auto counter = byte_counter{};

        SECTION("count equals string length, and -1 at EOF") {
            auto buffer = std::array<char, 64>{};
            auto n = counter.read(src, buffer.data(), buffer.size());
            CHECK(n == static_cast<std::streamsize>(text.size()));
            CHECK(counter.count() == text.size());
            CHECK(string(buffer.data(), static_cast<std::size_t>(n)) == text);

            CHECK(counter.read(src, buffer.data(), buffer.size()) == -1);
            CHECK(counter.count() == text.size());     // EOF does not change the count
            CHECK(counter.read(src, buffer.data(), buffer.size()) == -1);   // and stays at EOF
        }

        SECTION("small chunks add up") {
            CHECK(drain(counter, src, 4) == text);
            CHECK(counter.count() == text.size());
        }

        SECTION("one byte at a time") {
            CHECK(drain(counter, src, 1) == text);
            CHECK(counter.count() == text.size());
        }

        SECTION("reading zero bytes counts nothing") {
            // io::read() on a streambuf reports -1 for any 0-byte read, so only check the count
            auto buffer = std::array<char, 8>{};
            counter.read(src, buffer.data(), 0);
            CHECK(counter.count() == 0);
            CHECK(drain(counter, src, 8) == text);     // nothing was consumed
        }
    }

    SECTION("empty source gives -1 at once") {
        auto const empty = ""s;
        auto src = source_of(empty);
        auto counter = byte_counter{};
        auto buffer = std::array<char, 8>{};
        CHECK(counter.read(src, buffer.data(), buffer.size()) == -1);
        CHECK(counter.count() == 0);
    }

    SECTION("binary data, incl. NUL bytes") {
        auto data = string{};
        for (auto k = 0; k < 256; ++k) data += static_cast<char>(k);
        auto src = source_of(data);
        auto counter = byte_counter{};
        CHECK(drain(counter, src, 100) == data);
        CHECK(counter.count() == 256);
    }

    SECTION("in a filtering_istream") {
        auto const text = sample_text();
        auto in = io::filtering_istream{};
        in.push(byte_counter{});
        in.push(io::array_source{text.data(), text.size()});

        CHECK(read_all(in) == text);
        // the stream holds a copy of the counter, so ask the stream for it
        REQUIRE(in.component<byte_counter>(0) != nullptr);
        CHECK(in.component<byte_counter>(0)->count() == text.size());
    }

    SECTION("pushing a counter copies it") {
        auto const text = "abc"s;
        auto counter = byte_counter{};
        auto in = io::filtering_istream{};
        in.push(counter);
        in.push(io::array_source{text.data(), text.size()});
        read_all(in);
        CHECK(counter.count() == 0);
        CHECK(in.component<byte_counter>(0)->count() == 3);
    }

    SECTION("around gzip, compress and decompress") {
        auto const original = sample_text();

        // [counter] <- gzip_compressor <- [counter] <- original
        auto compress = io::filtering_istream{};
        compress.push(byte_counter{});
        compress.push(io::gzip_compressor{});
        compress.push(byte_counter{});
        compress.push(io::array_source{original.data(), original.size()});
        auto const compressed = read_all(compress);

        auto const zipped_count = compress.component<byte_counter>(0)->count();
        auto const plain_count  = compress.component<byte_counter>(2)->count();
        CHECK(plain_count == original.size());
        CHECK(zipped_count == compressed.size());
        CHECK(zipped_count < plain_count);   // repetitive text squeezes well

        // [counter] <- gzip_decompressor <- [counter] <- compressed
        auto decompress = io::filtering_istream{};
        decompress.push(byte_counter{});
        decompress.push(io::gzip_decompressor{});
        decompress.push(byte_counter{});
        decompress.push(io::array_source{compressed.data(), compressed.size()});
        auto const restored = read_all(decompress);

        CHECK(restored == original);
        CHECK(decompress.component<byte_counter>(0)->count() == original.size());
        CHECK(decompress.component<byte_counter>(2)->count() == compressed.size());
    }

    SECTION("large input, bigger than the stream buffers") {
        auto const data = string(3 * 1024 * 1024 + 17, 'x');
        auto in = io::filtering_istream{};
        in.push(byte_counter{});
        in.push(io::array_source{data.data(), data.size()});
        CHECK(read_all(in).size() == data.size());
        CHECK(in.component<byte_counter>(0)->count() == data.size());
    }
}
