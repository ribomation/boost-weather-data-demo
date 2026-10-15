#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "options.hxx"

namespace po = boost::program_options;
using ribomation::weatherdata::Options;

namespace {
    using namespace std::string_literals;
    using std::string;
    using std::optional;

    auto cli(std::vector<std::string> args) -> optional<Options> {
        args.insert(args.begin(), "./weatherdata-test");

        auto argv = std::vector<char*>{};
        argv.reserve(args.size() + 1);
        for (auto& a: args) argv.push_back(a.data());

        return Options::parse(static_cast<int>(argv.size()), argv.data());
    }
}

struct stdout_buffer {
    std::ostringstream buf;
    std::streambuf* orig;
    stdout_buffer() {
        buf = std::ostringstream{};
        orig = std::cout.rdbuf(buf.rdbuf());
    }
    auto output() const {
        return buf.str();
    }
    ~stdout_buffer() {
        std::cout.rdbuf(orig);
    }
};

TEST_CASE("options") {
    SECTION("long names") {
        auto opts = cli({"--year", "1962", "--top=7", "--download"});
        REQUIRE(opts.has_value());
        CHECK(opts->year == 1962);
        CHECK(opts->top == 7);
        CHECK(opts->download_only == true);
        CHECK(opts->config_file == "weather-data.ini"s);
    }

    SECTION("short names") {
        auto opts = cli({"-y", "1962", "-t7", "-d"});
        REQUIRE(opts.has_value());
        CHECK(opts->year == 1962);
        CHECK(opts->top == 7);
        CHECK(opts->download_only == true);
        CHECK(opts->config_file == "weather-data.ini"s);
    }

    SECTION("help text") {
        using Catch::Matchers::ContainsSubstring;
        optional<Options> opts{};
        string result{};
        {
            auto b = stdout_buffer{};
            opts = cli({"--help"});
            result = b.output();
        }
        REQUIRE(not opts.has_value());
        CHECK_THAT(result, ContainsSubstring("show version and quit"));
    }
}
