#include <string>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <stdexcept>
#include <boost/property_tree/ini_parser.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "config.hxx"

using Catch::Matchers::ContainsSubstring;
using ribomation::weatherdata::Config;
using ribomation::weatherdata::TemperatureUnit;

namespace {
    namespace fs = std::filesystem;
    using std::string;
    using namespace std::string_literals;

    class ConfigFile {
        fs::path path_;
    public:
        static auto filename_of(string const& name) -> fs::path {
            return fs::temp_directory_path() / ("weatherdata-"s + name);
        }

        explicit ConfigFile(const std::string& name, std::initializer_list<string> lines)
            : path_{filename_of(name)}
        {
            auto f = std::ofstream{path_};
            if (not f) throw std::invalid_argument{"cannot open for writing: " + path_.string()};
            for (auto&& line: lines) f << line << "\n";
        }

        ~ConfigFile() { std::filesystem::remove(path_); }

        ConfigFile(ConfigFile const&) = delete;
        auto operator=(ConfigFile const&) -> ConfigFile& = delete;

        [[nodiscard]] auto path() const -> std::string { return path_.string(); }
    };
}

TEST_CASE("reading some values from file", "[config]") {
    auto file = ConfigFile{"config.ini", {
        "[source]", "url = https://ribomation.se/whatever",
        "[output]", "unit = F"
    }};
    auto cfg = Config::load(file.path());
    //from file
    CHECK(cfg.url == "https://ribomation.se/whatever"s);
    CHECK(cfg.unit == TemperatureUnit::Fahrenheit);
    //from defaults
    CHECK(cfg.year == 1955);
}

TEST_CASE("missing file", "[config]") {
    auto file = ConfigFile::filename_of("missing.ini");
    CHECK_THROWS_WITH(Config::load(file), ContainsSubstring("cannot find configuration"));
}

TEST_CASE("invalid unit", "[config]") {
    auto file = ConfigFile{"invalid.ini", {
        "[output]", "unit = X"
    }};
    CHECK_THROWS_WITH(Config::load(file.path()), "unknown temperature unit: X");
}

TEST_CASE("malformed file", "[config]") {
    auto file = ConfigFile{"malformed.ini", {
        "[output", "unit = "
    }};
    CHECK_THROWS_AS(Config::load(file.path()), boost::property_tree::ini_parser_error);
}
