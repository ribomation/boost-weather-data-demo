#pragma once
#include <string>
#include <chrono>
#include <utility>

namespace ribomation::weatherdata {
    namespace ch = std::chrono;
    using TimePnt = ch::time_point<ch::system_clock>;
    using std::string;

    class Elapsed {
        string name_;
        TimePnt start;

    public:
        Elapsed() : start{ch::high_resolution_clock::now()} { }

        explicit Elapsed(string n)
            : name_{std::move(n)},
              start{ch::high_resolution_clock::now()} { }

        [[nodiscard]] auto time() const {
            auto stop = ch::high_resolution_clock::now();
            auto interval = ch::duration_cast<ch::milliseconds>(stop - start);
            return interval.count();
        }

        [[nodiscard]] auto const& name() const {
            return name_;
        }
    };
}
