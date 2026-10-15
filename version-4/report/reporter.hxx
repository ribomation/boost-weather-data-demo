#pragma once
#include <ostream>
#include <string_view>
#include "aggregator.hxx"
#include "pipeline-counters.hxx"
#include "elapsed.hxx"

namespace ribomation::weatherdata {
    using std::ostream;
    using std::string_view;
    using std::vector;

     class Reporter {
          std::ostream& out;
          Config const& cfg;
      public:
          Reporter(ostream& out_, Config const& cfg_) : out{out_}, cfg{cfg_} {}

          void ranking(string_view title, vector<StationStats const*> const& list) const;
          void summary(PipelineCounters const& p, Aggregator const& a, Ranking const& r, Elapsed const& e) const;
      };

}
