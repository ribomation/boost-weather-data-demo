#pragma once
#include <string>
#include <filesystem>

namespace ribomation::weatherdata {
    namespace fs = std::filesystem;

    enum class TemperatureUnit { Celsius, Fahrenheit, Kelvin };

    struct Config {
        std::string     url = "https://www.ncei.noaa.gov/data/global-summary-of-the-day/archive";
        unsigned        year = 1955;
        unsigned        top = 5;
        unsigned        min_days = 300;
        TemperatureUnit unit = TemperatureUnit::Celsius;
        bool            precipitation_mm = true;
        bool            cache = true;
        fs::path        cache_dir = "./cache";

        static auto load(fs::path const& cfg_file) -> Config;
        static auto parse_unit(std::string const& value) -> TemperatureUnit;
    };

    auto operator<<(std::ostream& os, Config const& c) -> std::ostream&;
}
