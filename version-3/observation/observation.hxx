#pragma once
#include <ostream>
#include <string>
#include <format>
#include <optional>
#include <chrono>
#include <unordered_map>
#include "config.hxx"
#include "csv-reader.hxx"


namespace ribomation::weatherdata {
    using std::string;
    using std::optional;
    using std::chrono::year_month_day;
    using std::unordered_map;
    using ribomation::weatherdata::TemperatureUnit;
    using namespace std::string_literals;

    struct Station {
        string id{};
        string name{};
        optional<double> latitude{};
        optional<double> longitude{};
        optional<double> elevation{};
        inline static unordered_map<string, Station> repo{};

        [[nodiscard]] auto to_string() const -> string;
    };

    inline auto operator<<(std::ostream& os, Station const& s) -> std::ostream& {
        return os << s.to_string();
    }


    struct Observation {
        constexpr static double missing_temp = 9999.9; // TEMP, MAX, MIN
        constexpr static double missing_prcp = 99.99; // PRCP

        string           station_id{};
        Station*         station = nullptr;
        year_month_day   date{};
        optional<double> temp_avg{};
        optional<double> temp_min{};
        optional<double> temp_max{};
        optional<double> precipitation{};
        TemperatureUnit  unit = TemperatureUnit::Celsius;
        bool             prcp_mm = true;

        [[nodiscard]] auto to_string() const -> string;
    };

    struct ObservationMapper : FieldsMapper {
        TemperatureUnit unit;
        bool prcp_mm;
        Observation obs{};

        ObservationMapper(TemperatureUnit unit, bool prcp_mm)
            : unit(unit), prcp_mm(prcp_mm) { }

        void operator()(CSVHeader const& hdr, Fields const& fields) override;
    };

    inline auto operator<<(std::ostream& os, Observation const& obs) -> std::ostream& {
        return os << obs.to_string();
    }

    namespace conversions {
        auto to_date(const std::string& text) -> std::chrono::year_month_day;
        auto to_measurement(string const& field_value, double missing_marker) -> optional<double>;
        auto to_unit(double fahrenheit, TemperatureUnit unit) -> double;
        auto to_mm(double inches) -> double;
        auto parse_double(string const& txt) -> double;
        auto parse_optional(string const& txt) -> optional<double>;
        auto as_string(TemperatureUnit unit) -> string;
    }
}

template<>
struct std::formatter<ribomation::weatherdata::Station> : std::formatter<std::string> {
    auto format(ribomation::weatherdata::Station const& s, std::format_context& ctx) const {
        return std::formatter<std::string>::format(s.to_string(), ctx);
    }
};

template<>
struct std::formatter<ribomation::weatherdata::Observation> : std::formatter<std::string> {
    auto format(ribomation::weatherdata::Observation const& obs, std::format_context& ctx) const {
        return std::formatter<std::string>::format(obs.to_string(), ctx);
    }
};
