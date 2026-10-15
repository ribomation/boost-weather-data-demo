#include <format>
#include "pipeline-counters.hxx"
#include "reporter.hxx"

namespace ribomation::weatherdata {
    constexpr auto MB = 1024 * 1024.0;
    using std::ostream;
    using std::string_view;
    using std::vector;
    using std::format;

    void Reporter::ranking(string_view title, vector<StationStats const*> const& list) const {
        auto unit = conversions::as_string(cfg.unit);
        out << format("{} ({})\n", title, unit);

        // °C is 2 bytes in UTF-8, keep it out of padded fields                                                                                                                             
        out << format("{:>3}  {:<11}  {:<32}  {:>6}  {:>6}  {:>6}  {:>4}\n",
                           "#", "Station", "Name", "Mean", "Min", "Max", "Days");

        for (auto rank = 1U; auto s: list) {
            out << format("{:>3}  {:<11}  {:<32.32}  {:>6.1f}  {:>6.1f}  {:>6.1f}  {:>4}\n",
                               rank++, s->id(), s->name(), s->mean(), s->min(), s->max(), s->count());
        }
    }

    void Reporter::summary(PipelineCounters const& p, Aggregator const& a, Ranking const& r, Elapsed const& e) const {
        auto ms = static_cast<double>(e.time());
        auto in = static_cast<double>(p.uncompress_bytes) / MB;

        out << format("{} stations ({} with at least {} days), {} observations\n",
                           a.stations(), r.eligible, cfg.min_days, a.observations());

        out << format("{:.1f} MB compressed, {:.1f} MB uncompressed, {} ({:.1f} MB/s)\n",
                           p.compress_bytes / MB, in, e.to_string(), in / (ms / 1000.0));
    }
}
