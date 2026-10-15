#include <iostream>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/null.hpp>
#include "config.hxx"
#include "options.hxx"
#include "http-source.hxx"
#include "elapsed.hxx"

using ribomation::weatherdata::Config;
using ribomation::weatherdata::Options;
using ribomation::weatherdata::http_source;
using ribomation::weatherdata::Elapsed;
namespace io = boost::iostreams;

int main(int argc, char** argv) {
    try {
        auto elapsed = Elapsed{};
        auto opts = Options::parse(argc, argv);
        if (not opts) return 0;
        auto cfg = Config::load(opts->config_file);

        auto url = http_source::archive_url(cfg.url, opts->year);
        auto file = http_source::archive_file(cfg.cache_dir, opts->year);
        constexpr auto MB = 1024 * 1024.0;

        std::cout << "fetching " << url << " ...\n";
        if (opts->download_only) {
            auto num_bytes = http_source::download(url, file);
            auto mb = static_cast<double>(num_bytes) / MB;
            std::cout << "saved " << file.string() << " (" << mb << " MB)\n";
        } else {
            auto num_bytes = io::copy(http_source{url}, io::null_sink{});
            auto mb = static_cast<double>(num_bytes) / MB;
            std::cout << "streamed " << mb << " MB\n";
        }
        std::cout << "elapsed " << elapsed.time() << " ms\n";
    } catch (std::exception const& x) {
        std::cout << "Program error: " << x.what() << "\n";
        std::cout << "Use '--help' for help text\n";
        return 1;
    }
}
