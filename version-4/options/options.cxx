#include  <string>
#include  <iostream>
#include  <print>
#include <boost/program_options.hpp>
#include "options.hxx"

namespace ribomation::weatherdata {
    namespace po = boost::program_options;
    using std::string;

    auto Options::parse(int argc, char** argv) -> optional<Options> {
        auto opts = Options{};
        auto cli = po::options_description{APP_NAME};
        cli.add_options()
                ("help,h", "show this help and quit")
                ("version,V", "show version and quit")
                ("year,y", po::value<unsigned>(&opts.year)->required(),
                 "the year to process")
                ("top,t", po::value<unsigned>(&opts.top)->default_value(opts.top),
                 "number of stations to show coldest/hottest")
                ("download,d", po::bool_switch(&opts.download_only),
                 "just download and save/cache the file")
                ("config,c", po::value<string>(&opts.config_file)->default_value(opts.config_file),
                 "alt. config file");

        auto parsed = po::parse_command_line(argc, argv, cli);
        auto values = po::variables_map{};
        po::store(parsed, values);

        if (values.contains("help")) {
            std::cout << cli;
            return std::nullopt;
        }
        if (values.contains("version")) {
            std::println("{}, version {}, step {}", APP_NAME, APP_VERSION, APP_STEP);
            return std::nullopt;
        }
        po::notify(values);

        return opts;
    }

    auto operator<<(std::ostream& os, Options const& o) -> std::ostream& {
        os << "-- Options --\n"
                << " year=" << o.year << "\n"
                << " top=" << o.top << "\n"
                << " config=" << o.config_file << "\n"
                << " download=" << std::boolalpha << o.download_only << "\n";
        return os;
    }
}
