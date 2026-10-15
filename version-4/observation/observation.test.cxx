#include <chrono>
#include <format>
#include <sstream>
#include <stdexcept>
#include <string>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "observation.hxx"

using ribomation::weatherdata::Station;
using ribomation::weatherdata::Observation;
using ribomation::weatherdata::TemperatureUnit;
using ribomation::weatherdata::ObservationMapper;
using ribomation::weatherdata::CSVHeader;
using ribomation::weatherdata::CSVReader;
using ribomation::weatherdata::Fields;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::StartsWith;

namespace {
    using namespace std::string_literals;
    using namespace std::chrono;

    auto gsod_header() -> CSVHeader {
        auto header = CSVHeader{};
        header.init(CSVReader::parse(
            R"("STATION","DATE","LATITUDE","LONGITUDE","ELEVATION","NAME","TEMP","MAX","MIN","PRCP")"));
        return header;
    }
}

TEST_CASE("observation") {
    auto station = Station{"SE01", "Uppsala", 59.86, 17.64, 24.0};

    SECTION("Station") {
        auto const expected = "[SE01] Uppsala <59.86, 17.64, 24>"s;

        SECTION("to_string") {
            CHECK(station.to_string() == expected);
        }

        SECTION("operator<<") {
            auto buf = std::ostringstream{};
            buf << station;
            CHECK(buf.str() == expected);
        }

        SECTION("std::format") {
            CHECK(std::format("{}", station) == expected);
        }

        SECTION("to_string without coordinates") {
            station.latitude = station.longitude = station.elevation = std::nullopt;
            CHECK(station.to_string() == "[SE01] Uppsala <-, -, ->");
        }

        SECTION("std::format with width") {
            CHECK(std::format("|{:>40}|", station) == "|       [SE01] Uppsala <59.86, 17.64, 24>|"s);
        }
    }

    SECTION("Conversions") {
        SECTION("to_unit") {
            using ribomation::weatherdata::conversions::to_unit;
            CHECK_THAT(to_unit(212.0, TemperatureUnit::Fahrenheit), WithinAbs(212.0, 1e-9));
            CHECK_THAT(to_unit(212.0, TemperatureUnit::Celsius), WithinAbs(100.0, 1e-9));
            CHECK_THAT(to_unit(32.0, TemperatureUnit::Kelvin), WithinAbs(273.15, 1e-9));
            CHECK_THROWS_WITH(to_unit(32.0, static_cast<TemperatureUnit>(42)), "invalid temperature unit");
        }

        SECTION("as_string") {
            using ribomation::weatherdata::conversions::as_string;
            CHECK(as_string(TemperatureUnit::Celsius) == "°C");
            CHECK(as_string(TemperatureUnit::Fahrenheit) == "°F");
            CHECK(as_string(TemperatureUnit::Kelvin) == "K");
        }

        SECTION("to_mm") {
            using ribomation::weatherdata::conversions::to_mm;
            CHECK_THAT(to_mm(1.0), WithinAbs(25.4, 1e-9));
            CHECK_THAT(to_mm(0.0), WithinAbs(0.0, 1e-9));
        }

        SECTION("parse_double") {
            using ribomation::weatherdata::conversions::parse_double;
            CHECK_THAT(parse_double("42.5"), WithinAbs(42.5, 1e-9));
            CHECK_THAT(parse_double("-3"), WithinAbs(-3.0, 1e-9));
            CHECK_THROWS_WITH(parse_double("12.5x"), ContainsSubstring("invalid numeric value"));
            CHECK_THROWS_AS(parse_double(""), std::invalid_argument);
        }

        SECTION("parse_optional") {
            using ribomation::weatherdata::conversions::parse_optional;
            CHECK_THAT(parse_optional("36.695").value(), WithinAbs(36.695, 1e-9));
            CHECK_FALSE(parse_optional("").has_value());
            CHECK_THROWS_WITH(parse_optional("north"), ContainsSubstring("invalid numeric value"));
        }

        SECTION("to_measurement") {
            using ribomation::weatherdata::conversions::to_measurement;
            auto value = to_measurement("17.5", 9999.9);
            REQUIRE(value.has_value());
            CHECK_THAT(*value, WithinAbs(17.5, 1e-9));
            CHECK_FALSE(to_measurement("9999.9", 9999.9).has_value());
        }

        SECTION("to_date") {
            using ribomation::weatherdata::conversions::to_date;
            CHECK(to_date("1955-06-21") == 1955y / June / 21d);
            CHECK_THROWS_WITH(to_date("not-a-date"), ContainsSubstring("invalid date"));
        }
    }

    SECTION("Observation") {
        auto obs = Observation{
            .station_id = "SE01",
            .station = &station,
            .date = 2026y / October / 12d,
            .temp_avg = 12.34,
            .temp_min = 8.0,
            .temp_max = 16.75,
            .precipitation = 1.27,
        };
        auto const expected = "2026-10-12 Uppsala: avg/min/max 12.3/8.0/16.8 °C, prcp 1.3 mm"s;

        SECTION("to_string") {
            CHECK(obs.to_string() == expected);
        }

        SECTION("to_string without station") {
            obs.station = nullptr;
            CHECK_THAT(obs.to_string(), ContainsSubstring(" SE01: "));
        }

        SECTION("to_string with missing values") {
            obs.temp_max = std::nullopt;
            obs.precipitation = std::nullopt;
            CHECK_THAT(obs.to_string(), ContainsSubstring("12.3/8.0/- °C, prcp - mm"));
        }

        SECTION("to_string with other units") {
            obs.unit = TemperatureUnit::Fahrenheit;
            CHECK_THAT(obs.to_string(), ContainsSubstring(" °F,"));
            obs.unit = TemperatureUnit::Kelvin;
            CHECK_THAT(obs.to_string(), ContainsSubstring(" K,"));
        }

        SECTION("to_string with precipitation in inches") {
            obs.prcp_mm = false;
            CHECK_THAT(obs.to_string(), ContainsSubstring(", prcp 1.3 in"));
        }

        SECTION("operator<<") {
            auto buf = std::ostringstream{};
            buf << obs;
            CHECK(buf.str() == expected);
        }

        SECTION("std::format") {
            CHECK(std::format("{}", obs) == expected);
        }
    }

    SECTION("ObservationMapper") {
        Station::repo.clear();
        auto header = gsod_header();
        auto fields = CSVReader::parse(
            R"("99999913762","1945-03-01","36.695","-76.13556","4.9",)"
            R"("FENTRESS NAVAL AUXILIARY FIELD, VA US","  44.3","  50.0","  32.0"," 0.50")");

        SECTION("celsius and mm") {
            auto mapper = ObservationMapper{TemperatureUnit::Celsius, true};
            mapper(header, fields);
            auto const& obs = mapper.obs;
            CHECK(obs.station_id == "99999913762");
            CHECK(obs.date == 1945y / March / 1d);
            CHECK(obs.unit == TemperatureUnit::Celsius);
            CHECK(obs.prcp_mm);
            REQUIRE(obs.temp_avg.has_value());
            REQUIRE(obs.temp_min.has_value());
            REQUIRE(obs.temp_max.has_value());
            REQUIRE(obs.precipitation.has_value());
            CHECK_THAT(*obs.temp_avg, WithinAbs(6.833, 1e-3));
            CHECK_THAT(*obs.temp_min, WithinAbs(0.0, 1e-9));
            CHECK_THAT(*obs.temp_max, WithinAbs(10.0, 1e-9));
            CHECK_THAT(*obs.precipitation, WithinAbs(12.7, 1e-9));
        }

        SECTION("fahrenheit and inches are kept as is") {
            auto mapper = ObservationMapper{TemperatureUnit::Fahrenheit, false};
            mapper(header, fields);
            auto const& obs = mapper.obs;
            CHECK_THAT(obs.temp_avg.value(), WithinAbs(44.3, 1e-9));
            CHECK_THAT(obs.temp_min.value(), WithinAbs(32.0, 1e-9));
            CHECK_THAT(obs.temp_max.value(), WithinAbs(50.0, 1e-9));
            CHECK_THAT(obs.precipitation.value(), WithinAbs(0.5, 1e-9));
            CHECK_FALSE(obs.prcp_mm);
        }

        SECTION("kelvin") {
            auto mapper = ObservationMapper{TemperatureUnit::Kelvin, true};
            mapper(header, fields);
            CHECK_THAT(mapper.obs.temp_min.value(), WithinAbs(273.15, 1e-9));
        }

        SECTION("missing values") {
            auto missing = CSVReader::parse(
                R"("99999913762","1945-03-02","36.695","-76.13556","4.9",)"
                R"("FENTRESS NAVAL AUXILIARY FIELD, VA US","9999.9","9999.9","9999.9","99.99")");
            auto mapper = ObservationMapper{TemperatureUnit::Celsius, true};
            mapper(header, missing);
            CHECK_FALSE(mapper.obs.temp_avg.has_value());
            CHECK_FALSE(mapper.obs.temp_min.has_value());
            CHECK_FALSE(mapper.obs.temp_max.has_value());
            CHECK_FALSE(mapper.obs.precipitation.has_value());
        }

        SECTION("station is added to the repo") {
            auto mapper = ObservationMapper{TemperatureUnit::Celsius, true};
            mapper(header, fields);
            REQUIRE(Station::repo.contains("99999913762"));
            auto const& s = Station::repo.at("99999913762");
            CHECK(s.name == "FENTRESS NAVAL AUXILIARY FIELD, VA US");
            CHECK_THAT(s.latitude.value(), WithinAbs(36.695, 1e-9));
            CHECK_THAT(s.longitude.value(), WithinAbs(-76.13556, 1e-9));
            CHECK_THAT(s.elevation.value(), WithinAbs(4.9, 1e-9));
            CHECK(mapper.obs.station == &s);
        }

        SECTION("station is added only once") {
            auto mapper = ObservationMapper{TemperatureUnit::Celsius, true};
            mapper(header, fields);
            auto first = mapper.obs.station;

            auto next_day = fields;
            next_day[1] = "1945-03-02";
            next_day[5] = "SOME OTHER NAME";
            mapper(header, next_day);

            CHECK(Station::repo.size() == 1);
            CHECK(mapper.obs.station == first);
            CHECK(mapper.obs.station->name == "FENTRESS NAVAL AUXILIARY FIELD, VA US");
            CHECK(mapper.obs.date == 1945y / March / 2d);
        }

        SECTION("station without metadata") {
            // e.g. 03864099999.csv in 1929.tar.gz, has empty LATITUDE, LONGITUDE, ELEVATION and NAME
            auto no_meta = CSVReader::parse(R"("03864099999","1929-10-01",,,,,"  55.0","  64.9","  46.0","99.99")");
            auto mapper = ObservationMapper{TemperatureUnit::Celsius, true};
            REQUIRE_NOTHROW(mapper(header, no_meta));

            REQUIRE(Station::repo.contains("03864099999"));
            auto const& s = Station::repo.at("03864099999");
            CHECK(s.name.empty());
            CHECK_FALSE(s.latitude.has_value());
            CHECK_FALSE(s.longitude.has_value());
            CHECK_FALSE(s.elevation.has_value());
            CHECK(mapper.obs.station == &Station::repo.at("03864099999"));
            CHECK(mapper.obs.date == 1929y / October / 1d);
            CHECK_THAT(mapper.obs.temp_avg.value(), WithinAbs(12.778, 1e-3));
            CHECK_FALSE(mapper.obs.precipitation.has_value());
        }

        SECTION("observation of station without name, shows the station id") {
            auto no_meta = CSVReader::parse(R"("03864099999","1929-10-01",,,,,"  55.0","  64.9","  46.0","99.99")");
            auto mapper = ObservationMapper{TemperatureUnit::Celsius, true};
            REQUIRE_NOTHROW(mapper(header, no_meta));
            CHECK_THAT(mapper.obs.to_string(), StartsWith("1929-10-01 03864099999: "));
        }

        SECTION("station without metadata does not stop the next station") {
            auto no_meta = CSVReader::parse(R"("03864099999","1929-10-01",,,,,"  55.0","  64.9","  46.0","99.99")");
            auto mapper = ObservationMapper{TemperatureUnit::Celsius, true};
            CHECK_NOTHROW(mapper(header, no_meta));
            CHECK_NOTHROW(mapper(header, fields));
            CHECK(Station::repo.size() == 2);
        }

        SECTION("invalid date") {
            fields[1] = "1945-13-01";
            auto mapper = ObservationMapper{TemperatureUnit::Celsius, true};
            CHECK_THROWS_WITH(mapper(header, fields), ContainsSubstring("invalid date"));
        }

        SECTION("invalid number") {
            fields[6] = "warm";
            auto mapper = ObservationMapper{TemperatureUnit::Celsius, true};
            CHECK_THROWS_WITH(mapper(header, fields), ContainsSubstring("invalid numeric value: 'warm'"));
        }

        SECTION("missing column") {
            auto partial = CSVHeader{};
            partial.init(Fields{"STATION", "DATE"});
            auto mapper = ObservationMapper{TemperatureUnit::Celsius, true};
            CHECK_THROWS_WITH(mapper(partial, fields), ContainsSubstring("missing field: NAME"));
        }

        SECTION("too few fields") {
            fields.resize(5);
            auto mapper = ObservationMapper{TemperatureUnit::Celsius, true};
            CHECK_THROWS_AS(mapper(header, fields), std::out_of_range);
        }

        Station::repo.clear();
    }
}
