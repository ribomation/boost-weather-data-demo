#include <filesystem>
#include <format>
#include <iostream>

#include <boost/iostreams/device/file.hpp>

#include "config.hxx"
#include "options.hxx"
#include "http-source.hxx"
#include "elapsed.hxx"
#include "reading-pipeline.hxx"
#include "aggregator.hxx"
#include "reporter.hxx"

namespace io = boost::iostreams;
namespace fs = std::filesystem;
using ribomation::weatherdata::Config;
using ribomation::weatherdata::Options;
using ribomation::weatherdata::http_source;
using ribomation::weatherdata::Elapsed;
using ribomation::weatherdata::pipeline;
using ribomation::weatherdata::Observation;
using ribomation::weatherdata::Aggregator;
using ribomation::weatherdata::PipelineCounters;
using ribomation::weatherdata::Reporter;


int main(int argc, char** argv) {
    auto elapsed = Elapsed{};
    try {
        auto opts = Options::parse(argc, argv);
        if (not opts) return 0;
        auto cfg = Config::load(opts->config_file);

        auto url = http_source::archive_url(cfg.url, opts->year);
        auto file = http_source::archive_file(cfg.cache_dir, opts->year);
        constexpr auto MB = 1024 * 1024.0;

        auto download = [&] {
            std::cout << "fetching " << url << " ...\n";
            auto num_bytes = http_source::download(url, file);
            auto mb = static_cast<double>(num_bytes) / MB;
            std::cout << std::format("saved {} ({:.2f} MB)\n", file.string(), mb);
        };

        if (opts->download_only) {
            if (fs::exists(file)) {
                std::cout << "already cached " << file.string() << "\n";
            } else {
                download();
            }
            std::cout << "Elapsed time: " << elapsed.to_string() << "\n";
            return 0;
        }

        auto aggregator = Aggregator{};
        auto cnt = PipelineCounters{};
        if (cfg.cache) {
            if (fs::exists(file)) {
                std::cout << "using cached " << file.string() << "\n";
            } else {
                download();
            }
            std::cout << "processing file " << file << " ...\n";
            cnt = pipeline(io::file_source{file.string(), std::ios::binary}, cfg, aggregator);
        } else {
            std::cout << "streaming url " << url << " ...\n";
            cnt = pipeline(http_source{url}, cfg, aggregator);
        }

        auto ranking = aggregator.rank(cfg.top, cfg.min_days);
        auto reporter = Reporter{std::cout, cfg};

        auto warmest = std::format("\nThe {} warmest stations in {}", ranking.warmest.size(), opts->year);
        reporter.ranking(warmest, ranking.warmest);

        auto coldest = std::format("\nThe {} coldest stations in {}", ranking.coldest.size(), opts->year);
        reporter.ranking(coldest, ranking.coldest);

        reporter.summary(cnt, aggregator, ranking, elapsed);
    } catch (std::exception const& x) {
        std::cout << "Program error: " << x.what() << "\n";
        std::cout << "Use '--help' for help text\n";
        return 1;
    }
}
