#include <format>
#include <iterator>
#include <sstream>
#include <string>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "tar-reader.hxx"

using ribomation::weatherdata::TarReader;
using ribomation::weatherdata::TarEntry;
using Catch::Matchers::ContainsSubstring;

namespace {
    using std::string;
    using namespace std::string_literals;

    constexpr auto block_size = std::size_t{512};

    void put(string& block, std::size_t offset, string const& value) {
        block.replace(offset, value.size(), value);
    }

    // builds a ustar header block, the same way as GNU tar --format=ustar
    auto header(string const& name, std::size_t size, char type = '0',
                string const& prefix = "", bool ustar = true) -> string {
        auto block = string(block_size, '\0');
        put(block, 0, name);
        put(block, 100, "0000644"s);
        put(block, 108, "0001750"s);
        put(block, 116, "0001750"s);
        put(block, 124, std::format("{:011o}", size));
        put(block, 136, "15071457000"s);
        block[156] = type;
        if (ustar) {
            put(block, 257, "ustar"s);
            put(block, 263, "00"s);
            put(block, 345, prefix);
        }
        put(block, 148, string(8, ' '));
        auto sum = 0U;
        for (auto c : block) sum += static_cast<unsigned char>(c);
        put(block, 148, std::format("{:06o}", sum) + '\0');
        return block;
    }

    auto padding(std::size_t size) -> string {
        return string((block_size - size % block_size) % block_size, '\0');
    }

    auto file(string const& name, string const& content) -> string {
        return header(name, content.size()) + content + padding(content.size());
    }

    auto end_of_archive() -> string {
        return string(2 * block_size, '\0');
    }

