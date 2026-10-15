#include <sstream>
#include "observation.hxx"

namespace ribomation::weatherdata::conversions {
    using std::string;
    using std::optional;
    using std::chrono::year_month_day;
    using std::unordered_map;
    using ribomation::weatherdata::TemperatureUnit;
    using namespace std::string_literals;

    auto to_unit(double fahrenheit, TemperatureUnit unit) -> double {
        if (unit == TemperatureUnit::Fahrenheit) {
            return fahrenheit;
        }
        if (unit == TemperatureUnit::Celsius) {
            return (fahrenheit - 32.0) * 5.0 / 9.0;
        }
        if (unit == TemperatureUnit::Kelvin) {
            return 273.15 + to_unit(fahrenheit, TemperatureUnit::Celsius);
        }
        throw std::invalid_argument{"invalid temperature unit"};
    }

    auto as_string(TemperatureUnit unit) -> string {
        if (unit == TemperatureUnit::Celsius) return "°C";
        if (unit == TemperatureUnit::Fahrenheit) return "°F";
        return "K";
    }

    auto to_mm(double inches) -> double {
        return inches * 25.4;
    }

    auto parse_double(string const& txt) -> double {
        auto value = 0.0;
        auto first = txt.data();
        auto last = first + txt.size();
        auto [end, errcode] = std::from_chars(first, last, value);
        if (errcode != std::errc{} || end != last) {
            throw std::invalid_argument{"invalid numeric value: '"s + txt + "'"s};
        }
        return value;
    }

    auto parse_optional(string const& txt) -> optional<double> {
        if (txt.empty()) return std::nullopt;
        return parse_double(txt);
    }

    auto to_measurement(string const& field_value, double missing_marker) -> optional<double> {
        auto value = parse_double(field_value);
        if (value != missing_marker) return value;
        return std::nullopt;
    }

    auto to_date(const std::string& text) -> year_month_day {
        auto ymd = year_month_day{};
        auto buf = std::istringstream{text};
        buf >> std::chrono::parse("%F", ymd); // %F == %Y-%m-%d
        if (not buf) {
            throw std::runtime_error("invalid date '" + text + "'");
        }
        return ymd;
    }
}

namespace ribomation::weatherdata {

    void ObservationMapper::operator()(CSVHeader const& hdr, Fields const& fields) {
        using conversions::parse_optional;
        using conversions::to_measurement;
        using conversions::to_date;
        using conversions::to_unit;
        using conversions::to_mm;
        using conversions::as_string;

        auto station_id = fields.at(hdr.getIndex("STATION"));
        if (not Station::repo.contains(station_id)) {
            auto name = fields.at(hdr.getIndex("NAME"));
            auto lat = parse_optional(fields.at(hdr.getIndex("LATITUDE")));
            auto lng = parse_optional(fields.at(hdr.getIndex("LONGITUDE")));
            auto elv = parse_optional(fields.at(hdr.getIndex("ELEVATION")));
            auto s = Station{.id = station_id, .name = name, .latitude = lat, .longitude = lng, .elevation = elv};
            Station::repo.emplace(station_id, s);
        }
        obs.station_id = station_id;
        obs.station = &Station::repo.at(station_id);

        obs.date = to_date(fields.at(hdr.getIndex("DATE")));
        obs.temp_avg = to_measurement( fields.at(hdr.getIndex("TEMP")), Observation::missing_temp);
        obs.temp_min = to_measurement( fields.at(hdr.getIndex("MIN")), Observation::missing_temp);
        obs.temp_max = to_measurement( fields.at(hdr.getIndex("MAX")), Observation::missing_temp);
        obs.precipitation = to_measurement( fields.at(hdr.getIndex("PRCP")), Observation::missing_prcp);

        obs.unit = unit;
        if (obs.temp_avg) obs.temp_avg = to_unit(*obs.temp_avg, obs.unit);
        if (obs.temp_min) obs.temp_min = to_unit(*obs.temp_min, obs.unit);
        if (obs.temp_max) obs.temp_max = to_unit(*obs.temp_max, obs.unit);

        obs.prcp_mm = prcp_mm;
        if (obs.precipitation && obs.prcp_mm) obs.precipitation = to_mm(*obs.precipitation);
    }

    auto Station::to_string() const -> string {
        auto value = [](optional<double> const& v) -> string {
            return v ? std::format("{}", *v) : "-"s;
        };
        return std::format("[{}] {} <{}, {}, {}>", id, name, value(latitude), value(longitude), value(elevation));
    }

    auto Observation::to_string() const -> string {
        auto value = [](optional<double> const& v) -> string {
            return v ? std::format("{:.1f}", *v) : "-"s;
        };
        auto name = (station && not station->name.empty()) ? station->name : station_id;
        return std::format("{} {}: avg/min/max {}/{}/{} {}, prcp {} {}",
                           date, name,
                           value(temp_avg), value(temp_min), value(temp_max), conversions::as_string(unit),
                           value(precipitation), prcp_mm ? "mm" : "in");
    }

}
