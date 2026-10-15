#include "config.hxx"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

namespace ribomation::weatherdata {
    namespace pt = boost::property_tree;
    using namespace std::string_literals;
    using std::string;

    auto Config::parse_unit(string const& value) -> TemperatureUnit {
        if (value == "C") return TemperatureUnit::Celsius;
        if (value == "F") return TemperatureUnit::Fahrenheit;
        if (value == "K") return TemperatureUnit::Kelvin;
        throw std::invalid_argument{"unknown temperature unit: "s + value};
    }

    auto to_string(TemperatureUnit u) -> string {
        if (u == TemperatureUnit::Celsius) return "Celsius";
        if (u == TemperatureUnit::Fahrenheit) return "Fahrenheit";
        if (u == TemperatureUnit::Kelvin) return "Kelvin";
        throw std::invalid_argument{"unknown temperature unit: "s};
    }

    auto Config::load(fs::path const& cfg_file) -> Config {
        if (not fs::exists(cfg_file)) {
            throw std::invalid_argument{"cannot find configuration file "s
                + cfg_file.string()};
        }

        auto cfg = pt::ptree{};
        pt::read_ini(cfg_file, cfg);

        auto c      = Config{};
        c.url       = cfg.get<string>("source.url", c.url);
        c.year      = cfg.get<unsigned>("query.year", c.year);
        c.top       = cfg.get<unsigned>("query.top", c.top);
        c.min_days  = cfg.get<unsigned>("query.min_days", c.min_days);
        c.unit      = parse_unit(cfg.get<string>("output.unit", "C"));
        c.cache     = cfg.get<bool>("download.cache", c.cache);
        c.cache_dir = cfg.get<string>("download.cache_dir", c.cache_dir);

        return c;
    }

    auto operator<<(std::ostream& os, Config const& c) -> std::ostream& {
        os << "-- Config --\n"
                << " url=" << c.url << "\n"
                << " year=" << c.year << "\n"
                << " top=" << c.top << "\n"
                << " unit=" << to_string(c.unit) << "\n"
                << " cache=" << std::boolalpha << c.cache << "\n"
                << " dir=" << c.cache_dir.string() << "\n";
        return os;
    }
}
