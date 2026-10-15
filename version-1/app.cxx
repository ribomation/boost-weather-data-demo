#include <iostream>
#include "config.hxx"
#include "options.hxx"

using ribomation::weatherdata::Config;
using ribomation::weatherdata::Options;

int main(int argc, char** argv) {
    try {
        auto opts = Options::parse(argc, argv);
        if (not opts) return 0;
        std::cout << *opts;

        auto cfg = Config::load(opts->config_file);
        std::cout << cfg;
    } catch (std::exception const& x) {
        std::cout << "Program error: " << x.what() << "\n";
        std::cout << "Use '--help' for help text\n";
        return 1;
    }
}
