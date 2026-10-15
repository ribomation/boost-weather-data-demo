#include <cmath>
#include <deque>
#include <format>
#include <optional>
#include <string>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "aggregator.hxx"

using ribomation::weatherdata::Aggregator;
using ribomation::weatherdata::Observation;
using ribomation::weatherdata::Ranking;
using ribomation::weatherdata::Station;
using ribomation::weatherdata::StationStats;
using Catch::Matchers::WithinAbs;

namespace {
    using std::string;
    using std::vector;
    using std::optional;

    auto observation(Station& s, optional<double> temp) -> Observation {
        return Observation{.station_id = s.id, .station = &s, .temp_avg = temp};
    }

    // feeds the temperatures of one station, in a row, as they come from a CSV file
    void feed(Aggregator& agg, Station& s, vector<optional<double>> const& temps) {
        for (auto t : temps) agg(observation(s, t));
    }

    auto ids(vector<StationStats const*> const& stats) -> vector<string> {
        auto result = vector<string>{};
        for (auto s : stats) result.push_back(s->id());
        return result;
    }
}

TEST_CASE("aggregator") {
    auto uppsala = Station{.id = "S1", .name = "UPPSALA"};
    auto kiruna  = Station{.id = "S2", .name = "KIRUNA"};
    auto lund    = Station{.id = "S3", .name = "LUND"};
    auto visby   = Station{.id = "S4", .name = "VISBY"};

    SECTION("StationStats") {
        auto stats = StationStats{&uppsala};

        SECTION("id and name come from the station") {
            CHECK(stats.id() == "S1");
            CHECK(stats.name() == "UPPSALA");
        }

        SECTION("without station") {
            auto anonymous = StationStats{"S9", nullptr};
            CHECK(anonymous.id() == "S9");
            CHECK(anonymous.name().empty());
        }

        SECTION("new stats are empty") {
            CHECK(stats.days() == 0);
            CHECK(stats.count() == 0);
            CHECK(std::isnan(stats.mean()));
        }

        SECTION("mean, min and max of the daily temperatures") {
            stats.add(observation(uppsala, 10.0));
            stats.add(observation(uppsala, 30.0));
            stats.add(observation(uppsala, 20.0));
            CHECK(stats.days() == 3);
            CHECK(stats.count() == 3);
            CHECK_THAT(stats.mean(), WithinAbs(20.0, 1e-9));
            CHECK_THAT(stats.min(), WithinAbs(10.0, 1e-9));
            CHECK_THAT(stats.max(), WithinAbs(30.0, 1e-9));
        }

        SECTION("negative temperatures") {
            stats.add(observation(uppsala, -12.5));
            stats.add(observation(uppsala, -2.5));
            CHECK_THAT(stats.mean(), WithinAbs(-7.5, 1e-9));
            CHECK_THAT(stats.min(), WithinAbs(-12.5, 1e-9));
            CHECK_THAT(stats.max(), WithinAbs(-2.5, 1e-9));
        }

        SECTION("a day without temperature counts as a day, but not in the stats") {
            stats.add(observation(uppsala, 10.0));
            stats.add(observation(uppsala, std::nullopt));
            stats.add(observation(uppsala, 20.0));
            CHECK(stats.days() == 3);
            CHECK(stats.count() == 2);
            CHECK_THAT(stats.mean(), WithinAbs(15.0, 1e-9));
        }

        SECTION("only days without temperature") {
            stats.add(observation(uppsala, std::nullopt));
            stats.add(observation(uppsala, std::nullopt));
            CHECK(stats.days() == 2);
            CHECK(stats.count() == 0);
            CHECK(std::isnan(stats.mean()));
        }
    }

    SECTION("Aggregator") {
        auto agg = Aggregator{};

        SECTION("empty") {
            CHECK(agg.stations() == 0);
            CHECK(agg.observations() == 0);
            auto r = agg.rank(5, 0);
            CHECK(r.warmest.empty());
            CHECK(r.coldest.empty());
            CHECK(r.eligible == 0);
        }

        SECTION("counts stations and observations") {
            feed(agg, uppsala, {1.0, 2.0, 3.0});
            feed(agg, kiruna, {-5.0, std::nullopt});
            CHECK(agg.stations() == 2);
            CHECK(agg.observations() == 5);
        }

        SECTION("rows of one station are aggregated together") {
            feed(agg, uppsala, {10.0, 20.0, std::nullopt});
            auto r = agg.rank(1, 0);
            REQUIRE(r.warmest.size() == 1);
            auto const& s = *r.warmest[0];
            CHECK(s.id() == "S1");
            CHECK(s.days() == 3);
            CHECK(s.count() == 2);
            CHECK_THAT(s.mean(), WithinAbs(15.0, 1e-9));
        }

        SECTION("a station that comes back later, is the same station") {
            feed(agg, uppsala, {10.0});
            feed(agg, kiruna, {-10.0});
            feed(agg, uppsala, {20.0});
            CHECK(agg.stations() == 2);
            auto r = agg.rank(2, 0);
            REQUIRE(r.warmest.size() == 2);
            CHECK(r.warmest[0]->id() == "S1");
            CHECK(r.warmest[0]->count() == 2);
            CHECK_THAT(r.warmest[0]->mean(), WithinAbs(15.0, 1e-9));
        }

        SECTION("rank") {
            feed(agg, uppsala, {5.0, 7.0});      // mean  6
            feed(agg, kiruna, {-15.0, -9.0});    // mean -12
            feed(agg, lund, {9.0, 11.0});        // mean 10
            feed(agg, visby, {7.0, 9.0});        // mean  8

            SECTION("warmest and coldest, in order") {
                auto r = agg.rank(2, 0);
                CHECK(ids(r.warmest) == vector<string>{"S3", "S4"});
                CHECK(ids(r.coldest) == vector<string>{"S2", "S1"});
                CHECK(r.eligible == 4);
            }

            SECTION("all stations") {
                auto r = agg.rank(4, 0);
                CHECK(ids(r.warmest) == vector<string>{"S3", "S4", "S1", "S2"});
                CHECK(ids(r.coldest) == vector<string>{"S2", "S1", "S4", "S3"});
            }

            SECTION("n larger than the number of stations") {
                auto r = agg.rank(10, 0);
                CHECK(r.warmest.size() == 4);
                CHECK(r.coldest.size() == 4);
            }

            SECTION("n is zero") {
                auto r = agg.rank(0, 0);
                CHECK(r.warmest.empty());
                CHECK(r.coldest.empty());
                CHECK(r.eligible == 4);
            }

            SECTION("ranking does not change the aggregator") {
                auto first = agg.rank(2, 0);
                auto again = agg.rank(2, 0);
                CHECK(ids(first.warmest) == ids(again.warmest));
                CHECK(ids(first.coldest) == ids(again.coldest));
            }
        }

        SECTION("observation without station") {
            auto obs = Observation{.station_id = "S9", .station = nullptr, .temp_avg = 12.0};
            agg(obs);
            agg(obs);
            CHECK(agg.stations() == 1);
            auto r = agg.rank(1, 0);
            REQUIRE(r.warmest.size() == 1);
            CHECK(r.warmest[0]->id() == "S9");
            CHECK(r.warmest[0]->name().empty());
            CHECK(r.warmest[0]->count() == 2);
        }

        SECTION("stations without temperatures are never ranked") {
            feed(agg, uppsala, {5.0});
            feed(agg, kiruna, {std::nullopt, std::nullopt});   // mean NaN
            feed(agg, lund, {8.0});
            feed(agg, visby, {std::nullopt});                  // mean NaN

            auto r = agg.rank(10, 0);
            CHECK(r.eligible == 2);
            CHECK(ids(r.warmest) == vector<string>{"S3", "S1"});
            CHECK(ids(r.coldest) == vector<string>{"S1", "S3"});
        }

        SECTION("min_days counts days with a temperature") {
            feed(agg, uppsala, {5.0, 6.0, 7.0});                                // 3 temp days
            feed(agg, kiruna, {30.0, std::nullopt, std::nullopt, std::nullopt}); // 4 days, 1 temp day
            feed(agg, lund, {8.0, 9.0});                                        // 2 temp days

            auto r = agg.rank(5, 2);
            CHECK(r.eligible == 2);
            CHECK(ids(r.warmest) == vector<string>{"S3", "S1"});   // KIRUNA is out, despite its 30 degrees

            CHECK(agg.rank(5, 3).eligible == 1);
            CHECK(agg.rank(5, 4).eligible == 0);
            CHECK(agg.rank(5, 4).warmest.empty());
        }

        SECTION("many stations") {
            // forces the map to rehash, while current_ points into it
            auto stations = std::deque<Station>{};
            for (auto k = 0; k < 1000; ++k) {
                auto& s = stations.emplace_back(Station{.id = std::format("S{:04}", k), .name = "STATION"});
                feed(agg, s, {static_cast<double>(k), static_cast<double>(k) + 2.0});
            }
            CHECK(agg.stations() == 1000);
            CHECK(agg.observations() == 2000);

            auto r = agg.rank(3, 2);
            CHECK(r.eligible == 1000);
            CHECK(ids(r.warmest) == vector<string>{"S0999", "S0998", "S0997"});
            CHECK(ids(r.coldest) == vector<string>{"S0000", "S0001", "S0002"});
            CHECK_THAT(r.warmest[0]->mean(), WithinAbs(1000.0, 1e-9));
        }
    }
}
