#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <string>
#include <utility>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>

#include "observation.hxx"


namespace ribomation::weatherdata {
    namespace acc = boost::accumulators;

    class StationStats {
        using Stats = acc::accumulator_set<double,
                                           acc::stats<acc::tag::count, acc::tag::mean,
                                                      acc::tag::min,   acc::tag::max>>;

        string         id_;
        Station const* station_;  // might be null, if the observation had no station
        unsigned       days_ = 0; // observations, with or without a temperature
        Stats temp_{}; // daily mean temperatures

    public:
        explicit StationStats(Station const* s) : StationStats{s ? s->id : string{}, s} { }
        StationStats(string id, Station const* s) : id_{std::move(id)}, station_{s} { }

        void add(Observation const& obs) {
            ++days_;
            if (obs.temp_avg) temp_(*obs.temp_avg);
        }

        [[nodiscard]] auto id() const -> string const& { return id_; }
        [[nodiscard]] auto name() const -> string const& {
            static auto const no_name = string{};
            return station_ ? station_->name : no_name;
        }
        [[nodiscard]] auto days() const { return days_; }
        [[nodiscard]] auto mean() const -> double { return acc::mean(temp_); } // NaN if no temp_days
        [[nodiscard]] auto min() const -> double { return acc::min(temp_); }
        [[nodiscard]] auto max() const -> double { return acc::max(temp_); }
        [[nodiscard]] auto count() const -> std::size_t { return acc::count(temp_); }
    };

    struct Ranking {
        std::vector<StationStats const*> warmest{};
        std::vector<StationStats const*> coldest{};
        std::size_t eligible = 0; // stations with at least min_days days with a temperature (and at least one)
    };

    class Aggregator {
        std::unordered_map<string, StationStats> stations_{};
        StationStats* current_ = nullptr; // the rows of a station come in a row
        std::uint64_t observations_ = 0;

    public:
        void operator()(Observation const& obs);
        [[nodiscard]] auto rank(unsigned n, unsigned min_days) const -> Ranking;

        [[nodiscard]] auto stations() const { return stations_.size(); }
        [[nodiscard]] auto observations() const { return observations_; }
    };
}
