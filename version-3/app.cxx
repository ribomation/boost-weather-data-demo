#include <filesystem>
#include <format>
#include <iostream>
// #include <flat_map>

#include <boost/iostreams/device/file.hpp>

#include "config.hxx"
#include "options.hxx"
#include "http-source.hxx"
#include "elapsed.hxx"
#include "reading-pipeline.hxx"

// using m = std::flat_map<std::string, StationStats>;
namespace io = boost::iostreams;
namespace fs = std::filesystem;
using ribomation::weatherdata::Config;
using ribomation::weatherdata::Options;
using ribomation::weatherdata::http_source;
using ribomation::weatherdata::Elapsed;
using ribomation::weatherdata::pipeline;
using ribomation::weatherdata::Observation;


int main(int argc, char** argv) {
    // auto x = std::flat_map<std::string,unsigned>{};
    // x.at("abc")= 42;
    try {
        auto elapsed = Elapsed{};
        auto opts = Options::parse(argc, argv);
        if (not opts) return 0;
        auto cfg = Config::load(opts->config_file);

        auto url = http_source::archive_url(cfg.url, opts->year);
        auto file = http_source::archive_file(cfg.cache_dir, opts->year);
        constexpr auto MB = 1024 * 1024.0;

        auto print = [](Observation& obs) {
            std::cout << obs << "\n";
        };
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
        } else if (cfg.cache) {
            if (fs::exists(file)) {
                std::cout << "using cached " << file.string() << "\n";
            } else {
                download();
            }
            pipeline(io::file_source{file.string(), std::ios::binary}, cfg, print);
        } else {
            std::cout << "streaming " << url << " ...\n";
            pipeline(http_source{url}, cfg, print);
        }
        std::cout << "elapsed " << elapsed.to_string() << "\n";
    } catch (std::exception const& x) {
        std::cout << "Program error: " << x.what() << "\n";
        std::cout << "Use '--help' for help text\n";
        return 1;
    }
}
