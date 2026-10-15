#pragma once
#include <format>
#include <ostream>
#include <cstdint>

namespace ribomation::weatherdata {
    struct PipelineCounters {
        uint64_t uncompress_bytes{};
        uint64_t compress_bytes{};
        uint64_t files{};
    };

    inline auto operator<<(std::ostream& os, PipelineCounters const& c) -> std::ostream& {
        auto percentage = c.uncompress_bytes ? 100.0 * c.compress_bytes / c.uncompress_bytes : 0.0;
        return os << std::format(
                   "-- Pipeline Counts --\n"
                   " # files     : {:>12}\n"
                   " uncompressed: {:>12}\n"
                   " compressed  : {:>12} ({:.1f}% of uncompressed)\n"
                   , c.files, c.uncompress_bytes, c.compress_bytes, percentage);
    }
}
