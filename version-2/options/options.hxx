#pragma once
#include <string>
#include <optional>

namespace ribomation::weatherdata {
    using std::string;
    using std::optional;

    struct Options {
        bool     download_only = false;
        string   config_file   = "weather-data.ini";
        unsigned year          = 1950;
        unsigned top           = 5;
        static auto parse(int argc, char** argv) -> optional<Options>;
    };

    auto operator<<(std::ostream& os, Options const& c) -> std::ostream&;
}
