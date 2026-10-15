#include <algorithm>
#include <ranges>
#include "aggregator.hxx"

namespace ribomation::weatherdata {
    namespace r = std::ranges;
    namespace v = std::views;

    void Aggregator::operator()(Observation const& obs) {
        ++observations_;
        if (current_ == nullptr || current_->id() != obs.station_id) {
            auto [it, inserted] = stations_.try_emplace(obs.station_id, obs.station_id, obs.station);
            current_ = &it->second; // stable, also when the map rehashes
        }
        current_->add(obs);
    }

    auto Aggregator::rank(unsigned n, unsigned min_days) const -> Ranking {
        min_days = std::max(min_days, 1U); // a station without temperatures has mean NaN, which breaks the sorting
        auto eligible = stations_
                        | v::values
                        | v::filter([=](auto const& s) { return s.count() >= min_days; })
                        | v::transform([](auto const& s) { return &s; })
                        | r::to<std::vector<StationStats const*>>();

        auto k = std::min<std::size_t>(n, eligible.size());
        auto warmest = eligible;
        r::partial_sort(warmest , warmest.begin()  + k, r::greater{}, &StationStats::mean);
        r::partial_sort(eligible, eligible.begin() + k, r::less{}   , &StationStats::mean);

        auto result = Ranking{.eligible = eligible.size()};
        result.warmest.assign(warmest.begin() , warmest.begin()  + k);
        result.coldest.assign(eligible.begin(), eligible.begin() + k);
        return result;
    }
}