    auto read_all(std::istream& in) -> string {
        return string{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
    }
}

TEST_CASE("tar-reader") {
    SECTION("empty archive") {
        auto input = std::istringstream{end_of_archive()};
        auto tar = TarReader{input};
        CHECK_FALSE(tar.next().has_value());
    }

    SECTION("single file") {
        auto input = std::istringstream{file("1945/01001099999.csv", "hello tar\n") + end_of_archive()};
        auto tar = TarReader{input};

        auto entry = tar.next();
        REQUIRE(entry.has_value());
        CHECK(entry->name == "1945/01001099999.csv");
        CHECK(entry->size == 10);
        CHECK(entry->type == '0');
        CHECK(entry->is_file());
        CHECK(read_all(tar.content_stream()) == "hello tar\n");

        CHECK_FALSE(tar.next().has_value());
    }

    SECTION("content stream ends where the entry ends") {
        auto input = std::istringstream{file("a.csv", "AAA") + file("b.csv", "BBB") + end_of_archive()};
        auto tar = TarReader{input};
        REQUIRE(tar.next().has_value());
        CHECK(read_all(tar.content_stream()) == "AAA");
        CHECK(tar.content_stream().get() == std::istream::traits_type::eof());
        CHECK(tar.content_stream().eof());
    }

    SECTION("several files, line by line") {
        auto input = std::istringstream{
            file("a.csv", "a1\na2\n") + file("b.csv", "b1\nb2\nb3\n") + end_of_archive()};
        auto tar = TarReader{input};

        auto names = std::vector<string>{};
        auto lines = std::vector<string>{};
        while (auto entry = tar.next()) {
            names.push_back(entry->name);
            for (auto line = string{}; std::getline(tar.content_stream(), line);) {
                lines.push_back(line);
            }
        }
        CHECK(names == std::vector<string>{"a.csv", "b.csv"});
        CHECK(lines == std::vector<string>{"a1", "a2", "b1", "b2", "b3"});
    }

    SECTION("unread content is skipped") {
        auto input = std::istringstream{
            file("a.csv", string(1000, 'a')) + file("b.csv", "BBB") + end_of_archive()};
        auto tar = TarReader{input};
        REQUIRE(tar.next()->name == "a.csv");
        auto entry = tar.next();
        REQUIRE(entry.has_value());
        CHECK(entry->name == "b.csv");
        CHECK(read_all(tar.content_stream()) == "BBB");
    }

    SECTION("partly read content is skipped") {
        auto input = std::istringstream{
            file("a.csv", "first\n" + string(10'000, 'a')) + file("b.csv", "BBB") + end_of_archive()};
        auto tar = TarReader{input};
        REQUIRE(tar.next().has_value());
        auto line = string{};
        REQUIRE(std::getline(tar.content_stream(), line));
        CHECK(line == "first");

        auto entry = tar.next();
        REQUIRE(entry.has_value());
        CHECK(entry->name == "b.csv");
        CHECK(read_all(tar.content_stream()) == "BBB");
    }

    SECTION("content larger than the internal buffer") {
        auto content = string{};
        for (auto k = 0; k < 2000; ++k) content += std::format("line {}\n", k);
        auto input = std::istringstream{file("big.csv", content) + end_of_archive()};
        auto tar = TarReader{input};
        REQUIRE(tar.next()->size == content.size());
        CHECK(read_all(tar.content_stream()) == content);
    }

    SECTION("size is an exact multiple of the block size") {
        auto content = string(block_size, 'x');
        auto input = std::istringstream{file("a.csv", content) + file("b.csv", "BBB") + end_of_archive()};
        auto tar = TarReader{input};
        REQUIRE(tar.next().has_value());
        CHECK(read_all(tar.content_stream()) == content);
        CHECK(tar.next()->name == "b.csv");
    }

    SECTION("empty file") {
        auto input = std::istringstream{file("empty.csv", "") + file("b.csv", "BBB") + end_of_archive()};
        auto tar = TarReader{input};
        auto entry = tar.next();
        REQUIRE(entry.has_value());
        CHECK(entry->size == 0);
        CHECK(read_all(tar.content_stream()).empty());
        CHECK(tar.next()->name == "b.csv");
    }

    SECTION("directory entry") {
        auto input = std::istringstream{header("1945/", 0, '5') + file("1945/a.csv", "A") + end_of_archive()};
        auto tar = TarReader{input};
        auto dir = tar.next();
        REQUIRE(dir.has_value());
        CHECK(dir->name == "1945/");
        CHECK(dir->type == '5');
        CHECK_FALSE(dir->is_file());
        CHECK(tar.next()->is_file());
    }

    SECTION("old-style regular file type") {
        auto input = std::istringstream{header("a.csv", 0, '\0') + end_of_archive()};
        auto tar = TarReader{input};
        CHECK(tar.next()->is_file());
    }

    SECTION("ustar prefix is prepended to the name") {
        auto input = std::istringstream{header("a.csv", 0, '0', "some/long/path") + end_of_archive()};
        auto tar = TarReader{input};
        CHECK(tar.next()->name == "some/long/path/a.csv");
    }

    SECTION("prefix is ignored without ustar magic") {
        auto input = std::istringstream{header("a.csv", 0, '0', "", false) + end_of_archive()};
        auto tar = TarReader{input};
        CHECK(tar.next()->name == "a.csv");
    }

    SECTION("errors") {
        SECTION("empty input") {
            auto input = std::istringstream{""};
            auto tar = TarReader{input};
            CHECK_THROWS_WITH(tar.next(), "tar: unexpected end of archive");
        }

        SECTION("missing end-of-archive blocks") {
            auto input = std::istringstream{file("a.csv", "AAA")};
            auto tar = TarReader{input};
            REQUIRE(tar.next().has_value());
            CHECK_THROWS_WITH(tar.next(), "tar: unexpected end of archive");
        }

        SECTION("truncated content") {
            auto input = std::istringstream{header("a.csv", 1000) + string(100, 'a')};
            auto tar = TarReader{input};
            REQUIRE(tar.next().has_value());
            CHECK_THROWS_WITH(tar.next(), "tar: unexpected end of archive");
        }

        SECTION("invalid size field") {
            auto block = header("a.csv", 0);
            put(block, 124, "12x45678901"s);
            auto input = std::istringstream{block + end_of_archive()};
            auto tar = TarReader{input};
            CHECK_THROWS_WITH(tar.next(), ContainsSubstring("invalid size field '12x45678901'"));
        }

        SECTION("GNU long-name header") {
            auto input = std::istringstream{header("././@LongLink", 200, 'L') + end_of_archive()};
            auto tar = TarReader{input};
            CHECK_THROWS_WITH(tar.next(), ContainsSubstring("not supported"));
        }

        SECTION("pax extension header") {
            auto input = std::istringstream{header("PaxHeaders/a.csv", 30, 'x') + end_of_archive()};
            auto tar = TarReader{input};
            CHECK_THROWS_WITH(tar.next(), ContainsSubstring("not supported"));
        }
    }
}
